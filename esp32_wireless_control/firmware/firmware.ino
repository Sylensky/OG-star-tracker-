#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <ErriezSerialTerminal.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <string.h>

#include "axis.h"
#include "catalogues/star_database.h"
#include "commands.h"
#include "common_strings.h"
#include "configs/config.h"
#include "eeprom_manager.h"
#include "functions/intervalometer/intervalometer.h"
#include "hardwaretimer.h"
#include "tracking_rates.h"
#include "uart.h"
#include "website/web_languages.h"
#include "website/website_strings.h"

SerialTerminal term(CLI_NEWLINE_CHAR, CLI_DELIMITER_CHAR);
WebServer server(WEBSERVER_PORT);
Languages language = EN;
StarDatabase* starDatabase = nullptr;
Intervalometer* intervalometer = nullptr;

void uartTask(void* pvParameters);
void consoleTask(void* pvParameters);
void webserverTask(void* pvParameters);
void intervalometerTask(void* pvParameters);

extern const uint8_t _interface_index_html_start[] asm("_binary_interface_index_html_start");
extern const uint8_t _interface_index_html_end[] asm("_binary_interface_index_html_end");

extern const uint8_t _catalogues_ngc_converted_ngc2000_bin_start[] asm(
    "_binary_catalogues_ngc_converted_ngc2000_bin_start");
extern const uint8_t _catalogues_ngc_converted_ngc2000_bin_end[] asm(
    "_binary_catalogues_ngc_converted_ngc2000_bin_end");

extern const uint8_t _catalogues_ngc_converted_ngc2000_compact_bin_start[] asm(
    "_binary_catalogues_ngc_converted_ngc2000_compact_bin_start");
extern const uint8_t _catalogues_ngc_converted_ngc2000_compact_bin_end[] asm(
    "_binary_catalogues_ngc_converted_ngc2000_compact_bin_end");

extern const uint8_t _catalogues_bsc5_converted_bsc5ra_bin_start[] asm(
    "_binary_catalogues_bsc5_converted_bsc5ra_bin_start");
extern const uint8_t _catalogues_bsc5_converted_bsc5ra_bin_end[] asm(
    "_binary_catalogues_bsc5_converted_bsc5ra_bin_end");

extern const uint8_t _catalogues_bsc5_converted_bsc5ra_compact_bin_start[] asm(
    "_binary_catalogues_bsc5_converted_bsc5ra_compact_bin_start");
extern const uint8_t _catalogues_bsc5_converted_bsc5ra_compact_bin_end[] asm(
    "_binary_catalogues_bsc5_converted_bsc5ra_compact_bin_end");

StarDatabase* handleStarDatabase(StarDatabaseType type)
{
    StarDatabase* db = nullptr;
    const uint8_t* bin_start = nullptr;
    const uint8_t* bin_end = nullptr;
    size_t len = 0;

    if (starDatabase != nullptr && starDatabase->getDatabaseType() == type)
        return starDatabase; // Return existing instance if already loaded

    if (starDatabase != nullptr)
    {
        starDatabase->unloadDatabase();
        delete starDatabase;
        starDatabase = nullptr;
    }

    switch (type)
    {
        case DB_NGC2000:
            bin_start = _catalogues_ngc_converted_ngc2000_bin_start;
            bin_end = _catalogues_ngc_converted_ngc2000_bin_end;
            len = bin_end - bin_start;
            break;
        case DB_NGC2000_COMPACT:
            bin_start = _catalogues_ngc_converted_ngc2000_compact_bin_start;
            bin_end = _catalogues_ngc_converted_ngc2000_compact_bin_end;
            len = bin_end - bin_start;
            break;
        case DB_BSC5:
            bin_start = _catalogues_bsc5_converted_bsc5ra_bin_start;
            bin_end = _catalogues_bsc5_converted_bsc5ra_bin_end;
            len = bin_end - bin_start;
            break;
        case DB_BSC5_COMPACT:
            bin_start = _catalogues_bsc5_converted_bsc5ra_compact_bin_start;
            bin_end = _catalogues_bsc5_converted_bsc5ra_compact_bin_end;
            len = bin_end - bin_start;
            break;
        default:
            print_out("Error: Unsupported database type %d", type);
            return nullptr;
    }

    db = new StarDatabase(type, bin_start, bin_end);
    if (!db->loadDatabase((const char*) bin_start, len))
    {
        delete db;
        return nullptr;
    }

    return db;
}

