#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <WebServer.h>

class OTAHandler
{
  public:
    // Singleton pattern - get the instance
    static OTAHandler& getInstance();

    // Initialize with server (call once)
    void init(WebServer* server);

    /**
     * @endpoint GET /ota
     * @brief Serve OTA firmware update web interface
     * @response 200 OK with HTML content
     */
    void handleOTAPage();

    /**
     * @endpoint POST /update
     * @brief Upload and install firmware update
     * @param firmware - Binary firmware file (.bin)
     * @response 200 OK with message "Update Success! Rebooting..." or "Update Failed"
     * @note Device will automatically reboot after successful update
     */
    void handleOTAUpload();

    /**
     * @endpoint POST /update (completion handler)
     * @brief Handle completion of OTA update
     * @response 200 OK with status message
     */
    void handleOTAComplete();

    /**
     * @endpoint GET /checkversion
     * @brief Check for firmware updates on GitHub
     * @response 200 OK with JSON containing version info and update availability
     * @response JSON format:
     * {
     *   "currentVersion": "v2.1",
     *   "buildDate": "Nov 16 2025 10:30:00",
     *   "updateAvailable": true/false,
     *   "latestVersion": "v2.2",  // if update available
     *   "releaseUrl": "https://...",  // if update available
     *   "downloadUrl": "https://...",  // if update available
     *   "releaseNotes": "..."  // if update available
     * }
     */
    void handleCheckVersion();

    /**
     * @endpoint GET /downloadupdate
     * @brief Download and install firmware update directly from GitHub
     * @param url - URL of the firmware.bin file to download
     * @response 200 OK with status updates
     * @note Device will reboot automatically after successful update
     */
    void handleDownloadUpdate();

    /**
     * @endpoint GET /otastatus
     * @brief Get current OTA update progress
     * @response JSON with {active, percent, bytesWritten, totalBytes, complete}
     */
    void handleOTAStatus();

    bool isUpdating() const
    {
        return _updating;
    }

  private:
    // Private constructor for singleton
    OTAHandler() : _server(nullptr), _updating(false), _updateProgress(0)
    {
    }

    // Delete copy constructor and assignment operator
    OTAHandler(const OTAHandler&) = delete;
    OTAHandler& operator=(const OTAHandler&) = delete;

    WebServer* _server;
    bool _updating;
    size_t _updateProgress;

    // OTA download progress tracking
    volatile bool otaActive;
    volatile bool otaComplete;
    volatile bool otaError;
    volatile size_t otaBytesWritten;
    volatile size_t otaTotalBytes;

    // Constants
    static constexpr const char* GITHUB_API_URL =
        "https://api.github.com/repos/OG-star-tech/OG-star-tracker-/releases/latest";
    static constexpr int HTTP_TIMEOUT_MS = 10000;
    static constexpr int DOWNLOAD_TIMEOUT_MS = 30000;
    static constexpr size_t DOWNLOAD_BUFFER_SIZE = 512;
    static constexpr size_t LOG_INTERVAL_KB = 10;

    // Helper methods
    String getCurrentVersion();
    String getCurrentBuildDate();
    String sanitizeString(const String& input, size_t maxLen = 500);
    String extractJsonValue(const String& json, const char* key);
    void resetOTAState();
    void rebootWithDelay(int delaySeconds = 5);
};

#endif // OTA_HANDLER_H
