/*
 * DefaultStorageMedia Example
 *
 * Demonstrates the built-in default storage media wrappers:
 * - Arduino (ESP32): ArduinoSDMMCStorageMedia (SD_MMC)
 * - ESP-IDF:         ESPIDFSDMMCStorageMedia (mount point)
 * - Native:          NativeSuggestedStorageMedia
 */

#include <Terminal.h>
#include <DirectoryNavigator.h>
#include <StorageSystem.h>
#include <commands/BuiltinCommands.h>
#include <hal/common/DefaultStorageMedia.h>

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

cmd::cat catCommand(nav);
cmd::cd cdCommand(nav);
cmd::download downloadCommand(nav);
cmd::ls lsCommand(nav);
cmd::mkdir mkdirCommand(nav);
cmd::rm rmCommand(nav);
cmd::rmdir rmdirCommand(nav);
cmd::tail tailCommand(nav);
cmd::pwd pwdCommand(nav);
cmd::xxd xxdCommand(nav);
cmd::touch touchCommand(nav);
cmd::echo echoCommand;
cmd::wc wcCommand(nav);
cmd::df dfCommand(storage);
cmd::help helpCommand(term);

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

    term.registerCommand("cat", &catCommand);
    term.registerCommand("cd", &cdCommand);
    term.registerCommand("download", &downloadCommand);
    term.registerCommand("ls", &lsCommand);
    term.registerCommand("mkdir", &mkdirCommand);
    term.registerCommand("rm", &rmCommand);
    term.registerCommand("rmdir", &rmdirCommand);
    term.registerCommand("tail", &tailCommand);
    term.registerCommand("pwd", &pwdCommand);
    term.registerCommand("xxd", &xxdCommand);
    term.registerCommand("touch", &touchCommand);
    term.registerCommand("echo", &echoCommand);
    term.registerCommand("wc", &wcCommand);
    term.registerCommand("df", &dfCommand);
    term.registerCommand("help", &helpCommand);

#if defined(ARDUINO)
    Serial.println("Default storage media example ready. Type 'help'.");
    Serial.print("> ");
#endif
}

void loop()
{
    term.loop();
}
