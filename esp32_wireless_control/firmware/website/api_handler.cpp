#include "api_handler.h"
#include "../axis.h"
#include "../catalogues/star_database.h"
#include "../commands.h"
#include "../configs/consts.h"
#include "../eeprom_manager.h"
#include "../error.h"
#include "../functions/intervalometer/intervalometer.h"
#include "../tracking_rates.h"
#include "../uart.h"
#include "web_languages.h"
#include "website_strings.h"

// Forward declarations of external objects
extern Axis ra_axis;
extern Intervalometer intervalometer;
extern TrackingRates trackingRates;
extern Languages language;
extern StarDatabase* starDatabase;
extern StarDatabase* handleStarDatabase(StarDatabaseType catalogType);

// External HTML interface data
extern const uint8_t _interface_index_html_start[] asm("_binary_interface_index_html_start");
extern const uint8_t _interface_index_html_end[] asm("_binary_interface_index_html_end");

// Global pointer to ApiHandler instance for lambda callbacks
static ApiHandler* g_apiHandler = nullptr;

ApiHandler::ApiHandler(WebServer& server) : _server(&server)
{
}

#if DEBUG == 1
static void logRequest(WebServer* server, const char* endpoint)
{
    print_out("HTTP Request: %s %s", server->method() == HTTP_GET ? "GET" : "POST", endpoint);
    print_out("  Client: %s", server->client().remoteIP().toString().c_str());
    if (server->args() > 0)
    {
        print_out("  Arguments (%d):", server->args());
        for (int i = 0; i < server->args(); i++)
        {
            print_out("    %s = %s", server->argName(i).c_str(), server->arg(i).c_str());
        }
    }
}
#endif

static Position calculatePosition(String Arg)
{
    Position position(0, 0, 0);
    position.arcseconds = Arg.toInt();

    if (position.arcseconds == -1)
    {
        position.arcseconds = 0;
#if DEBUG == 1
        print_out("Invalid position input. Defaulting to 0.");
#endif
    }

    return position;
}

void ApiHandler::registerEndpoints()
{
    // Store global pointer for lambda callbacks
    g_apiHandler = this;

    // Web interface
    _server->on("/", HTTP_GET, []() { g_apiHandler->handleRoot(); });

    // Tracking control
    _server->on("/on", HTTP_GET, []() { g_apiHandler->handleOn(); });
    _server->on("/off", HTTP_GET, []() { g_apiHandler->handleOff(); });

    // Slewing control
    _server->on("/startslew", HTTP_GET, []() { g_apiHandler->handleSlewRequest(); });
    _server->on("/stopslew", HTTP_GET, []() { g_apiHandler->handleSlewOff(); });

    // Goto control
    _server->on("/gotoRA", HTTP_GET, []() { g_apiHandler->handleGotoRA(); });
    _server->on("/abort-goto-ra", HTTP_GET, []() { g_apiHandler->handleAbortGoToRA(); });

    // Position management
    _server->on("/setPosition", HTTP_GET, []() { g_apiHandler->handleSetPosition(); });
    _server->on("/getCurrentPosition", HTTP_GET,
                []() { g_apiHandler->handleGetCurrentPosition(); });

    // Intervalometer control
    _server->on("/setCurrent", HTTP_GET, []() { g_apiHandler->handleSetCurrent(); });
    _server->on("/readPreset", HTTP_GET, []() { g_apiHandler->handleGetPresetExposureSettings(); });
    _server->on("/abort", HTTP_GET, []() { g_apiHandler->handleAbortCapture(); });

    // Tracking rates
    _server->on("/getTrackingRates", HTTP_GET, []() { g_apiHandler->handleGetTrackingRates(); });
    _server->on("/saveTrackingRatePreset", HTTP_GET,
                []() { g_apiHandler->handleSaveTrackingRatePreset(); });
    _server->on("/loadTrackingRatePreset", HTTP_GET,
                []() { g_apiHandler->handleLoadTrackingRatePreset(); });

    // Status & info
    _server->on("/status", HTTP_GET, []() { g_apiHandler->handleStatusRequest(); });
    _server->on("/version", HTTP_GET, []() { g_apiHandler->handleVersion(); });

    // Catalog search
    _server->on("/starSearch", HTTP_GET, []() { g_apiHandler->handleCatalogSearch(); });

    // Settings
    _server->on("/setlang", HTTP_GET, []() { g_apiHandler->handleSetLanguage(); });
}