String getChipID()
{
    uint64_t chipid = ESP.getEfuseMac();
    uint8_t baseMac[6];

    // Extract each byte of the MAC address from the chipid
    baseMac[0] = (chipid >> 40) & 0xFF;
    baseMac[1] = (chipid >> 32) & 0xFF;
    baseMac[2] = (chipid >> 24) & 0xFF;
    baseMac[3] = (chipid >> 16) & 0xFF;
    baseMac[4] = (chipid >> 8) & 0xFF;
    baseMac[5] = chipid & 0xFF;

    // Create a String object to hold the formatted ID
    String id = String(baseMac[4], HEX) + String(baseMac[5], HEX);
    return id;
}

// Handle requests to the root URL ("/")
void handleRoot()
{
#if DEBUG == 1
    print_out("HTTP Request: GET / (serving main page)");
    print_out("  Client IP: %s", server.client().remoteIP().toString().c_str());
    print_out("  User-Agent: %s", server.header("User-Agent").c_str());
#endif
    size_t htmlSize = _interface_index_html_end - _interface_index_html_start;

    // Work with raw bytes and avoid String for null-byte handling
    // Allocate buffer for HTML with extra space for replacements
    char* htmlBuffer = (char*) pvPortMalloc(htmlSize + 10000);
    if (!htmlBuffer)
    {
        server.send(500, MIME_TYPE_TEXT, "Internal Server Error: Out of memory");
        return;
    }

    memcpy(htmlBuffer, _interface_index_html_start, htmlSize);
    size_t currentSize = htmlSize;

    for (int placeholder = 0; placeholder < numberOfHTMLStrings; placeholder++)
    {
        const char* searchStr = HTMLplaceHolders[placeholder];
        const char* replaceStr = languageHTMLStrings[language][placeholder];

        size_t searchLen = strlen(searchStr);
        size_t replaceLen = strlen(replaceStr);

        char* pos = htmlBuffer;
        while ((pos = strstr(pos, searchStr)) != nullptr)
        {
            if (replaceLen > searchLen)
            {
                memmove(pos + replaceLen, pos + searchLen,
                        currentSize - (pos - htmlBuffer) - searchLen);
            }
            memcpy(pos, replaceStr, replaceLen);
            if (replaceLen < searchLen)
            {
                memmove(pos + replaceLen, pos + searchLen,
                        currentSize - (pos - htmlBuffer) - searchLen);
            }
            currentSize += replaceLen - searchLen;
            pos += replaceLen;
        }
    }

    char buffer[100];
    String selectString = "";
    for (int lang = 0; lang < LANG_COUNT; lang++)
    {
        if (lang == language)
        {
            snprintf(buffer, sizeof(buffer), "<option value=\"%u\" selected>%s</option>\n", lang,
                     languageNames[language][lang]);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "<option value=\"%u\">%s</option>\n", lang,
                     languageNames[language][lang]);
        }
        selectString.concat(buffer);
    }

    char* langPos = strstr(htmlBuffer, "%LANG_SELECT%");
    if (langPos)
    {
        const char* langReplacement = selectString.c_str();
        size_t langSearchLen = 13; // strlen("%LANG_SELECT%")
        size_t langReplaceLen = selectString.length();

        if (langReplaceLen > langSearchLen)
        {
            memmove(langPos + langReplaceLen, langPos + langSearchLen,
                    currentSize - (langPos - htmlBuffer) - langSearchLen);
        }
        memcpy(langPos, langReplacement, langReplaceLen);
        if (langReplaceLen < langSearchLen)
        {
            memmove(langPos + langReplaceLen, langPos + langSearchLen,
                    currentSize - (langPos - htmlBuffer) - langSearchLen);
        }
        currentSize += langReplaceLen - langSearchLen;
    }

    server.send_P(200, MIME_TYPE_HTML, htmlBuffer, currentSize);
    vPortFree(htmlBuffer);

#if DEBUG == 1
    print_out("  Raw HTML pointers: start=%p, end=%p, calculated size=%d",
              _interface_index_html_start, _interface_index_html_end, currentSize);
    print_out("  Final HTML size: %d bytes", currentSize);
    print_out("  Language: %d, Placeholders replaced: %d", language, numberOfHTMLStrings);
    print_out("  Response sent successfully");
#endif
}

void handleOn()
{
#if DEBUG == 1
    logRequest("/on");
#endif
    print_out("Handling tracking ON request with custom rate: %d",
              server.arg(TRACKING_SPEED).toInt());

    uint64_t custom_rate = server.arg(TRACKING_SPEED).toInt();
    int direction = server.arg(DIRECTION).toInt();

    trackingRates.setCustomRate(custom_rate);
#if DEBUG == 1
    print_out("  Direction: %d, Final rate: %llu", direction, trackingRates.getRate());
#endif
    ra_axis.startTracking(trackingRates.getRate(), direction);

    if (intervalometer->getErrorMessage() == ErrorMessage::ERR_MSG_NONE)
        server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_TRACKING_ON]);
    else
        server.send(200, MIME_TYPE_TEXT,
                    languageErrorMessageStrings[language][intervalometer->getErrorMessage()]);
