#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>

#include <uart.h>

#include "../../website/website_strings.h"
#include "ota_handler.h"

extern const uint8_t _interface_ota_html_start[] asm("_binary_interface_ota_html_start");
extern const uint8_t _interface_ota_html_end[] asm("_binary_interface_ota_html_end");

// Constants
constexpr const char* OTAHandler::GITHUB_API_URL;
constexpr int OTAHandler::HTTP_TIMEOUT_MS;
constexpr int OTAHandler::DOWNLOAD_TIMEOUT_MS;
constexpr size_t OTAHandler::DOWNLOAD_BUFFER_SIZE;
constexpr size_t OTAHandler::LOG_INTERVAL_KB;

OTAHandler& OTAHandler::getInstance()
{
    static OTAHandler instance;
    return instance;
}

void OTAHandler::init(WebServer* server)
{
    _server = server;
    resetOTAState();
    print_out("OTA Handler initialized");
}

void OTAHandler::resetOTAState()
{
    otaActive = false;
    otaComplete = false;
    otaError = false;
    otaBytesWritten = 0;
    otaTotalBytes = 0;
}

void OTAHandler::rebootWithDelay(int delaySeconds)
{
    print_out("Rebooting in %d seconds...", delaySeconds);
    for (int i = delaySeconds; i > 0; i--)
    {
        print_out("%d...", i);
        _server->handleClient();
        vTaskDelay(1000);
    }
    print_out("Rebooting now.");
    systemShutdown();
}

String OTAHandler::sanitizeString(const String& input, size_t maxLen)
{
    String result;
    result.reserve(min(input.length(), maxLen));
    for (size_t i = 0; i < input.length() && i < maxLen; i++)
    {
        char c = input[i];
        if (c >= 32 && c <= 126)
            result += c;
    }
    return result;
}

String OTAHandler::extractJsonValue(const String& json, const char* key)
{
    String searchKey = String("\"") + key + "\":\"";
    int start = json.indexOf(searchKey);
    if (start == -1)
        return "";
    start += searchKey.length();
    int end = json.indexOf("\"", start);
    return (end != -1) ? json.substring(start, end) : "";
}

void OTAHandler::handleOTAPage()
{
    if (!_server)
        return;

    resetOTAState();
    size_t htmlSize = _interface_ota_html_end - _interface_ota_html_start;
    _server->send_P(200, MIME_TYPE_HTML, (const char*) _interface_ota_html_start, htmlSize);
}

void OTAHandler::handleOTAUpload()
{
    HTTPUpload& upload = _server->upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        print_out("OTA Update Start: %s", upload.filename.c_str());
        _updating = true;
        _updateProgress = 0;
        resetOTAState();
        otaActive = true;

        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
        {
            print_out("OTA Error: Not enough space");
            Update.printError(Serial);
            _updating = false;
            otaActive = false;
            otaError = true;
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
            print_out("OTA Write Error");
            Update.printError(Serial);
            otaError = true;
            otaActive = false;
            _updating = false;
            return;
        }

        _updateProgress += upload.currentSize;
        otaBytesWritten += upload.currentSize;

        // Print progress every 10KB
        if (_updateProgress / 10240 > (_updateProgress - upload.currentSize) / 10240)
            print_out("OTA Progress: %d KB", _updateProgress / 1024);

        // Update totalBytes estimate
        if (upload.totalSize > 0)
            otaTotalBytes = upload.totalSize;
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        otaTotalBytes = upload.totalSize;
        otaBytesWritten = upload.totalSize;
        _updating = false;
        otaActive = false;

        if (!otaError && Update.end(true))
        {
            print_out("OTA Update Success: %u bytes", upload.totalSize);
            otaComplete = true;
        }
        else
        {
            print_out("OTA Update Failed");
            Update.printError(Serial);
            otaError = true;
        }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED)
    {
        Update.end();
        print_out("OTA Update Aborted");
        otaError = true;
        _updating = false;
        otaActive = false;
    }
}

String OTAHandler::getCurrentVersion()
{
#ifdef BUILD_VERSION
    return String(BUILD_VERSION);
#else
    return String("v") + String(INTERNAL_VERSION);
#endif
}

String OTAHandler::getCurrentBuildDate()
{
    return String(__DATE__) + " " + String(__TIME__);
}

void OTAHandler::handleOTAComplete()
{
    bool hasError = Update.hasError() || otaError;
    _server->send(200, MIME_TYPE_TEXT, hasError ? "Update Failed" : "Update Success! Rebooting...");

    if (!hasError && otaComplete)
        rebootWithDelay();
}

