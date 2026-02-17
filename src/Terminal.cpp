#include "Terminal.h"
#include <string>
#include <algorithm>

namespace EmbeddedTerminal
{

    Terminal::Terminal(ITerminalStream &input) : _input(input)
#if defined(ARDUINO) || defined(ESP_PLATFORM)
                                                 ,
                                                 _ownedStream(nullptr)
#endif
    {
        buffer.reserve(BUFFER_RESERVE_SIZE);
    }

#if defined(ARDUINO) || defined(ESP_PLATFORM)
    Terminal::Terminal(Stream &stream) : _ownedStream(new ArduinoStream(stream)), _input(*_ownedStream)
    {
        buffer.reserve(BUFFER_RESERVE_SIZE);
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
        // Terminal does NOT own commands - caller is responsible for cleanup
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

    void Terminal::registerCommand(const ETString &keyword, ICommand *observer)
    {
        if (observer == nullptr)
        {
            return; // Ignore null command pointers
        }

        ETString trimmedKeyword = keyword;
        trimmedKeyword.trim();

        if (trimmedKeyword.empty())
        {
            return; // Ignore empty keywords
        }

        // Store command pointer - Terminal does NOT take ownership
        _observer[trimmedKeyword] = observer;
    }

    const ETMap<ETString, ICommand *> &Terminal::getCommands() const
    {
        return _observer;
    }

    void Terminal::deregisterCommand(const ETString &keyword)
    {
        ETString trimmedKeyword = keyword;
        trimmedKeyword.trim();
        auto it = _observer.find(trimmedKeyword);
        if (it != _observer.end())
        {
            // Just remove from map - Terminal does NOT own commands
            _observer.erase(it);
        }
    }

    void Terminal::call(const ETString &keyword, const ETString &additional)
    {
        ETString trimmedKeyword = keyword;
        trimmedKeyword.trim();

        if (trimmedKeyword.empty())
            return;
        auto search = _observer.find(trimmedKeyword);
        if (search != _observer.end())
        {
            _input.print(search->second->trigger(trimmedKeyword, additional));
        }
        else
        {
            _input.printf("%s is unknown!\n", trimmedKeyword.c_str());
        }
    }
}