#if DEBUG == 1
    print_out("  Tracking ON response sent");
#endif
}

void handleOff()
{
    ra_axis.stopTracking();
    server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_TRACKING_OFF]);
}

void handleSlewRequest()
{
    if (!ra_axis.slewActive)
    { // if slew is not active - needed for ipad (passes multiple touchon events)
        int slew_speed = server.arg(SPEED).toInt();
        int direction = server.arg(DIRECTION).toInt();
        // limit custom slew speed to 2-400
        slew_speed = slew_speed > MAX_CUSTOM_SLEW_RATE   ? MAX_CUSTOM_SLEW_RATE
                     : slew_speed < MIN_CUSTOM_SLEW_RATE ? MIN_CUSTOM_SLEW_RATE
                                                         : slew_speed;
        ra_axis.startSlew((2 * ra_axis.rate.tracking) / slew_speed, direction);
        server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_SLEWING]);
    }
}

void handleSlewOff()
{
    if (ra_axis.slewActive)
    { // if slew is active needed for ipad (passes multiple touchoff events)
        ra_axis.stopSlew();
    }
    server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_SLEW_CANCELLED]);
}

void handleSetLanguage()
{
    int lang = server.arg("lang").toInt();
    language = static_cast<Languages>(lang);
    EepromManager::writeObject(LANG_EEPROM_ADDR, language);
    server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_OK]);
}

void handleSetCurrent()
{
    if (!intervalometer->isActive())
    {
        // Reset the current error message
        intervalometer->setErrorMessage(ERR_MSG_NONE);

        Intervalometer::Settings settings = intervalometer->getSettings();

        int modeInt = server.arg(CAPTURE_MODE).toInt();
        if (modeInt < 0 || modeInt >= static_cast<int>(Intervalometer::Mode::MaxModes))
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_CAPTURE_MODE);
            return;
        }
        intervalometer->setMode(static_cast<Intervalometer::Mode>(modeInt));

        int exposureTime = server.arg(EXPOSURE_TIME).toInt();
        if (exposureTime <= 0)
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_EXPOSURE_LENGTH);
            return;
        }
        settings.exposureTime = exposureTime;

        int exposures = server.arg(EXPOSURES).toInt();
        if (exposures <= 0)
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_EXPOSURE_AMOUNT);
            return;
        }
        settings.exposures = exposures;

        int preDelay = server.arg(PREDELAY).toInt();
        if (preDelay < 0)
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_PREDELAY_TIME);
            return;
        }
        else if (preDelay == 0)
            preDelay = 5;
        settings.preDelay = preDelay;

        int delayTime = server.arg(DELAY).toInt();
        if (delayTime < 0)
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_DELAY_TIME);
            return;
        }
        settings.delayTime = delayTime;

        int frames = server.arg(FRAMES).toInt();
        if (frames <= 0)
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_FRAME_AMOUNT);
            return;
        }
        settings.frames = frames;

        float panAngle = server.arg(PAN_ANGLE).toFloat() / 100;
        if (panAngle < 0.0)
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_PAN_ANGLE);
            return;
        }
        settings.panAngle = panAngle;

        int panDirection = server.arg(PAN_DIRECTION).toInt();
        if (panDirection < 0 || panDirection > 1)
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_PAN_DIRECTION);
            return;
        }
        settings.panDirection = panDirection;

        int continuousPan = server.arg(CONTINUOUS_PAN).toInt();
        settings.continuousPan = continuousPan;

        int enableTracking = server.arg(ENABLE_TRACKING).toInt();
        if (enableTracking < 0 || enableTracking > 1)
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_ENABLE_TRACKING_VALUE);
            return;
        }
        settings.enableTracking = enableTracking;

        int dither = server.arg(DITHER_CHOICE).toInt();
        if (dither < 0 || dither > 1)
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_DITHER_CHOICE);
            return;
        }
        settings.dither = dither;

        int ditherFrequency = server.arg(DITHER_FREQUENCY).toInt();
        if (ditherFrequency <= 0)
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_DITHER_FREQUENCY);
            return;
        }
        settings.ditherFrequency = ditherFrequency;

        int focalLength = server.arg(FOCAL_LENGTH).toInt();
        if (focalLength <= 0)
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_FOCAL_LENGTH);
            return;
        }
        settings.focalLength = focalLength;

        float pixelSize = server.arg(PIXEL_SIZE).toFloat() / 100;
        if (pixelSize <= 0.0)
        {
            intervalometer->setErrorMessage(ERR_MSG_INVALID_PIXEL_SIZE);
            return;
        }
        settings.pixelSize = pixelSize;
        settings.mode = static_cast<uint8_t>(intervalometer->getMode());
        intervalometer->setSettings(settings);

        String currentMode = server.arg(MODE);
        if (currentMode == "save")
        {
            int preset = server.arg(PRESET).toInt();
            intervalometer->saveSettingsToPreset(preset);
            server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_SAVED_PRESET]);
        }
        else if (currentMode == "start")
        {
            Intervalometer::Mode mode = intervalometer->getMode();
            if ((mode == Intervalometer::Mode::LongExposureMovie ||
                 mode == Intervalometer::Mode::LongExposureStill) &&
                !ra_axis.trackingActive)
            {
                server.send(200, MIME_TYPE_TEXT,
                            languageMessageStrings[language][MSG_TRACKING_NOT_ACTIVE]);
            }
            else
            {
                intervalometer->startCapture();
                server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_CAPTURE_ON]);
            }
        }
    }
    else
    {
        server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_CAPTURE_ALREADY_ON]);
    }
}

