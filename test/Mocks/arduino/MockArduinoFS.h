#pragma once

#if defined(ARDUINO) && __has_include(<LittleFS.h>)
#include <LittleFS.h>

#define MockArduinoFS LittleFS

inline void resetMockArduinoFS()
{
    LittleFS.end();
    LittleFS.begin(true);
    LittleFS.format();
    LittleFS.begin(true);
}

#elif defined(ARDUINO) && __has_include(<SPIFFS.h>)
#include <SPIFFS.h>

#define MockArduinoFS SPIFFS

inline void resetMockArduinoFS()
{
    SPIFFS.end();
    SPIFFS.begin(true);
    SPIFFS.format();
    SPIFFS.begin(true);
}

#else

#include "FS.h"

inline FS &getMockArduinoFS()
{
    static FS mockFs;
    return mockFs;
}

#define MockArduinoFS getMockArduinoFS()

inline void resetMockArduinoFS()
{
    MockArduinoFS.clear();
}

#endif
