#ifndef COLORED_ARDUINO_STREAM_H
#define COLORED_ARDUINO_STREAM_H

#if defined(ARDUINO) || defined(ESP_PLATFORM)

#include "ArduinoStream.h"

namespace EmbeddedTerminal
{
    class ColoredArduinoStream : public ArduinoStream
    {

    public:
        ColoredArduinoStream(Stream &stream) : ArduinoStream(stream) {}

        void printTo(TerminalChannel channel, const ETString &s) override
        {
            if (channel == TerminalChannel::StdErr)
            {
                // ANSI escape code for red text
                print("\033[31m");
                print(s);
                // Reset color
                print("\033[0m");
            }
            else
            {
                print(s);
            }
        }
    };
} // namespace EmbeddedTerminal

#endif // ARDUINO || ESP_PLATFORM

#endif // COLORED_ARDUINO_STREAM_H