#if DEBUG == 1
void logRequest(const char* endpoint)
{
    print_out("HTTP Request: %s %s", server.method() == HTTP_GET ? "GET" : "POST", endpoint);
    print_out("  Client: %s", server.client().remoteIP().toString().c_str());
    if (server.args() > 0)
    {
        print_out("  Arguments (%d):", server.args());
        for (int i = 0; i < server.args(); i++)
        {
            print_out("    %s = %s", server.argName(i).c_str(), server.arg(i).c_str());
        }
    }
}

void handleNotFound()
{
    print_out("HTTP 404: %s %s", server.method() == HTTP_GET ? "GET" : "POST",
              server.uri().c_str());
    print_out("  Client: %s", server.client().remoteIP().toString().c_str());
    print_out("  User-Agent: %s", server.header("User-Agent").c_str());
    server.send(404, MIME_TYPE_TEXT, "Not Found");
}
#endif

Position calculatePosition(String raArg, String decArg)
{
    Position position;
    position.ra_arcseconds = raArg.toInt();
    position.dec_arcseconds = decArg.toInt();

    if (position.ra_arcseconds == -1)
    {
        position.ra_arcseconds = 0;
#if DEBUG == 1
        print_out("Invalid RA position input. Defaulting to 0.");
#endif
    }

    if (position.dec_arcseconds == -1)
    {
        position.dec_arcseconds = 0;
#if DEBUG == 1
        print_out("Invalid DEC position input. Defaulting to 0.");
#endif
    }

    return position;
}

void handleGotoRA()
{
    Position currentPosition = calculatePosition(server.arg("currentRA"), server.arg("currentDEC"));
    Position targetPosition = calculatePosition(server.arg("targetRA"), server.arg("targetDEC"));
    int pan_speed = server.arg(SPEED).toInt();

    pan_speed = pan_speed > MAX_CUSTOM_SLEW_RATE   ? MAX_CUSTOM_SLEW_RATE
                : pan_speed < MIN_CUSTOM_SLEW_RATE ? MIN_CUSTOM_SLEW_RATE
                                                   : pan_speed;

    print_out("GotoRA called with:");
    print_out("  Current RA: %lld arcseconds, DEC: %lld arcseconds", currentPosition.ra_arcseconds,
              currentPosition.dec_arcseconds);
    print_out("  Target RA: %lld arcseconds, DEC: %lld arcseconds", targetPosition.ra_arcseconds,
              targetPosition.dec_arcseconds);
    print_out("  rate: %lld", (int) ((2 * ra_axis.rate.tracking) / pan_speed));

    ra_axis.gotoTarget(TRACKER_MOTOR_MICROSTEPPING / 2, (2 * ra_axis.rate.tracking) / pan_speed,
                       currentPosition, targetPosition);
    server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_GOTO_RA_PANNING_ON]);
}

void handleSetPosition()
{
    Position currentPosition = calculatePosition(server.arg("currentRA"), server.arg("currentDEC"));
    int64_t stepPosition = currentPosition.ra_arcseconds * trackingRates.getStepsPerSecondSolar();

    ra_axis.setPosition(stepPosition);
    server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_POSITION_SET_SUCCESS]);
}