// Handler implementations
void ApiHandler::handleRoot()
{
#if DEBUG == 1
    logRequest(_server, "/");
    print_out("  Client IP: %s", _server->client().remoteIP().toString().c_str());
    print_out("  User-Agent: %s", _server->header("User-Agent").c_str());
#endif
    size_t htmlSize = _interface_index_html_end - _interface_index_html_start;

    // Work with raw bytes and avoid String for null-byte handling
    // Allocate buffer for HTML with extra space for replacements
    char* htmlBuffer = (char*) pvPortMalloc(htmlSize + 10000);
    if (!htmlBuffer)
    {
        _server->send(500, MIME_TYPE_TEXT, "Internal Server Error: Out of memory");
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

    _server->send_P(200, MIME_TYPE_HTML, htmlBuffer, currentSize);
    vPortFree(htmlBuffer);

#if DEBUG == 1
    print_out("  Raw HTML pointers: start=%p, end=%p, calculated size=%d",
              _interface_index_html_start, _interface_index_html_end, currentSize);
    print_out("  Final HTML size: %d bytes", currentSize);
    print_out("  Language: %d, Placeholders replaced: %d", language, numberOfHTMLStrings);
    print_out("  Response sent successfully");
#endif
}

void ApiHandler::handleOn()
{
#if DEBUG == 1
    logRequest(_server, "/on");
#endif
    print_out("Handling tracking ON request with custom rate: %d",
              _server->arg(TRACKING_SPEED).toInt());

    uint64_t custom_rate = _server->arg(TRACKING_SPEED).toInt();
    int direction = _server->arg(DIRECTION).toInt();

    trackingRates.setCustomRate(custom_rate);
#if DEBUG == 1
    print_out("  Direction: %d, Final rate: %llu", direction, trackingRates.getRate());
#endif
    ra_axis.startTracking(trackingRates.getRate(), direction);

    if (intervalometer.getErrorMessage() == ErrorMessage::ERR_MSG_NONE)
        _server->send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_TRACKING_ON]);
    else
        _server->send(200, MIME_TYPE_TEXT,
                      languageErrorMessageStrings[language][intervalometer.getErrorMessage()]);
#if DEBUG == 1
    print_out("  Tracking ON response sent");
#endif
}

void ApiHandler::handleOff()
{
    ra_axis.stopTracking();
    _server->send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_TRACKING_OFF]);
}

void ApiHandler::handleSlewRequest()
{
    if (!ra_axis.slewActive)
    { // if slew is not active - needed for ipad (passes multiple touchon events)
        int slew_speed = _server->arg(SPEED).toInt();
        int direction = _server->arg(DIRECTION).toInt();
        // limit custom slew speed to 2-400
        slew_speed = slew_speed > MAX_CUSTOM_SLEW_RATE   ? MAX_CUSTOM_SLEW_RATE
                     : slew_speed < MIN_CUSTOM_SLEW_RATE ? MIN_CUSTOM_SLEW_RATE
                                                         : slew_speed;
        ra_axis.startSlew((2 * ra_axis.rate.tracking) / slew_speed, direction);
        _server->send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_SLEWING]);
    }
}

void ApiHandler::handleSlewOff()
{
    if (ra_axis.slewActive)
    { // if slew is active needed for ipad (passes multiple touchoff events)
        ra_axis.stopSlew();
    }
    _server->send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_SLEW_CANCELLED]);
}

