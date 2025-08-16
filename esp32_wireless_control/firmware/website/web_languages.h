#ifndef WEB_LANGUAGES_H
#define WEB_LANGUAGES_H

#define numberOfHTMLStrings 77

#include "error.h"

enum Languages
{
    EN,
    CN,
    DE,
    // Add next language
    LANG_COUNT
};

enum Messages
{
    MSG_TRACKING_ON,
    MSG_TRACKING_OFF,
    MSG_SLEWING,
    MSG_SLEW_CANCELLED,
    MSG_OK,
    MSG_SAVED_PRESET,
    MSG_TRACKING_NOT_ACTIVE,
    MSG_CAPTURE_ON,
    MSG_CAPTURE_OFF,
    MSG_CAPTURE_ALREADY_ON,
    MSG_CAPTURE_ALREADY_OFF,
    MSG_CAP_PREDELAY,
    MSG_CAP_EXPOSING,
    MSG_CAP_DITHER,
    MSG_CAP_PANNING,
    MSG_CAP_DELAY,
    MSG_CAP_REWIND,
    MSG_GOTO_RA_PANNING_ON,
    MSG_GOTO_RA_PANNING_OFF,
    MSG_IDLE,
    NUMBER_OF_MESSAGES,
};

extern const char** languageNames[];
extern const char** languageMessageStrings[];
extern const char** languageErrorMessageStrings[];
extern const char** languageHTMLStrings[];
extern const char* HTMLplaceHolders[numberOfHTMLStrings];

#endif