void handleGetPresetExposureSettings()
{
    int preset = server.arg(PRESET).toInt();
    intervalometer->readSettingsFromPreset(preset);
    ArduinoJson::JsonDocument settings;
    String json;
    const Intervalometer::Settings& s = intervalometer->getSettings();
    settings[MODE] = static_cast<uint8_t>(intervalometer->getMode());
    settings[EXPOSURES] = s.exposures;
    settings[DELAY] = s.delayTime;
    settings[PREDELAY] = s.preDelay;
    settings[EXPOSURE_TIME] = s.exposureTime;
    settings[PAN_ANGLE] = s.panAngle * 100;
    settings[PAN_DIRECTION] = s.panDirection;
    settings[CONTINUOUS_PAN] = s.continuousPan;
    settings[DITHER_CHOICE] = s.dither;
    settings[DITHER_FREQUENCY] = s.ditherFrequency;
    settings[ENABLE_TRACKING] = s.enableTracking;
    settings[FRAMES] = s.frames;
    settings[PIXEL_SIZE] = s.pixelSize * 100;
    settings[FOCAL_LENGTH] = s.focalLength;
    serializeJson(settings, json);
    // print_out(json);
    server.send(200, MIME_APPLICATION_JSON, json);
}

void handleAbortCapture()
{
    if (intervalometer->isActive())
    {
        intervalometer->abortCapture();
        server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_CAPTURE_OFF]);
    }
    else
    {
        server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_CAPTURE_ALREADY_OFF]);
    }
}

void handleAbortGoToRA()
{
    if (ra_axis.slewActive)
    {
        ra_axis.stopGotoTarget();
        server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_GOTO_RA_PANNING_OFF]);
    }
}

void handleStatusRequest()
{
    if (intervalometer->isActive())
    {
        // Build status string with progress info
        String statusMsg;
        uint16_t currentExp = intervalometer->getExposuresTaken();
        uint16_t totalExp = intervalometer->getSettings().exposures;

        switch (intervalometer->getState())
        {
            case Intervalometer::State::PreDelay:
                statusMsg = languageMessageStrings[language][MSG_CAP_PREDELAY];
                break;
            case Intervalometer::State::Capture:
                statusMsg = languageMessageStrings[language][MSG_CAP_EXPOSING];
                statusMsg += " (" + String(currentExp + 1) + "/" + String(totalExp) + ")";
                break;
            case Intervalometer::State::Dither:
                statusMsg = languageMessageStrings[language][MSG_CAP_DITHER];
                statusMsg += " (" + String(currentExp) + "/" + String(totalExp) + ")";
                break;
            case Intervalometer::State::Pan:
                statusMsg = languageMessageStrings[language][MSG_CAP_PANNING];
                statusMsg += " (" + String(currentExp) + "/" + String(totalExp) + ")";
                break;
            case Intervalometer::State::Delay:
                statusMsg = languageMessageStrings[language][MSG_CAP_DELAY];
                statusMsg += " (" + String(currentExp) + "/" + String(totalExp) + ")";
                break;
            case Intervalometer::State::Rewind:
                statusMsg = languageMessageStrings[language][MSG_CAP_REWIND];
                break;
            case Intervalometer::State::Inactive:
            case Intervalometer::State::Complete:
            default:
                break;
        }

        if (statusMsg.length() > 0)
        {
            server.send(200, MIME_TYPE_TEXT, statusMsg);
        }
    }
    else if (ra_axis.slewActive && !ra_axis.goToTarget)
    {
        server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_SLEWING]);
    }
    else if (ra_axis.slewActive && ra_axis.goToTarget)
    {
        server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_GOTO_RA_PANNING_ON]);
    }
    else if (ra_axis.trackingActive)
    {
        server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_TRACKING_ON]);
    }
    else
    {
        if (intervalometer->getErrorMessage() == ErrorMessage::ERR_MSG_NONE)
            server.send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_IDLE]);
        else
            server.send(200, MIME_TYPE_TEXT,
                        languageErrorMessageStrings[language][intervalometer->getErrorMessage()]);
    }

    if (ra_axis.slewActive)
    {
        slewTimeOut.setCountValue(0); // reset timeout counter
    }

    server.send(204, MIME_TYPE_TEXT, "dummy");
}

void handleVersion()
{
    server.send(200, MIME_TYPE_TEXT, (String) INTERNAL_VERSION);
}

