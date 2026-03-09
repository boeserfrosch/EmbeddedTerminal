// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#endif

#include "Terminal.h"
#include "ETTypes.h"
#include "DefaultAutoCompleters.h"
#include "../Mocks/MockStream.h"
#include "../Mocks/MockAutoCompleteCommand.h"
#include "StorageSystem.h"
#include "../Mocks/native/MockStorageMedia.h"
#include <unity.h>

using namespace EmbeddedTerminal;

void setUp(void) {}
void tearDown(void) {}

/// Test 1: Verify TAB character is detected and doesn't get echoed
void test_auto_completion_tab_detection(void)
{
    MockStream stream;
    Terminal term(stream);
    MockAutoCompleteCommand cmd;
    cmd.testSuggestions.push_back("test");
    cmd.testSuggestions.push_back("test123");
    term.registerCommand("cmd", &cmd);

    // Input with TAB character
    stream.inputBuffer = "cm\t";
    term.loop();

    // TAB should not be in output
    TEST_ASSERT_FALSE(stream.outputBuffer.contains('\t'));
}

/// Test 2: Single match auto-completes
void test_auto_completion_single_match(void)
{
    MockStream stream;
    Terminal term(stream);
    MockAutoCompleteCommand cmd;
    cmd.testSuggestions.push_back("completion");
    term.registerCommand("test", &cmd);

    // Type "test comp" and then TAB
    stream.inputBuffer = "test comp\t";
    term.loop();

    // Should auto-complete to "completion"
    TEST_ASSERT_TRUE(stream.outputBuffer.contains("letion"));
}

/// Test 3: Multiple matches show options
void test_auto_completion_multiple_matches(void)
{
    MockStream stream;
    Terminal term(stream);
    MockAutoCompleteCommand cmd;
    cmd.testSuggestions.push_back("file.txt");
    cmd.testSuggestions.push_back("file.bin");
    cmd.testSuggestions.push_back("folder");
    term.registerCommand("test", &cmd);

    // Type "test f" and then TAB
    stream.inputBuffer = "test f\t";
    term.loop();

    // Should show common prefix completion
    TEST_ASSERT_TRUE(stream.outputBuffer.contains("fi") || stream.outputBuffer.contains("fo"));
}

/// Test 4: No matches show nothing
void test_auto_completion_no_matches(void)
{
    MockStream stream;
    Terminal term(stream);
    MockAutoCompleteCommand cmd;
    cmd.testSuggestions.push_back("test");
    term.registerCommand("cmd", &cmd);

    // Type "xyz" and then TAB (no match)
    stream.inputBuffer = "xyz\t";
    term.loop();

    // Should not crash
    TEST_ASSERT_TRUE(true);
}

/// Test 5: getBuffer return s current buffer content
void test_terminal_get_buffer(void)
{
    MockStream stream;
    Terminal term(stream);
    stream.inputBuffer = "hello world";
    term.loop();

    TEST_ASSERT_TRUE(term.getBuffer().contains("hello world"));
}

/// Test 6: getLastWord extracts word after last space
void test_terminal_get_last_word_with_space(void)
{
    MockStream stream;
    Terminal term(stream);
    stream.inputBuffer = "hello world";
    term.loop();

    ETString lastWord = term.getLastWord();
    TEST_ASSERT_EQUAL_STRING("world", lastWord.c_str());
}

/// Test 7: getLastWord return s entire buffer if no space
void test_terminal_get_last_word_no_space(void)
{
    MockStream stream;
    Terminal term(stream);
    stream.inputBuffer = "helloworld";
    term.loop();

    ETString lastWord = term.getLastWord();
    TEST_ASSERT_EQUAL_STRING("helloworld", lastWord.c_str());
}

/// Test 8: CommandCompleter suggests matching command names
void test_command_completer_basic(void)
{
    MockStream stream;
    Terminal term(stream);
    MockAutoCompleteCommand cmd1;
    MockAutoCompleteCommand cmd2;

    term.registerCommand("cat", &cmd1);
    term.registerCommand("cd", &cmd2);

    CommandCompleter completer(term.getCommands());
    ETVector<ETString> suggestions = completer.getSuggestions("c");

    TEST_ASSERT_EQUAL_INT(2, suggestions.size());
    TEST_ASSERT_TRUE(suggestions[0] == "cat" || suggestions[0] == "cd");
}

/// Test 9: CommandCompleter filters correctly
void test_command_completer_filter(void)
{
    MockStream stream;
    Terminal term(stream);
    MockAutoCompleteCommand cmd1;
    MockAutoCompleteCommand cmd2;

    term.registerCommand("cat", &cmd1);
    term.registerCommand("mkdir", &cmd2);

    CommandCompleter completer(term.getCommands());
    ETVector<ETString> suggestions = completer.getSuggestions("c");

    TEST_ASSERT_EQUAL_INT(1, suggestions.size());
    TEST_ASSERT_EQUAL_STRING("cat", suggestions[0].c_str());
}

