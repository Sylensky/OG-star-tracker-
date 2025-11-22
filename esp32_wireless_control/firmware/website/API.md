# OG Star Tracker REST API Documentation

## Overview
The OG Star Tracker provides a REST API over HTTP for remote control and monitoring. All endpoints use GET requests and return either plain text or JSON responses.

**Base URL:** `http://<device-ip>/`  
**Default Port:** 80

---

## Table of Contents
1. [Tracking Control](#tracking-control)
2. [Slewing Control](#slewing-control)
3. [Goto Control](#goto-control)
4. [Position Management](#position-management)
5. [Intervalometer Control](#intervalometer-control)
6. [Tracking Rates](#tracking-rates)
7. [Status & Info](#status--info)
8. [Catalog Search](#catalog-search)
9. [Settings](#settings)

---

## Tracking Control

### Enable Tracking
**Endpoint:** `GET /on`  
**Description:** Enable sidereal tracking at specified rate and direction

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `direction` | integer | Yes | Hemisphere/direction: 0=northern/clockwise, 1=southern/counter-clockwise |
| `trackingSpeed` | integer | Yes | Tracking rate value (depends on tracking rate type selected) |

**Response:** `200 OK` - "Tracking ON"

**Example:**
```
GET http://192.168.4.1/on?direction=0&trackingSpeed=15956
```

### Disable Tracking
**Endpoint:** `GET /off`  
**Description:** Disable sidereal tracking  
**Response:** `200 OK` - "Tracking OFF"

**Example:**
```
GET http://192.168.4.1/off
```

---

## Slewing Control

### Start Slewing
**Endpoint:** `GET /startslew`  
**Description:** Start manual slewing at specified speed  

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `speed` | integer | Yes | Slew speed multiplier (2-400, lower=faster) |
| `direction` | integer | Yes | Direction: 0=left/west, 1=right/east |

**Response:** `200 OK` - "Slewing"

**Example:**
```
GET http://192.168.4.1/startslew?speed=16&direction=1
```

### Stop Slewing
**Endpoint:** `GET /stopslew`  
**Description:** Stop current slewing operation  
**Response:** `200 OK` - "Slew Cancelled"

**Example:**
```
GET http://192.168.4.1/stopslew
```

---

## Goto Control

### Goto RA Position
**Endpoint:** `GET /gotoRA`  
**Description:** Move mount to target RA position  

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `currentRA` | string | Yes | Current RA position (HH:MM:SS format) |
| `targetRA` | string | Yes | Target RA position (HH:MM:SS format) |
| `speed` | integer | Yes | Goto speed multiplier (2-400, lower=faster) |

**Response:** `200 OK` - "Goto RA - Panning ON"

**Example:**
```
GET http://192.168.4.1/gotoRA?currentRA=12:30:45&targetRA=14:20:30&speed=8
```

### Abort Goto
**Endpoint:** `GET /abort-goto-ra`  
**Description:** Abort current goto RA operation  
**Response:** `200 OK` - "Goto RA aborted"

**Example:**
```
GET http://192.168.4.1/abort-goto-ra
```

---

## Position Management

### Set Position
**Endpoint:** `GET /setPosition`  
**Description:** Set current mount position  

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `currentRA` | string | Yes | Current RA position (HH:MM:SS format) |

**Response:** `200 OK` - "Position Set Success"

**Example:**
```
GET http://192.168.4.1/setPosition?currentRA=12:30:45
```

### Get Current Position
**Endpoint:** `GET /getCurrentPosition`  
**Description:** Get current mount position in steps  

**Response:** `200 OK` - JSON object
```json
{
  "position": 123456
}
```

**Example:**
```
GET http://192.168.4.1/getCurrentPosition
```

---

## Intervalometer Control

### Configure and Start Capture
**Endpoint:** `GET /setCurrent`  
**Description:** Configure and start intervalometer capture sequence  

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `mode` | integer | Yes | 0=start capture, 1=save to preset |
| `preset` | integer | Yes | Preset number (0-4) |
| `captureMode` | integer | Yes | 0=LONG_EXPOSURE_STILL, 1=LONG_EXPOSURE_MOVIE, 2=TIMELAPSE, 3=TIMELAPSE_PAN, |
| `exposureTime` | integer | Yes | Exposure duration in seconds |
| `exposures` | integer | Yes | Number of exposures |
| `preDelay` | integer | Yes | Pre-delay in seconds |
| `delay` | integer | Yes | Delay between exposures in seconds |
| `frames` | integer | Yes | Number of frames (for MOVIE mode) |
| `panAngle` | integer | Yes | Pan angle in degrees × 100 |
| `panDirection` | integer | Yes | 0=left/west, 1=right/east |
| `enableTracking` | integer | Yes | 0=off, 1=on |
| `ditherChoice` | integer | Yes | 0=off, 1=on |
| `ditherFrequency` | integer | Yes | Dither every N exposures |
| `focalLength` | integer | Yes | Focal length in mm |
| `pixelSize` | integer | Yes | Pixel size in microns × 100 |

**Response:** `200 OK` - Success message or error

**Example:**
```
GET http://192.168.4.1/setCurrent?mode=0&preset=0&captureMode=0&exposureTime=30&exposures=10&preDelay=5&delay=2&frames=1&panAngle=0&panDirection=1&enableTracking=1&ditherChoice=1&ditherFrequency=3&focalLength=200&pixelSize=350
```

### Read Preset
**Endpoint:** `GET /readPreset`  
**Description:** Load intervalometer preset settings  

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `preset` | integer | Yes | Preset number (0-4) |

**Response:** `200 OK` - JSON object with preset settings
```json
{
  "mode": 0,
  "exposures": 10,
  "delay": 2,
  "preDelay": 5,
  "exposureTime": 30,
  "panAngle": 0,
  "panDirection": 1,
  "ditherChoice": 1,
  "ditherFrequency": 3,
  "enableTracking": 1,
  "frames": 1,
  "pixelSize": 350,
  "focalLength": 200
}
```

**Example:**
```
GET http://192.168.4.1/readPreset?preset=0
```

### Abort Capture
**Endpoint:** `GET /abort`  
**Description:** Abort current intervalometer capture sequence  
**Response:** `200 OK` - "Capture OFF"

**Example:**
```
GET http://192.168.4.1/abort
```

---

## Tracking Rates

### Get Tracking Rates
**Endpoint:** `GET /getTrackingRates`  
**Description:** Get current tracking rate configuration  

**Response:** `200 OK` - JSON object
```json
{
  "type": 0,
  "customRate": 0
}
```
**Types:** 0=SIDEREAL, 1=LUNAR, 2=SOLAR, 3=CUSTOM

**Example:**
```
GET http://192.168.4.1/getTrackingRates
```

### Save Tracking Rate Preset
**Endpoint:** `GET /saveTrackingRatePreset`  
**Description:** Save tracking rate to preset  

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `preset` | integer | Yes | Preset number (0-4) |
| `type` | integer | Yes | 0=SIDEREAL, 1=LUNAR, 2=SOLAR, 3=CUSTOM |
| `customRate` | integer | No | Custom rate value (required if type=3) |

**Response:** `200 OK` - Success message

**Example:**
```
GET http://192.168.4.1/saveTrackingRatePreset?preset=0&type=0&customRate=0
```

### Load Tracking Rate Preset
**Endpoint:** `GET /loadTrackingRatePreset`  
**Description:** Load tracking rate from preset  

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `preset` | integer | Yes | Preset number (0-4) |

**Response:** `200 OK` - JSON object with tracking rate settings

**Example:**
```
GET http://192.168.4.1/loadTrackingRatePreset?preset=0
```

---

## Status & Info

### Get Status
**Endpoint:** `GET /status`  
**Description:** Get device status information  

**Response:** `200 OK` - JSON object
```json
{
  "slewActive": false,
  "trackingActive": true,
  "intervalometerActive": false,
  "goToTarget": false,
  "exposuresTaken": 0,
  "currentExposure": 0
}
```

**Example:**
```
GET http://192.168.4.1/status
```

### Get Version
**Endpoint:** `GET /version`  
**Description:** Get firmware version information  

**Response:** `200 OK` - JSON object
```json
{
  "version": "2.0.0",
  "buildDate": "2025-11-07"
}
```

**Example:**
```
GET http://192.168.4.1/version
```

---

## Catalog Search

### Search Star/Object Catalog
**Endpoint:** `GET /starSearch`  
**Description:** Search star/object catalog by name  

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `catalog` | integer | Yes | 0=NGC2000, 1=NGC2000_COMPACT, 2=BSC5, 3=BSC5_COMPACT |
| `query` | string | Yes | Search query string (star/object name) |

**Response:** `200 OK` - JSON object with search results

**Example:**
```
GET http://192.168.4.1/starSearch?catalog=0&query=M31
```

---

## Settings

### Set Language
**Endpoint:** `GET /setlang`  
**Description:** Set web interface language  

**Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `lang` | integer | Yes | 0=English, 1=German, 2=Chinese |

**Response:** `200 OK` - Success message

**Example:**
```
GET http://192.168.4.1/setlang?lang=0
```

### Get Web Interface
**Endpoint:** `GET /`  
**Description:** Serve main web interface HTML  

**Response:** `200 OK` - HTML content

**Example:**
```
GET http://192.168.4.1/
```

---

## Error Responses

All endpoints may return error messages in plain text format with appropriate HTTP status codes:

- `400 Bad Request` - Invalid parameters
- `404 Not Found` - Endpoint not found
- `500 Internal Server Error` - Server error

---

## Notes

1. **RA Format:** All RA positions use HH:MM:SS format (e.g., "12:30:45")
2. **Scaled Values:** Some values are scaled by 100 (panAngle, pixelSize) to avoid floating point in URL parameters
3. **Direction Convention:** 0=left/west, 1=right/east for all directional parameters
4. **Speed Convention:** Lower speed values = faster movement (range: 2-400)
5. **Preset Numbering:** All presets are numbered 0-4 (5 presets total)

---

## Example Usage

### Python Example
```python
import requests

# Base URL of your star tracker
base_url = "http://192.168.4.1"

# Enable tracking
response = requests.get(f"{base_url}/on")
print(response.text)

# Start a 30-second exposure timelapse with 10 exposures
params = {
    "mode": 0,
    "preset": 0,
    "captureMode": 0,
    "exposureTime": 30,
    "exposures": 10,
    "preDelay": 5,
    "delay": 2,
    "frames": 1,
    "panAngle": 0,
    "panDirection": 1,
    "enableTracking": 1,
    "ditherChoice": 0,
    "ditherFrequency": 1,
    "focalLength": 200,
    "pixelSize": 350
}
response = requests.get(f"{base_url}/setCurrent", params=params)
print(response.text)

# Check status
response = requests.get(f"{base_url}/status")
print(response.json())
```

### JavaScript Example
```javascript
const baseUrl = "http://192.168.4.1";

// Enable tracking
fetch(`${baseUrl}/on`)
  .then(response => response.text())
  .then(data => console.log(data));

// Get status
fetch(`${baseUrl}/status`)
  .then(response => response.json())
  .then(data => console.log(data));

// Start slewing
fetch(`${baseUrl}/startslew?speed=16&direction=1`)
  .then(response => response.text())
  .then(data => console.log(data));
```

---

## Version History

- **v2.1.0-beta03** - Initial REST API documentation
  - Added TIMELAPSE_PAN mode
  - Improved position tracking
  - Enhanced error handling