void handleGetTrackingRates()
{
#if DEBUG == 1
    print_out("HTTP Request: GET /getTrackingRates");
    print_out("  Client IP: %s", server.client().remoteIP().toString().c_str());
#endif

    int rateType = server.arg("type").toInt(); // 0=current, 1=sidereal, 2=solar, 3=lunar
    print_out("Received tracking rate request with type: %d", rateType);
    uint64_t rate = 0;

    switch (rateType)
    {
        case 0:
            rate = trackingRates.getRate(); // current rate
            break;
        case 1:
            rate = trackingRates.getSiderealRate();
            break;
        case 2:
            rate = trackingRates.getSolarRate();
            break;
        case 3:
            rate = trackingRates.getLunarRate();
            break;
        default:
            rate = trackingRates.getRate(); // default to current
            break;
    }

    server.send(200, MIME_TYPE_TEXT, String(rate));
#if DEBUG == 1
    print_out("  Response sent: %s", String(rate).c_str());
#endif
}

void handleGetCurrentPosition()
{
    String utcTimeStr = server.arg("utcTime");
    String timezoneStr = server.arg("timezone");
    float longitude = server.arg("longitude").toFloat();
    int64_t currentStepPosition = ra_axis.getPosition();

    const int64_t STEPS_PER_FULL_REV = STEPS_PER_TRACKER_FULL_REV_INT;
    const int64_t RA_SECONDS_PER_FULL_REV = (SOLAR_DAY_MS / 1000);

    // Calculate current RA position in seconds (0-86399)
    // Normalize position to handle negative values and multiple revolutions
    int64_t normalizedSteps =
        ((currentStepPosition % STEPS_PER_FULL_REV) + STEPS_PER_FULL_REV) % STEPS_PER_FULL_REV;
    long raSeconds = (long) ((normalizedSteps * RA_SECONDS_PER_FULL_REV) / STEPS_PER_FULL_REV);

    String response = "{\"ra\":" + String(raSeconds) + ",\"utcTime\":\"" + utcTimeStr + "\"" +
                      ",\"longitude\":" + String(longitude) + "}";

    server.send(200, MIME_APPLICATION_JSON, response);
}

void handleSaveTrackingRatePreset()
{
    int preset = server.arg(PRESET).toInt();
    uint8_t trackingType = server.arg(TRACKING_TYPE).toInt();
    uint64_t customRate = server.arg(CUSTOM_RATE).toInt();

    // Save directly to tracking rate preset system
    trackingRates.saveTrackingRatePreset(preset, trackingType, customRate);

    server.send(200, MIME_TYPE_TEXT, "Tracking rate preset saved");
}

void handleLoadTrackingRatePreset()
{
    int preset = server.arg(PRESET).toInt();

    if (preset < 5)
    {
        ArduinoJson::JsonDocument response;
        response["trackingRateType"] = trackingRates.trackingRatePresets[preset].trackingRateType;
        response["customTrackingRate"] =
            (long long) trackingRates.trackingRatePresets[preset].customTrackingRate;

        trackingRates.loadTrackingRatePreset(preset);

        String json;
        serializeJson(response, json);
        server.send(200, MIME_APPLICATION_JSON, json);
    }
    else
        server.send(400, MIME_TYPE_TEXT, "Invalid preset number");
}

void handleCatalogSearch()
{
    StarDatabaseType catalogType = (StarDatabaseType) server.arg(STAR_CATALOG).toInt();
    String objectName = server.arg(STAR_NAME);

    starDatabase = handleStarDatabase(catalogType);

#if DEBUG == 1
    print_out("Received catalog=%d, name=%s", catalogType, objectName.c_str());
#endif

    if (objectName.length() == 0)
    {
        server.send(400, MIME_TYPE_TEXT, "Object name required");
        return;
    }

    StarUnifiedEntry foundObject;
    bool found = starDatabase->findByName(objectName.c_str(), foundObject);

    if (found)
    {
        ArduinoJson::JsonDocument objectData;
        String json;
        objectData["name"] = foundObject.name;
        objectData["ra"] = (long long) (foundObject.ra_hours * 3600.0);
        objectData["dec"] = (long long) (foundObject.dec_deg * 3600.0);
        objectData["type"] = foundObject.type_str;
        objectData["magnitude"] = foundObject.magnitude;
        objectData["constellation"] = foundObject.constellation;
        serializeJson(objectData, json);

#if DEBUG == 1
        print_out("Found object: %s at RA=%.2fh, Dec=%.2f°", foundObject.name.c_str(),
                  foundObject.ra_hours, foundObject.dec_deg);
#endif
        server.send(200, MIME_APPLICATION_JSON, json);
    }
    else
    {
#if DEBUG == 1
        print_out("Object not found: %s", objectName.c_str());
#endif
        server.send(404, "text/plain", "Object not found");
    }
}

