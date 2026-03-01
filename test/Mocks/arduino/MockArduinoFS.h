#pragma once

#include "FS.h"

inline FS MockArduinoFS;

inline void resetMockArduinoFS()
{
    MockArduinoFS.clear();
}
