/*
 * DefaultStorageMedia Example
 *
 * Demonstrates the built-in default storage media wrappers:
 * - Arduino (ESP32): ArduinoSDMMCStorageMedia (SD_MMC)
 * - ESP-IDF:         ESPIDFSDMMCStorageMedia (mount point)
 * - Native:          NativeSuggestedStorageMedia
 */

#include <Terminal.h>
#include <BuiltinCommandFactory.h>
#include <DirectoryNavigator.h>
#include <StorageSystem.h>
#include <hal/DefaultStorageMedia.h>

using namespace EmbeddedTerminal;

StorageSystem storage;

#if defined(ARDUINO) && defined(ESP32)
ArduinoSDMMCStorageMedia media;
#elif defined(ESP_PLATFORM) || defined(ESP_32)
ESPIDFSDMMCStorageMedia media("sdmmc", "/sdcard");
#else
NativeSuggestedStorageMedia media("native", ".");
#endif

DirectoryNavigator nav(&storage);
Terminal term(Serial);
BuiltinCommandFactory factory;

void setup()
{
#if defined(ARDUINO)
    Serial.begin(115200);
    delay(1000);
#endif

#if defined(ARDUINO) && defined(ESP32)
    media.begin();
#endif

    storage.mountMedia(&media, "");

    factory.registerFilesystemCommands(term, nav);
    factory.registerDiskCommands(term, nav);
    factory.registerHelpCommand(term);

#if defined(ARDUINO)
    Serial.println("Default storage media example ready. Type 'help'.");
    Serial.print("> ");
#endif
}

void loop()
{
    term.loop();
}