void setupWireless()
{
#if AP_MODE == 1
    // Create unique SSID for each device
    String chipID = getChipID();
    char ssid[32];
    sprintf(ssid, "%s#%s", WIFI_SSID, chipID.c_str());

    WiFi.mode(WIFI_MODE_AP);
    print_out("Creating Wifi Network: %s", ssid);

    // Configure AP with specific IP settings
    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);

    if (!WiFi.softAPConfig(local_IP, gateway, subnet))
    {
        print_out("Failed to configure AP IP settings");
    }

    if (!WiFi.softAP(ssid, WIFI_PASSWORD))
    {
        print_out("Failed to start AP");
    }

    vTaskDelay(1000); // Give more time for AP to initialize

    // ANDROID 10 WORKAROUND==================================================
    // set new WiFi configurations
    print_out("Applying Android 10 compatibility workaround...");
    WiFi.disconnect();
    /*Stop wifi to change config parameters*/
    esp_wifi_stop();
    esp_wifi_deinit();
    /*Disabling AMPDU RX is necessary for Android 10 support*/
    wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT(); // We use the default config ...
    my_config.ampdu_rx_enable = 0;                             //... and modify only what we want.
    print_out("WiFi: Disabled AMPDU...");
    esp_wifi_init(&my_config); // set the new config = "Disable AMPDU"
    esp_wifi_start();          // Restart WiFi

    // Restart AP after workaround
    WiFi.mode(WIFI_MODE_AP);
    if (!WiFi.softAPConfig(local_IP, gateway, subnet))
    {
        print_out("Failed to reconfigure AP IP settings after workaround");
    }
    if (!WiFi.softAP(ssid, WIFI_PASSWORD))
    {
        print_out("Failed to restart AP after workaround");
    }

    vTaskDelay(1000);
    // ANDROID 10 WORKAROUND==================================================

    print_out("AP setup complete. Checking status...");
    print_out("AP Status: %s", WiFi.softAPgetStationNum() >= 0 ? "Active" : "Inactive");
    print_out("WiFi Mode: %d", WiFi.getMode());
#else
    WiFi.mode(WIFI_MODE_STA); // Set ESP32 in station mode
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    print_out("Connecting to SSID: %s with password: %s", WIFI_SSID, WIFI_PASSWORD);
    print_out("Connecting to Network in STA mode");
    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(1000);
        print_out(".");
    }
#endif

    server.on("/", HTTP_GET, handleRoot);
    server.on("/on", HTTP_GET, handleOn);
    server.on("/off", HTTP_GET, handleOff);
    server.on("/startslew", HTTP_GET, handleSlewRequest);
    server.on("/stopslew", HTTP_GET, handleSlewOff);
    server.on("/setCurrent", HTTP_GET, handleSetCurrent);
    server.on("/readPreset", HTTP_GET, handleGetPresetExposureSettings);
    server.on("/abort", HTTP_GET, handleAbortCapture);
    server.on("/status", HTTP_GET, handleStatusRequest);
    server.on("/gotoRA", HTTP_GET, handleGotoRA);
    server.on("/setPosition", HTTP_GET, handleSetPosition);
    server.on("/getCurrentPosition", HTTP_GET, handleGetCurrentPosition);
    server.on("/abort-goto-ra", HTTP_GET, handleAbortGoToRA);
    server.on("/version", HTTP_GET, handleVersion);
    server.on("/getTrackingRates", HTTP_GET, handleGetTrackingRates);
    server.on("/saveTrackingRatePreset", HTTP_GET, handleSaveTrackingRatePreset);
    server.on("/loadTrackingRatePreset", HTTP_GET, handleLoadTrackingRatePreset);
    server.on("/setlang", HTTP_GET, handleSetLanguage);
    server.on("/starSearch", HTTP_GET, handleCatalogSearch);

#if DEBUG == 1
    server.onNotFound(handleNotFound);
    print_out("Web server debug logging enabled");
#endif

    // Start the server
    server.begin();

#if AP_MODE == 1
    IPAddress apIP = WiFi.softAPIP();
    print_out("AP IP address: %s", apIP.toString().c_str());
    if (apIP == IPAddress(0, 0, 0, 0))
    {
        print_out("ERROR: AP IP is 0.0.0.0 - AP may not be configured correctly");
        print_out("WiFi Mode: %d", WiFi.getMode());
        print_out("AP Status: %d", WiFi.status());
    }
#else
    print_out("STA IP address: %s", WiFi.localIP().toString().c_str());
