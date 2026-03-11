// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#endif

#include <unity.h>
#include "../src/BuiltinCommandFactory.h"
#include "../src/Terminal.h"
#include "../src/DirectoryNavigator.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/native/MockFileSystem.h"
#include "../Mocks/MockNetworkInterface.h"
#include "../Mocks/MockNetworkSystem.h"
#include "StorageSystem.h"
#include "../Mocks/native/MockStorageMedia.h"
#include "../Mocks/MockGpioInterface.h"
#include "../Mocks/MockGpioPolicy.h"

using namespace EmbeddedTerminal;

IStorageSystem *storage = nullptr;
DirectoryNavigator *dir = nullptr;

void setUp(void)
{
    storage = new StorageSystem();
    auto media = new MockStorageMedia("mock", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage->mountMedia(media, "/");
    dir = new DirectoryNavigator(storage);
}
void tearDown(void)
{
    auto medias = storage->media();
    for (auto media : medias)
    {
        storage->unmountMedia(media->name());
        delete media;
    }
    delete dir;
    dir = nullptr;
    delete storage;
    storage = nullptr;
}

// Test basic factory construction and destruction
void test_factory_construction(void)
{
    BuiltinCommandFactory factory;
    // Constructor should not crash
    TEST_ASSERT_TRUE(true);
}

// Test registerFilesystemCommands with all flags
void test_register_filesystem_commands_all(void)
{
    MockStream stream;
    Terminal term(stream);
    DirectoryNavigator nav(*dir);
    BuiltinCommandFactory factory;

    factory.registerFilesystemCommands(term, nav, CMD_FILESYSTEM_ALL);

    auto commands = term.getCommands();
    TEST_ASSERT_TRUE(commands.find("cat") != commands.end());
    TEST_ASSERT_TRUE(commands.find("cd") != commands.end());
    TEST_ASSERT_TRUE(commands.find("ls") != commands.end());
    TEST_ASSERT_TRUE(commands.find("mkdir") != commands.end());
    TEST_ASSERT_TRUE(commands.find("rm") != commands.end());
    TEST_ASSERT_TRUE(commands.find("rmdir") != commands.end());
    TEST_ASSERT_TRUE(commands.find("tail") != commands.end());
}

// Test selective filesystem command registration
void test_register_filesystem_commands_selective(void)
{
    MockStream stream;
    Terminal term(stream);
    DirectoryNavigator nav(*dir);
    BuiltinCommandFactory factory;

    factory.registerFilesystemCommands(term, nav, CMD_LS | CMD_CD | CMD_CAT);

    auto commands = term.getCommands();
    TEST_ASSERT_TRUE(commands.find("cat") != commands.end());
    TEST_ASSERT_TRUE(commands.find("cd") != commands.end());
    TEST_ASSERT_TRUE(commands.find("ls") != commands.end());
    TEST_ASSERT_TRUE(commands.find("mkdir") == commands.end()); // Not registered
    TEST_ASSERT_TRUE(commands.find("rm") == commands.end());    // Not registered
}

// Test registerDiskCommands
void test_register_disk_commands(void)
{
    MockStream stream;
    Terminal term(stream);
    DirectoryNavigator nav(*dir);
    BuiltinCommandFactory factory;

    factory.registerDiskCommands(term, nav, CMD_DF);

    auto commands = term.getCommands();
    TEST_ASSERT_TRUE(commands.find("df") != commands.end());
}

// Test registerNetworkCommands
void test_register_network_commands(void)
{
    MockStream stream;
    Terminal term(stream);
    MockNetworkSystem net;
    BuiltinCommandFactory factory;

    factory.registerNetworkCommands(term, net, CMD_NETWORK_ALL);

    auto commands = term.getCommands();
    TEST_ASSERT_TRUE(commands.find("ip") != commands.end());
    // Note: download is a filesystem command, not network
}

void test_register_gpio_commands(void)
{
    MockStream stream;
    Terminal term(stream);
    MockGpioInterface gpio;
    MockGpioPolicy policy;
    BuiltinCommandFactory factory;

    factory.registerGpioCommands(term, gpio, policy, nullptr, CMD_GPIO_ALL);

    auto commands = term.getCommands();
    TEST_ASSERT_TRUE(commands.find("gpio") != commands.end());
}

// Test registerHelpCommand
void test_register_help_command(void)
{
    MockStream stream;
    Terminal term(stream);
    BuiltinCommandFactory factory;

    factory.registerHelpCommand(term);

    auto commands = term.getCommands();
    TEST_ASSERT_TRUE(commands.find("help") != commands.end());
}

// Test registerAllCommands
void test_register_all_commands(void)
{
    MockStream stream;
    Terminal term(stream);
    DirectoryNavigator nav(*dir);
    MockNetworkSystem net;
    BuiltinCommandFactory factory;

    factory.registerAllCommands(term, nav, net);

    auto commands = term.getCommands();
    // Verify all command categories are registered
    TEST_ASSERT_TRUE(commands.find("cat") != commands.end());
    TEST_ASSERT_TRUE(commands.find("cd") != commands.end());
    TEST_ASSERT_TRUE(commands.find("ls") != commands.end());
    TEST_ASSERT_TRUE(commands.find("df") != commands.end());
    TEST_ASSERT_TRUE(commands.find("ip") != commands.end());
    TEST_ASSERT_TRUE(commands.find("help") != commands.end());
}

// Test deregisterFilesystemCommands
void test_deregister_filesystem_commands(void)
{
    MockStream stream;
    Terminal term(stream);
    DirectoryNavigator nav(*dir);
    BuiltinCommandFactory factory;

    factory.registerFilesystemCommands(term, nav, CMD_LS | CMD_CD);
    factory.deregisterFilesystemCommands(term, CMD_LS);

    auto commands = term.getCommands();
    TEST_ASSERT_TRUE(commands.find("ls") == commands.end()); // Deregistered
    TEST_ASSERT_TRUE(commands.find("cd") != commands.end()); // Still registered
}

// Test deregisterAllCommands
void test_deregister_all_commands(void)
{
    MockStream stream;
    Terminal term(stream);
    DirectoryNavigator nav(*dir);
    MockNetworkSystem net;
    BuiltinCommandFactory factory;

    factory.registerAllCommands(term, nav, net);
    factory.deregisterAllCommands(term);

    auto commands = term.getCommands();
    TEST_ASSERT_TRUE(commands.find("cat") == commands.end());
    TEST_ASSERT_TRUE(commands.find("df") == commands.end());
    TEST_ASSERT_TRUE(commands.find("ip") == commands.end());
    TEST_ASSERT_TRUE(commands.find("help") == commands.end());
}

// Test that factory owns commands (no double registration)
void test_factory_no_duplicate_registration(void)
{
    MockStream stream;
    Terminal term(stream);

    DirectoryNavigator nav(*dir);
    BuiltinCommandFactory factory;

    // Register twice
    factory.registerFilesystemCommands(term, nav, CMD_LS);
    factory.registerFilesystemCommands(term, nav, CMD_LS);

    auto commands = term.getCommands();
    TEST_ASSERT_TRUE(commands.find("ls") != commands.end());
    // Should not crash or create duplicates
}

// Test factory destruction cleans up commands
void test_factory_cleanup_on_destruction(void)
{
    MockStream stream;
    Terminal term(stream);
    {

        DirectoryNavigator nav(*dir);
        BuiltinCommandFactory factory;
        factory.registerFilesystemCommands(term, nav, CMD_LS);
        // Factory goes out of scope here and should clean up
    }
    // Terminal should still be valid (doesn't own the commands)
    TEST_ASSERT_TRUE(true);
}

// Test multiple factories don't interfere
void test_multiple_factories(void)
{
    MockStream stream1, stream2;
    Terminal term1(stream1);
    Terminal term2(stream2);

    DirectoryNavigator nav(*dir);

    BuiltinCommandFactory factory1;
    BuiltinCommandFactory factory2;

    factory1.registerFilesystemCommands(term1, nav, CMD_LS);
    factory2.registerFilesystemCommands(term2, nav, CMD_CD);

    auto cmds1 = term1.getCommands();
    auto cmds2 = term2.getCommands();

    TEST_ASSERT_TRUE(cmds1.find("ls") != cmds1.end());
    TEST_ASSERT_TRUE(cmds1.find("cd") == cmds1.end());
    TEST_ASSERT_TRUE(cmds2.find("cd") != cmds2.end());
    TEST_ASSERT_TRUE(cmds2.find("ls") == cmds2.end());
}

// Test command execution through factory
void test_factory_command_execution(void)
{
    MockStream stream;
    Terminal term(stream);
    DirectoryNavigator nav(*dir);
    BuiltinCommandFactory factory;

    factory.registerHelpCommand(term);

    stream.inputBuffer = "help\n";
    term.loop();

    // Help should produce output
    TEST_ASSERT_TRUE(stream.outputBuffer.length() > 0);
    TEST_ASSERT_TRUE(stream.outputBuffer.find("help") != ETString::npos);
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_factory_construction);
    RUN_TEST(test_register_filesystem_commands_all);
    RUN_TEST(test_register_filesystem_commands_selective);
    RUN_TEST(test_register_disk_commands);
    RUN_TEST(test_register_network_commands);
    RUN_TEST(test_register_gpio_commands);
    RUN_TEST(test_register_help_command);
    RUN_TEST(test_register_all_commands);
    RUN_TEST(test_deregister_filesystem_commands);
    RUN_TEST(test_deregister_all_commands);
    RUN_TEST(test_factory_no_duplicate_registration);
    RUN_TEST(test_factory_cleanup_on_destruction);
    RUN_TEST(test_multiple_factories);
    RUN_TEST(test_factory_command_execution);
    UNITY_END();
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    process_tests();
}
#elif defined(ARDUINO)
void setup()
{
    delay(2500);
    process_tests();
}
void loop() {}
#else
int main()
{
    process_tests();
    return 0;
}
#endif
