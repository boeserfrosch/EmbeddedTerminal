/*
 * CustomCommand Example
 *
 * This example demonstrates how to create and register custom commands
 * with the EmbeddedTerminal library.
 *
 * Custom commands included:
 * - echo   : Echo back the input text
 * - uptime : Show system uptime
 * - led    : Control built-in LED (on/off)
 *
 * Usage:
 * 1. Upload this sketch to your ESP32/Arduino board
 * 2. Open Serial Monitor (115200 baud)
 * 3. Try the custom commands:
 *    > echo Hello World!
 *    > uptime
 *    > led on
 *    > led off
 */

#include <Arduino.h>
#include <Terminal.h>
#include <commands/help.h>
#include <interfaces/ICommand.h>

using namespace EmbeddedTerminal;

// Create terminal instance (Terminal accepts Stream directly on Arduino)
Terminal term(Serial);

// Custom Command 1: Echo
class EchoCommand : public ICommand
{
public:
  ETString usage(const ETString &keyword) const override
  {
    return keyword + " <text> - Echo back the input text";
  }

  CommandResult invoke(CommandInvocation &invocation) override
  {
    ETString text = join(invocation.arguments, " ");
    if (text.empty())
    {
      invocation.streams.error.print("Usage: echo <text>\n");
      return CommandResult::completed(1);
    }

    invocation.streams.output.print(text + "\n");
    return CommandResult::completed(0);
  }
};

// Custom Command 2: Uptime
class UptimeCommand : public ICommand
{
public:
  ETString usage(const ETString &keyword) const override
  {
    return keyword + " - Show system uptime";
  }

  CommandResult invoke(CommandInvocation &invocation) override
  {
    (void)invocation;
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;

    seconds %= 60;
    minutes %= 60;
    hours %= 24;

    ETString result = "System uptime: ";
    if (days > 0)
      result += toETString(static_cast<size_t>(days)) + "d ";
    if (hours > 0)
      result += toETString(static_cast<size_t>(hours)) + "h ";
    if (minutes > 0)
      result += toETString(static_cast<size_t>(minutes)) + "m ";
    result += toETString(static_cast<size_t>(seconds)) + "s";

    invocation.streams.output.print(result + "\n");
    return CommandResult::completed(0);
  }
};

// Custom Command 3: LED Control
class LedCommand : public ICommand
{
  const int LED_PIN = LED_BUILTIN;

public:
  LedCommand()
  {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
  }

  ETString usage(const ETString &keyword) const override
  {
    return keyword + " <on|off> - Control the built-in LED";
  }

  CommandResult invoke(CommandInvocation &invocation) override
  {
    ETString arg = join(invocation.arguments, " ");
    arg.toLowerCase();

    if (arg.empty())
    {
      invocation.streams.error.print("Usage: led <on|off>\n");
      return CommandResult::completed(1);
    }

    if (arg.find("on") != ETString::npos)
    {
      digitalWrite(LED_PIN, HIGH);
      invocation.streams.output.print("LED turned ON\n");
      return CommandResult::completed(0);
    }
    else if (arg.find("off") != ETString::npos)
    {
      digitalWrite(LED_PIN, LOW);
      invocation.streams.output.print("LED turned OFF\n");
      return CommandResult::completed(0);
    }
    else
    {
      invocation.streams.error.print("Invalid argument. Use 'on' or 'off'\n");
      return CommandResult::completed(1);
    }
  }
};

// Custom Command 4: System Info
class InfoCommand : public ICommand
{
public:
  ETString usage(const ETString &keyword) const override
  {
    return keyword + " - Display system information";
  }

  CommandResult invoke(CommandInvocation &invocation) override
  {
    (void)invocation;
    ETString info;

    info += "System Information:\n";
    info += "===================\n";

#if defined(ESP32)
    info += "Platform: ESP32\n";
    info += "Chip Model: " + ETString(ESP.getChipModel()) + "\n";
    info += "Chip Revision: " + toETString(ESP.getChipRevision()) + "\n";
    info += "CPU Frequency: " + toETString(ESP.getCpuFreqMHz()) + " MHz\n";
    info += "Free Heap: " + toETString(ESP.getFreeHeap()) + " bytes\n";
    info += "Flash Size: " + toETString(ESP.getFlashChipSize()) + " bytes\n";
#elif defined(ARDUINO)
    info += "Platform: Arduino\n";
    info += "Free RAM: " + toETString(freeMemory()) + " bytes\n";
#endif

    invocation.streams.output.print(info);
    return CommandResult::completed(0);
  }

private:
#if !defined(ESP32) && defined(ARDUINO)
  // Simple free memory calculation for Arduino
  int freeMemory()
  {
    extern int __heap_start, *__brkval;
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
  }
#endif
};

// Command instances (must be static to persist)
static cmd::help helpCmd(term);
static EchoCommand echoCmd;
static UptimeCommand uptimeCmd;
static LedCommand ledCmd;
static InfoCommand infoCmd;

void setup()
{
  // Initialize Serial
  Serial.begin(115200);
  delay(1000);

  // Welcome message
  Serial.println("\n\n");
  Serial.println("====================================");
  Serial.println("  Custom Commands Example");
  Serial.println("====================================");
  Serial.println();

  // Register built-in help command
  term.registerCommand("help", &helpCmd);

  // Register custom commands
  term.registerCommand("echo", &echoCmd);
  term.registerCommand("uptime", &uptimeCmd);
  term.registerCommand("led", &ledCmd);
  term.registerCommand("info", &infoCmd);

  // Show help
  Serial.println("Custom commands registered. Type 'help' for usage information.");
  Serial.println("Try commands like:");
  Serial.println("  echo Hello World!");
  Serial.println("  uptime");
  Serial.println("  led on");
  Serial.println("  led off");
  Serial.println("  info");
  Serial.print("\n> ");
}

void loop()
{
  // Process terminal input
  term.loop();
}
