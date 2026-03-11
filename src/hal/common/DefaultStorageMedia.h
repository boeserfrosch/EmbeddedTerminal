#pragma once

#include "hal/common/StorageMediaAdapter.h"
#include "hal/common/DefaultGpioSupport.h"

#if defined(ARDUINO)
#include "hal/arduino/ArduinoSDMMCStorageMedia.h"
#endif

#if defined(ESP_PLATFORM) && !defined(ARDUINO)
#include "hal/espidf/ESPIDFSDMMCStorageMedia.h"
#endif

#if !defined(ARDUINO) && !defined(ESP_PLATFORM) && !defined(ESP_32)
#include "hal/native/NativeSuggestedStorageMedia.h"
#endif
