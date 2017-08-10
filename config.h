//
// Created by qmd on 17-1-6.
//

#ifndef ALEXA_EVENT_CONFIG_H
#define ALEXA_EVENT_CONFIG_H

#define APP_VERSION    "Sat Mar 4 15:34:07 2017 +0800"  //"Sun Jan 22 16:14:13 2017 +0800 c758eed7478d"

#define WAKE_UP_RINGBUFFER_SIZE  (1 * 1024 * 1024L)

#define USER_WAKE_UP   1

//#define USER_CBUF   1

//#define DEB_SAVE_UPLOAD    1

//#define DEB_OLD_LIBMW     1

#define DEV_CONFIG_FILE    "/mnt/UDISK/sphinx/config.ini"
#define WIFI_CONFIG_FILE    "/etc/wifi/wpa_supplicant.conf"

#define VOLUME_CONFIG_FILE    "/mnt/UDISK/sphinx/volume"

#define VOLUME_MIN     99
#define VOLUME_MAX     0


#define DEV_CONFIG_FILE_JSON    "/mnt/UDISK/sphinx/config.json"

#define ALARM_MP3        "/mnt/UDISK/sphinx/alarm.mp3"

#define UPLOAD_APP_URL       "www.iv-tech.com/update/product/avs/arm_alexa_event"
#define UPLOAD_INDEX_URL     "www.iv-tech.com/update/product/avs/index"
#define UPLOAD_INDEX_DIR     "/tmp/index.txt"

#define APP_RUN_DIR    "/mnt/UDISK/sphinx"

#define USER_TRANSFER_BRIDGE 1

#define BUFFER_SIZE 320


#define FILE_SESSION_JSON     "/mnt/UDISK/sphinx/configuration/session.json"
#define FILE_ALERTS_JSON      "/mnt/UDISK/sphinx/session/alerts.json"
#define FILE_SPEAKER_JSON     "/mnt/UDISK/sphinx/session/speaker.json"
#define FILE_TOKEN            "/mnt/UDISK/sphinx/session/token"


#endif //ALEXA_EVENT_CONFIG_H
