# Wifi Config

## `platformio.ini` changes

The `platformio.ini` file *must* define a macro `HAVE_WIFI_CONFIG` in order to config to detect wifi configuration:

Example:
```yaml
build_flags =
    -D HAVE_WIFI_CONFIG=1
    -D AP_MODE=1
    -D BINARY_NAME=${ogstartracker.custom_binary_name}
    -D VERSION=${ogstartracker.custom_version}
    -D DEBUG=0
    -D MDNS_NAME='"tracker"'
    -D BUILD_VERSION=\"${ogstartracker.custom_version}\"
    -D INTERNAL_VERSION=7
    -D WIFI_SSID='"OG Star Tracker"'
    -D WIFI_PASSWORD='"password123"'
    -D STEPPER_TYPE=STEPPER_0_9
    -D WEBSERVER_PORT=80
    -D AP_MODE=1
    -D TRACKING_RATE=TRACKING_SIDEREAL
    -Wall -Wextra -Os
```

## `wifi_config.h` file

One must a file called `wifi_config.h` with his/hers own wifi network credentials:

```c
#ifndef _WIFI_CONFIG_H_
#define _WIFI_CONFIG_H_

#define WIFI_SSID "your ssid here"
#define WIFI_PASSWORD "your wifi password"

#endif /* _WIFI_CONFIG_H_ */

```