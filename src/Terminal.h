#ifndef TERMINAL_H
#define TERMINAL_H

#include "ETTypes.h"
#include "interfaces/ITerminalStream.h"
#include "interfaces/ICommand.h"
#include <map>
#include <string>
#include <sstream>

#if defined(ARDUINO) || defined(ESP_PLATFORM)
#include <Arduino.h>
#include "hal/ArduinoStream.h"
#endif

namespace EmbeddedTerminal
{
    class Terminal
    {
    public:
        Terminal(ITerminalStream &input);
#if defined(ARDUINO) || defined(ESP_PLATFORM)
        Terminal(Stream &stream);
#endif
        ~Terminal();
        void registerCommand(ETString keyword, ICommand *observer);
        void loop();
        const ETMap<ETString, ICommand *> getCommands();

    protected:
        void call(ETString keyword, ETString additional);

    private:
        ETMap<ETString, ICommand *> _observer;
        ITerminalStream &_input;
#if defined(ARDUINO) || defined(ESP_PLATFORM)
        ArduinoStream *_ownedStream;
#endif

        ETString buffer;
        ETString lineBuffer;
        ETString lineDelimiter = "\n";
    };
}
#endif // TERMINAL_H