void ApiHandler::handleSetLanguage()
{
    int lang = _server->arg("lang").toInt();
    language = static_cast<Languages>(lang);
    EepromManager::writeObject(LANG_EEPROM_ADDR, language);
    _server->send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_OK]);
}

void ApiHandler::handleSetCurrent()
{
    if (!intervalometer.isActive())
    {
        // Reset the current error message
        intervalometer.setErrorMessage(ERR_MSG_NONE);

        Intervalometer::Settings settings = intervalometer.getSettings();

        int modeInt = _server->arg(CAPTURE_MODE).toInt();
        if (modeInt < 0 || modeInt >= static_cast<int>(Intervalometer::Mode::MaxModes))
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_CAPTURE_MODE);
            return;
        }
        intervalometer.setMode(static_cast<Intervalometer::Mode>(modeInt));

        int exposureTime = _server->arg(EXPOSURE_TIME).toInt();
        if (exposureTime <= 0)
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_EXPOSURE_LENGTH);
            return;
        }
        settings.exposureTime = exposureTime;

        int exposures = _server->arg(EXPOSURES).toInt();
        if (exposures <= 0)
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_EXPOSURE_AMOUNT);
            return;
        }
        settings.exposures = exposures;

        int preDelay = _server->arg(PREDELAY).toInt();
        if (preDelay < 0)
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_PREDELAY_TIME);
            return;
        }
        else if (preDelay == 0)
            preDelay = 5;
        settings.preDelay = preDelay;

        int delayTime = _server->arg(DELAY).toInt();
        if (delayTime < 0)
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_DELAY_TIME);
            return;
        }
        settings.delayTime = delayTime;

        int frames = _server->arg(FRAMES).toInt();
        if (frames <= 0)
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_FRAME_AMOUNT);
            return;
        }
        settings.frames = frames;

        float panAngle = _server->arg(PAN_ANGLE).toFloat() / 100;
        if (panAngle < 0.0)
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_PAN_ANGLE);
            return;
        }
        settings.panAngle = panAngle;

        int panDirection = _server->arg(PAN_DIRECTION).toInt();
        if (panDirection < 0 || panDirection > 1)
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_PAN_DIRECTION);
            return;
        }
        settings.panDirection = panDirection;

        int continuousPan = _server->arg(CONTINUOUS_PAN).toInt();
        settings.continuousPan = continuousPan;

        int enableTracking = _server->arg(ENABLE_TRACKING).toInt();
        if (enableTracking < 0 || enableTracking > 1)
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_ENABLE_TRACKING_VALUE);
            return;
        }
        settings.enableTracking = enableTracking;

        int dither = _server->arg(DITHER_CHOICE).toInt();
        if (dither < 0 || dither > 1)
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_DITHER_CHOICE);
            return;
        }
        settings.dither = dither;

        int ditherFrequency = _server->arg(DITHER_FREQUENCY).toInt();
        if (ditherFrequency <= 0)
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_DITHER_FREQUENCY);
            return;
        }
        settings.ditherFrequency = ditherFrequency;

        int focalLength = _server->arg(FOCAL_LENGTH).toInt();
        if (focalLength <= 0)
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_FOCAL_LENGTH);
            return;
        }
        settings.focalLength = focalLength;

        float pixelSize = _server->arg(PIXEL_SIZE).toFloat() / 100;
        if (pixelSize <= 0.0)
        {
            intervalometer.setErrorMessage(ERR_MSG_INVALID_PIXEL_SIZE);
            return;
        }
        settings.pixelSize = pixelSize;
        settings.mode = static_cast<uint8_t>(intervalometer.getMode());
        intervalometer.setSettings(settings);

        String currentMode = _server->arg(MODE);
        if (currentMode == "save")
        {
            int preset = _server->arg(PRESET).toInt();
            intervalometer.saveSettingsToPreset(preset);
            _server->send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_SAVED_PRESET]);
        }
        else if (currentMode == "start")
        {
            Intervalometer::Mode mode = intervalometer.getMode();
            if ((mode == Intervalometer::Mode::LongExposureMovie ||
                 mode == Intervalometer::Mode::LongExposureStill) &&
                !ra_axis.trackingActive)
            {
                _server->send(200, MIME_TYPE_TEXT,
                              languageMessageStrings[language][MSG_TRACKING_NOT_ACTIVE]);
            }
            else
            {
                intervalometer.startCapture();
                _server->send(200, MIME_TYPE_TEXT,
                              languageMessageStrings[language][MSG_CAPTURE_ON]);
            }
        }
    }
    else
    {
        _server->send(200, MIME_TYPE_TEXT,
                      languageMessageStrings[language][MSG_CAPTURE_ALREADY_ON]);
    }
}