void OTAHandler::handleCheckVersion()
{
    if (!_server)
        return;

    JsonDocument doc;
    doc["currentVersion"] = getCurrentVersion();
    doc["buildDate"] = getCurrentBuildDate();

    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.begin(GITHUB_API_URL);
    http.addHeader("User-Agent", "OG-Star-Tracker");

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();
        http.end();

        String latestVersion = extractJsonValue(payload, "tag_name");
        if (latestVersion.length() > 0)
        {
            doc["latestVersion"] = latestVersion;
            doc["releaseUrl"] = extractJsonValue(payload, "html_url");

            // Find .bin download URL
            int start = payload.indexOf("\"browser_download_url\":\"");
            while (start != -1)
            {
                start += 24;
                int end = payload.indexOf("\"", start);
                String url = payload.substring(start, end);
                if (url.endsWith(".bin"))
                {
                    doc["downloadUrl"] = url;
                    break;
                }
                start = payload.indexOf("\"browser_download_url\":\"", end);
            }

            // Parse and clean release notes
            String notes = extractJsonValue(payload, "body");
            notes.replace("\\r\\n", "|");
            notes.replace("\\n", "|");
            notes.replace("\\r", "");
            notes.replace("\\\"", "'");
            notes.replace("\\t", "  ");
            doc["releaseNotes"] = sanitizeString(notes);

            print_out("GitHub: %s (current: %s)", latestVersion.c_str(),
                      getCurrentVersion().c_str());
        }
        else
        {
            doc["error"] = "Failed to parse GitHub response";
        }
    }
    else
    {
        http.end();
        doc["error"] = httpCode > 0 ? "GitHub API request failed" : "No internet connection";
        print_out("GitHub API failed: %d", httpCode);
    }

    String response;
    serializeJson(doc, response);
    _server->send(200, MIME_APPLICATION_JSON, response);
}

void OTAHandler::handleDownloadUpdate()
{
    if (!_server->hasArg("url"))
    {
        _server->send(400, MIME_TYPE_TEXT, "Missing URL parameter");
        return;
    }

    String firmwareUrl = _server->arg("url");
    _server->send(200, MIME_TYPE_TEXT, "Starting download...");
    resetOTAState();

    HTTPClient http;
    http.setTimeout(DOWNLOAD_TIMEOUT_MS);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(firmwareUrl);
    http.addHeader("User-Agent", "OG-Star-Tracker");

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
    {
        int contentLength = http.getSize();
        if (contentLength <= 0)
        {
            print_out("OTA init failed: Invalid size");
            otaError = true;
            http.end();
            return;
        }

        if (!Update.begin(contentLength))
        {
            print_out("OTA init failed: No space");
            Update.printError(Serial);
            otaError = true;
            http.end();
            return;
        }

        print_out("Downloading %d KB...", contentLength / 1024);
        otaActive = true;
        otaTotalBytes = contentLength;

        WiFiClient* stream = http.getStreamPtr();
        uint8_t buff[DOWNLOAD_BUFFER_SIZE];
        size_t written = 0;
        size_t lastLoggedKB = 0;

        print_out("Starting firmware download...");

        while (http.connected() && (written < contentLength))
        {
            // Allow web server to handle status requests during download
            _server->handleClient();

            size_t available = stream->available();

            if (available)
            {
                int bytesRead = stream->readBytes(
                    buff, ((available > sizeof(buff)) ? sizeof(buff) : available));

                size_t bytesWritten = Update.write(buff, bytesRead);
                if (bytesWritten != bytesRead)
                {
                    print_out("Write error: wrote %d of %d bytes", bytesWritten, bytesRead);
                    otaError = true;
                    otaActive = false;
                    break;
                }

                written += bytesWritten;
                otaBytesWritten = written;

                // Log progress every LOG_INTERVAL_KB
                size_t currentKB = written / 1024;
                if (currentKB >= lastLoggedKB + LOG_INTERVAL_KB)
                {
                    print_out("%d KB (%d%%)", currentKB, (written * 100) / contentLength);
                    lastLoggedKB = currentKB;
                }
            }
            vTaskDelay(100);
        }

        print_out("Download complete: %d bytes", written);

        if (written == contentLength && Update.end(true))
        {
            otaComplete = true;
            otaActive = false;
            rebootWithDelay();
        }
        else
        {
            otaActive = false;
            otaError = true;
            print_out(written != contentLength ? "Download incomplete: %d of %d bytes"
                                               : "OTA finalization failed",
                      written, contentLength);
            Update.end(false);
            if (written == contentLength)
                Update.printError(Serial);
        }
    }
    else
    {
        otaError = true;
        print_out("HTTP download failed, code: %d", httpCode);
    }

    http.end();
}

void OTAHandler::handleOTAStatus()
{
    JsonDocument doc;
    doc["active"] = otaActive;
    doc["complete"] = otaComplete;
    doc["error"] = otaError;
    doc["bytesWritten"] = (unsigned long) otaBytesWritten;
    doc["totalBytes"] = (unsigned long) otaTotalBytes;
    doc["percent"] = otaTotalBytes > 0 ? (otaBytesWritten * 100) / otaTotalBytes : 0;

    String response;
    serializeJson(doc, response);
    _server->send(200, MIME_APPLICATION_JSON, response);
}
