#include "Terminal.h"
#include <string>
#include <algorithm>

namespace EmbeddedTerminal
{

    Terminal::Terminal(ITerminalStream &input) : _input(input)
#if defined(ARDUINO)
                                                 ,
                                                 _ownedStream(nullptr)
#endif
    {
        buffer.reserve(BUFFER_RESERVE_SIZE);
    }

#if defined(ARDUINO)
    Terminal::Terminal(Stream &stream) : _ownedStream(new ArduinoStream(stream)), _input(*_ownedStream)
    {
        buffer.reserve(BUFFER_RESERVE_SIZE);
    }
#endif

    Terminal::~Terminal()
    {
#if defined(ARDUINO)
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

        // Check for TAB character (0x09) for auto completion
        bool hasTab = inputLine.contains('\t');
        if (hasTab)
        {
            // Remove the TAB from the input line (don't echo it)
            size_t tabPos = inputLine.find('\t');
            while (tabPos != ETString::npos)
            {
                inputLine.erase(tabPos, 1);
                tabPos = inputLine.find('\t');
            }
        }

        _input.print(inputLine);
        buffer += inputLine;

        // Handle auto completion after buffer is updated
        if (hasTab)
        {
            _handleAutoCompletion();
        }

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

    const ETString &Terminal::getBuffer() const
    {
        return buffer;
    }

    ETString Terminal::getLastWord() const
    {
        // Extract the last word from the buffer (after the last space)
        size_t lastSpacePos = buffer.find_last_of(' ');
        if (lastSpacePos != ETString::npos)
        {
            return buffer.substr(lastSpacePos + 1);
        }
        return buffer;
    }

    void Terminal::_handleAutoCompletion()
    {
        // Get the keyword (first word in buffer)
        ETString keywordPart = buffer;
        size_t spacePos = buffer.find(' ');
        if (spacePos != ETString::npos)
        {
            keywordPart = buffer.substr(0, spacePos);
        }

        // Get the partial argument being typed
        ETString partialArg = getLastWord();

        // Lookup the command and check if it has auto completion support
        auto search = _observer.find(keywordPart);
        if (search != _observer.end())
        {
            ICommand *cmd = search->second;
            // Call getSuggestions directly - it returns empty vector if not overridden
            ETVector<ETString> suggestions = cmd->getSuggestions(partialArg);

            if (suggestions.empty())
            {
                // No matches, just beep
                _input.print("\a");
                return;
            }

            if (suggestions.size() == 1)
            {
                // Single match - auto-complete it
                ETString match = suggestions[0];
                if (match.length() > partialArg.length())
                {
                    ETString toAppend = match.substr(partialArg.length());
                    buffer += toAppend;
                    _input.print(toAppend);
                }
            }
            else
            {
                // Multiple matches - find common prefix and display options
                ETString commonPrefix = suggestions[0];
                for (size_t i = 1; i < suggestions.size(); i++)
                {
                    size_t j = 0;
                    while (j < commonPrefix.length() && j < suggestions[i].length() &&
                           commonPrefix[j] == suggestions[i][j])
                    {
                        j++;
                    }
                    commonPrefix = commonPrefix.substr(0, j);
                }

                // Auto-complete to common prefix
                if (commonPrefix.length() > partialArg.length())
                {
                    ETString toAppend = commonPrefix.substr(partialArg.length());
                    buffer += toAppend;
                    _input.print(toAppend);
                }

                // Display remaining options
                _input.print("\n");
                for (const auto &suggestion : suggestions)
                {
                    _input.printf("  %s\n", suggestion.c_str());
                }
            }
        }
    }
}