void ApiHandler::handleGotoRA()
{
    Position currentPosition = calculatePosition(_server->arg("currentRA"));
    Position targetPosition = calculatePosition(_server->arg("targetRA"));
    int pan_speed = _server->arg(SPEED).toInt();

    pan_speed = pan_speed > MAX_CUSTOM_SLEW_RATE   ? MAX_CUSTOM_SLEW_RATE
                : pan_speed < MIN_CUSTOM_SLEW_RATE ? MIN_CUSTOM_SLEW_RATE
                                                   : pan_speed;

    print_out("GotoRA called with:");
    print_out("  Current RA: %lld arcseconds", currentPosition.arcseconds);
    print_out("  Target RA: %lld arcseconds", targetPosition.arcseconds);
    print_out("  rate: %lld", (int) ((2 * ra_axis.rate.tracking) / pan_speed));

    ra_axis.gotoTarget(TRACKER_MOTOR_MICROSTEPPING / 2, (2 * ra_axis.rate.tracking) / pan_speed,
                       currentPosition, targetPosition);
    _server->send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_GOTO_RA_PANNING_ON]);
}

void ApiHandler::handleSetPosition()
{
    Position currentPosition = calculatePosition(_server->arg("currentRA"));
    int64_t stepPosition = currentPosition.arcseconds * trackingRates.getStepsPerSecondSolar();

    ra_axis.setPosition(stepPosition);
    _server->send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_POSITION_SET_SUCCESS]);
}

void ApiHandler::handleGetPresetExposureSettings()
{
    int preset = _server->arg(PRESET).toInt();
    intervalometer.readSettingsFromPreset(preset);
    ArduinoJson::JsonDocument settings;
    String json;
    const Intervalometer::Settings& s = intervalometer.getSettings();
    settings[MODE] = static_cast<uint8_t>(intervalometer.getMode());
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
    _server->send(200, MIME_APPLICATION_JSON, json);
}

void ApiHandler::handleAbortCapture()
{
    if (intervalometer.isActive())
    {
        intervalometer.abortCapture();
        _server->send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_CAPTURE_OFF]);
    }
    else
    {
        _server->send(200, MIME_TYPE_TEXT,
                      languageMessageStrings[language][MSG_CAPTURE_ALREADY_OFF]);
    }
}

void ApiHandler::handleAbortGoToRA()
{
    if (ra_axis.slewActive)
    {
        ra_axis.stopGotoTarget();
        _server->send(200, MIME_TYPE_TEXT,
                      languageMessageStrings[language][MSG_GOTO_RA_PANNING_OFF]);
    }
}