#endif

    print_out("Starting mDNS responder");
    if (!MDNS.begin(MDNS_NAME))
    {
        print_out("Error starting mDNS responder");
        return;
    }
    print_out("mDNS responder started");

    MDNS.addService("http", "tcp", WEBSERVER_PORT);
    MDNS.addServiceTxt("http", "tcp", "ogtracker", "1");
    MDNS.addServiceTxt("http", "tcp", "version", BUILD_VERSION);

    MDNS.addService("ogtracker", "tcp", WEBSERVER_PORT);
    MDNS.addServiceTxt("ogtracker", "tcp", "version", BUILD_VERSION);
}

void setup()
{
    // Start the debug serial connection
    setup_uart(&Serial, 115200);

    // start UART task before any usage of print_out
    if (xTaskCreatePinnedToCore(uartTask, "uart", 4096, NULL, 1, NULL, 1))
    {
        print_out_tbl(TSK_CLEAR_SCREEN);
        print_out_tbl(TSK_START_UART);
    }

    print_out_tbl(HEAD_LINE);
    print_out_tbl(HEAD_LINE_TRACKER);
    print_out("***         Running on %d MHz         ***", getCpuFrequencyMhz());
    print_out("***     Dual Core Setup: ISR Core 0    ***");
    print_out("***     Application Tasks on Core 1    ***");
    print_out_tbl(HEAD_LINE_VERSION);

    // Initialize EEPROM manager
    EepromManager::begin(512); // SIZE = 5 x presets = 5 x 32 bytes = 160 bytes
    uint8_t langNum = 0;
    EepromManager::readObject(LANG_EEPROM_ADDR, langNum);

    if (langNum >= LANG_COUNT)
        language = static_cast<Languages>(0);
    else
        language = static_cast<Languages>(langNum);

    // Initialize the pins
    pinMode(INTERV_PIN, OUTPUT);
    pinMode(STATUS_LED, OUTPUT);
    pinMode(AXIS1_STEP, OUTPUT);
    pinMode(AXIS1_DIR, OUTPUT);
    pinMode(EN12_n, OUTPUT);
    digitalWrite(AXIS1_STEP, LOW);
    digitalWrite(EN12_n, LOW);
    // handleExposureSettings();

    // Initialize Wifi and web server
    setupWireless();

    // Initialize the console serial
    setup_terminal(&term);

    if (xTaskCreatePinnedToCore(consoleTask, "console", 4096, NULL, 1, NULL, 1))
        print_out_tbl(TSK_START_CONSOLE);

    if (xTaskCreatePinnedToCore(intervalometerTask, "intervalometer", 4096, NULL, 1, NULL, 1))
        print_out_tbl(TSK_START_INTERVALOMETER);
    // Increase webserver task stack size to handle large HTML responses and concurrent connections
    if (xTaskCreatePinnedToCore(webserverTask, "webserver", 8192, NULL, 1, NULL, 1))
        print_out_tbl(TSK_START_WEBSERVER);

    // Give tasks time to fully initialize before starting axis
    vTaskDelay(100);
    print_out("Initializing axis with TMC driver...");

    ra_axis.begin();
}

void loop()
{
    int delay_ticks = 0;
    trackingRates.readTrackingRatePresetsFromEEPROM();

    if (DEFAULT_ENABLE_TRACKING == 1)
    {
        ra_axis.startTracking(ra_axis.rate.tracking, ra_axis.direction.tracking);
    }

    for (;;)
    {
        if (ra_axis.slewActive)
        {
            // Blink status LED if mount is in slew mode
            digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
            delay_ticks = 150; // Delay for 150 ms
        }
        else
        {
            // Turn on status LED if sidereal tracking is ON
            digitalWrite(STATUS_LED, ra_axis.trackingActive ? HIGH : LOW);
            delay_ticks = 1000; // Delay for 1 second
        }
        ra_axis.print_status();
        vTaskDelay(delay_ticks);
    }
}

void webserverTask(void* pvParameters)
{
    for (;;)
    {
        server.handleClient();
        vTaskDelay(1);
    }
}

void intervalometerTask(void* pvParameters)
{
    intervalometer = new Intervalometer(INTERV_PIN);
    intervalometer->readPresetsFromEEPROM();

    for (;;)
    {
        if (intervalometer->isActive())
            intervalometer->cleanup();
        vTaskDelay(1);
    }
}

void uartTask(void* pvParameters)
{
    for (;;)
    {
        uart_task();
        vTaskDelay(1);
    }
}

void consoleTask(void* pvParameters)
{
    for (;;)
    {
        term.readSerial();
        vTaskDelay(1);
    }
}