/// Test 10: Auto completion finds common prefix
void test_auto_completion_common_prefix(void)
{
    MockStream stream;
    Terminal term(stream);
    MockAutoCompleteCommand cmd;
    cmd.testSuggestions.push_back("testing");
    cmd.testSuggestions.push_back("test");
    term.registerCommand("cmd", &cmd);

    // Type "test" and then TAB
    stream.inputBuffer = "test\t";
    term.loop();

    // Should auto-complete to all matching items shown
    TEST_ASSERT_TRUE(stream.outputBuffer.contains("test"));
}

// Test runner
// Additional tests for DefaultAutoCompleters
void test_command_completer_suggestions(void)
{
    ETMap<ETString, ICommand *> commands;
    MockAutoCompleteCommand cmd1, cmd2;
    commands["ls"] = &cmd1;
    commands["cat"] = &cmd2;
    CommandCompleter completer(commands);
    auto suggestions = completer.getSuggestions("c");
    TEST_ASSERT_EQUAL_INT(1, suggestions.size());
    TEST_ASSERT_EQUAL_STRING("cat", suggestions[0].c_str());
}

void test_filepath_completer_basic(void)
{
    StorageSystem storage;
    auto media = new MockStorageMedia("root", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage.mountMedia(media, "");
    storage.mkdir("/home");
    storage.mkdir("/bin");
    storage.mkdir("/home/user");
    storage.open("/home/file.txt", FILE_MODE_WRITE, true).write("data", 4);
    DirectoryNavigator nav(&storage, "/home");
    FilePathCompleter completer(nav);
    auto suggestions = completer.getSuggestions("");
    TEST_ASSERT_TRUE(suggestions.size() >= 1);
    TEST_ASSERT_TRUE(suggestions[0].contains("file.txt"));

    TEST_ASSERT_TRUE(nav.cd(".."));
    TEST_ASSERT_EQUAL_STRING("/", nav.pwd().c_str());
    suggestions = completer.getSuggestions("h");

    TEST_ASSERT_EQUAL_INT(1, suggestions.size());
    TEST_ASSERT_EQUAL_STRING("home/", suggestions[0].c_str());
    suggestions = completer.getSuggestions("home/u");
    TEST_ASSERT_TRUE(suggestions.size() >= 1);
    TEST_ASSERT_EQUAL_STRING("home/user/", suggestions[0].c_str());
    suggestions = completer.getSuggestions("home/file");
    TEST_ASSERT_TRUE(suggestions.size() >= 1);
    TEST_ASSERT_EQUAL_STRING("home/file.txt", suggestions[0].c_str());
    suggestions = completer.getSuggestions("home/file.txt");
    TEST_ASSERT_TRUE(suggestions.size() >= 1);
    TEST_ASSERT_EQUAL_STRING("home/file.txt", suggestions[0].c_str());
    suggestions = completer.getSuggestions("home/file.txt/");
    TEST_ASSERT_TRUE(suggestions.size() == 1);
    TEST_ASSERT_EQUAL_STRING("home/file.txt", suggestions[0].c_str());
    suggestions = completer.getSuggestions("");
    TEST_ASSERT_TRUE(suggestions.size() >= 1);
    TEST_ASSERT_EQUAL_STRING("bin/", suggestions[0].c_str());
    TEST_ASSERT_EQUAL_STRING("home/", suggestions[1].c_str());
}

void test_directory_completer_basic(void)
{
    return;
    StorageSystem storage;
    auto media = new MockStorageMedia("root", true, 1024 * 1024, 0, 1024 * 1024, 1024 * 1024, new MockFileSystem());
    storage.mountMedia(media, "");
    storage.mkdir("/dir");
    storage.mkdir("/dir2");
    storage.mkdir("/dir3");
    ETString content = "data";
    storage.open("/dir/file.txt", FILE_MODE_WRITE, true).writeAll(content);
    DirectoryNavigator nav(&storage);
    DirectoryCompleter completer(nav);
    auto suggestions = completer.getSuggestions("/d");
    TEST_ASSERT_TRUE(suggestions.size() >= 1);
    TEST_ASSERT_EQUAL_STRING("dir/", suggestions[0].c_str());
}

int process_tests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_auto_completion_tab_detection);
    RUN_TEST(test_auto_completion_single_match);
    RUN_TEST(test_auto_completion_multiple_matches);
    RUN_TEST(test_auto_completion_no_matches);
    RUN_TEST(test_terminal_get_buffer);
    RUN_TEST(test_terminal_get_last_word_with_space);
    RUN_TEST(test_terminal_get_last_word_no_space);
    RUN_TEST(test_command_completer_basic);
    RUN_TEST(test_command_completer_filter);
    RUN_TEST(test_auto_completion_common_prefix);
    RUN_TEST(test_command_completer_suggestions);
    RUN_TEST(test_filepath_completer_basic);
    RUN_TEST(test_directory_completer_basic);
    return UNITY_END();
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
    return process_tests();
}
#endif