#include "Terminal.h"
#include <string>
#include <algorithm>
namespace EmbeddedTerminal
{

    Terminal::Terminal(ITerminalStream &input) : _input(input)
#if defined(ARDUINO) || defined(ESP_PLATFORM)
        , _ownedStream(nullptr)
#endif
    {
        buffer.reserve(256);
    }

#if defined(ARDUINO) || defined(ESP_PLATFORM)
    Terminal::Terminal(Stream &stream) : _ownedStream(new ArduinoStream(stream)), _input(*_ownedStream)
    {
        buffer.reserve(256);
    }
#endif

    Terminal::~Terminal()
    {
#if defined(ARDUINO) || defined(ESP_PLATFORM)
        if (_ownedStream != nullptr)
        {
            delete _ownedStream;
        }
#endif
    }

    void Terminal::loop()
    {
        if (!_input.available())
            return;

        // Read input and append to buffer
        ETString inputLine = _input.readAll();
        _input.print(inputLine);
        buffer += inputLine;

        size_t delimPosition;
        while ((delimPosition = buffer.find(lineDelimiter)) != ETString::npos)
        {
            // Extract line
            ETString line = buffer.substr(0, delimPosition);
            buffer.erase(0, delimPosition + 1);

            // Remove non-printable and backspace chars
            ETString cleanedLine = line.cleanupString();

            cleanedLine.trim();
            if (cleanedLine.contains(' '))
            {

                // Split into keyword and residual
                size_t spacePos = cleanedLine.find(' ');
                ETString keyword = cleanedLine.substr(0, spacePos);
                ETString residual = cleanedLine.substr(spacePos + 1);
                call(keyword, residual);
            }
            else
            {
                call(cleanedLine, "");
            }
        }
    }

    void Terminal::registerCommand(ETString keyword, ICommand *observer)
    {
        keyword.trim();
        _observer.insert({keyword, observer});
    }

    const ETMap<ETString, ICommand *> Terminal::getCommands()
    {
        return _observer;
    }

    void Terminal::call(ETString keyword, ETString additional)
    {
        keyword.trim();

        if (keyword.empty())
            return;
        auto search = _observer.find(keyword);
        if (search != _observer.end())
        {
            _input.print(search->second->trigger(keyword, additional));
        }
        else
        {
            _input.printf("%s is unknown!\n", keyword.c_str());
        }
    }
}