void ApiHandler::handleStatusRequest()
{
    if (intervalometer.isActive())
    {
        // Build status string with progress info
        String statusMsg;
        uint16_t currentExp = intervalometer.getExposuresTaken();
        uint16_t totalExp = intervalometer.getSettings().exposures;

        switch (intervalometer.getState())
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
            _server->send(200, MIME_TYPE_TEXT, statusMsg);
        }
    }
    else if (ra_axis.slewActive && !ra_axis.goToTarget)
    {
        _server->send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_SLEWING]);
    }
    else if (ra_axis.slewActive && ra_axis.goToTarget)
    {
        _server->send(200, MIME_TYPE_TEXT,
                      languageMessageStrings[language][MSG_GOTO_RA_PANNING_ON]);
    }
    else if (ra_axis.trackingActive)
    {
        _server->send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_TRACKING_ON]);
    }
    else
    {
        if (intervalometer.getErrorMessage() == ErrorMessage::ERR_MSG_NONE)
            _server->send(200, MIME_TYPE_TEXT, languageMessageStrings[language][MSG_IDLE]);
        else
            _server->send(200, MIME_TYPE_TEXT,
                          languageErrorMessageStrings[language][intervalometer.getErrorMessage()]);
    }

    _server->send(204, MIME_TYPE_TEXT, "dummy");
}

void ApiHandler::handleVersion()
{
    _server->send(200, MIME_TYPE_TEXT, (String) INTERNAL_VERSION);
}

void ApiHandler::handleGetTrackingRates()
{
#if DEBUG == 1
    logRequest(_server, "/getTrackingRates");
    print_out("  Client IP: %s", _server->client().remoteIP().toString().c_str());
#endif

    int rateType = _server->arg("type").toInt(); // 0=current, 1=sidereal, 2=solar, 3=lunar
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

    _server->send(200, MIME_TYPE_TEXT, String(rate));
#if DEBUG == 1
    print_out("  Response sent: %s", String(rate).c_str());
#endif
}

void ApiHandler::handleGetCurrentPosition()
{
    String utcTimeStr = _server->arg("utcTime");
    String timezoneStr = _server->arg("timezone");
    float longitude = _server->arg("longitude").toFloat();
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

    _server->send(200, MIME_APPLICATION_JSON, response);
}

void ApiHandler::handleSaveTrackingRatePreset()
{
    int preset = _server->arg(PRESET).toInt();
    uint8_t trackingType = _server->arg(TRACKING_TYPE).toInt();
    uint64_t customRate = _server->arg(CUSTOM_RATE).toInt();

    // Save directly to tracking rate preset system
    trackingRates.saveTrackingRatePreset(preset, trackingType, customRate);

    _server->send(200, MIME_TYPE_TEXT, "Tracking rate preset saved");
}

void ApiHandler::handleLoadTrackingRatePreset()
{
    int preset = _server->arg(PRESET).toInt();

    if (preset < 5)
    {
        ArduinoJson::JsonDocument response;
        response["trackingRateType"] = trackingRates.trackingRatePresets[preset].trackingRateType;
        response["customTrackingRate"] =
            (long long) trackingRates.trackingRatePresets[preset].customTrackingRate;

        trackingRates.loadTrackingRatePreset(preset);

        String json;
        serializeJson(response, json);
        _server->send(200, MIME_APPLICATION_JSON, json);
    }
    else
        _server->send(400, MIME_TYPE_TEXT, "Invalid preset number");
}

void ApiHandler::handleCatalogSearch()
{
    StarDatabaseType catalogType = (StarDatabaseType) _server->arg(STAR_CATALOG).toInt();
    String objectName = _server->arg(STAR_NAME);

    starDatabase = handleStarDatabase(catalogType);

#if DEBUG == 1
    print_out("Received catalog=%d, name=%s", catalogType, objectName.c_str());
#endif

    if (objectName.length() == 0)
    {
        _server->send(400, MIME_TYPE_TEXT, "Object name required");
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
        _server->send(200, MIME_APPLICATION_JSON, json);
    }
    else
    {
#if DEBUG == 1
        print_out("Object not found: %s", objectName.c_str());
#endif
        _server->send(404, "text/plain", "Object not found");
    }
}
