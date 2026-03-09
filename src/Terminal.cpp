#include "Terminal.h"
#include <algorithm>

namespace EmbeddedTerminal
{
    class StreamInputChannel : public IInputChannel
    {
    public:
        explicit StreamInputChannel(ITerminalStream &stream) : stream_(stream)
        {
        }

        bool available() override
        {
            return stream_.available();
        }

        ETString readAll() override
        {
            return stream_.readAll();
        }

    private:
        ITerminalStream &stream_;
    };

    class StreamOutputChannel : public IOutputChannel
    {
    public:
        StreamOutputChannel(ITerminalStream &stream, TerminalChannel channel)
            : stream_(stream), channel_(channel)
        {
        }

        void print(const ETString &s) override
        {
            stream_.printTo(channel_, s);
        }

    private:
        ITerminalStream &stream_;
        TerminalChannel channel_;
    };

    Terminal::Terminal(ITerminalStream &input) : input_(input)
#if defined(ARDUINO)
                                                 ,
                                                 ownedStream_(nullptr)
#endif
    {
        buffer.reserve(BUFFER_RESERVE_SIZE);
    }

#if defined(ARDUINO)
    Terminal::Terminal(Stream &stream) : input_(*(ownedStream_ = new ArduinoStream(stream)))
    {
        buffer.reserve(BUFFER_RESERVE_SIZE);
    }
#endif

    Terminal::~Terminal()
    {
#if defined(ARDUINO)
        if (ownedStream_ != nullptr)
        {
            delete ownedStream_;
            ownedStream_ = nullptr;
        }
#endif
        // Terminal does NOT own commands - caller is responsible for cleanup
    }

    void Terminal::loop()
    {
        if (!input_.available())
            return;

        // Read input and append to buffer
        ETString inputLine = input_.readAll();

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

        input_.print(inputLine);
        buffer += inputLine;

        // Handle auto completion after buffer is updated
        if (hasTab)
        {
            handleAutoCompletion_();
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
        observer_[trimmedKeyword] = observer;
    }

    const ETMap<ETString, ICommand *> &Terminal::getCommands() const
    {
        return observer_;
    }

    void Terminal::deregisterCommand(const ETString &keyword)
    {
        ETString trimmedKeyword = keyword;
        trimmedKeyword.trim();
        auto it = observer_.find(trimmedKeyword);
        if (it != observer_.end())
        {
            // Just remove from map - Terminal does NOT own commands
            observer_.erase(it);
        }
    }

    void Terminal::call(const ETString &keyword, const ETString &additional)
    {
        ETString trimmedKeyword = keyword;
        trimmedKeyword.trim();

        if (trimmedKeyword.empty())
            return;
        auto search = observer_.find(trimmedKeyword);
        if (search != observer_.end())
        {
            StreamInputChannel stdinChannel(input_);
            StreamOutputChannel stdoutChannel(input_, TerminalChannel::StdOut);
            StreamOutputChannel stderrChannel(input_, TerminalChannel::StdErr);

            CommandContext context(sessionVariables_, lastExitCode_, true);
            CommandInvocation invocation{trimmedKeyword, additional, context, stdinChannel, stdoutChannel, stderrChannel};
            CommandResult result = search->second->execute(invocation);
            lastExitCode_ = result.exitCode;
        }
        else
        {
            lastExitCode_ = 127;
            input_.printfTo(TerminalChannel::StdErr, "%s is unknown!\n", trimmedKeyword.c_str());
        }
    }

    int Terminal::getLastExitCode() const
    {
        return lastExitCode_;
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

    void Terminal::handleAutoCompletion_()
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
        auto search = observer_.find(keywordPart);
        if (search != observer_.end())
        {
            ICommand *cmd = search->second;
            // Call getSuggestions directly - it return s empty vector if not overridden
            ETVector<ETString> suggestions = cmd->getSuggestions(partialArg);

            if (suggestions.empty())
            {
                // No matches, just beep
                input_.print("\a");
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
                    input_.print(toAppend);
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
                    input_.print(toAppend);
                }

                // Display remaining options
                input_.print("\n");
                for (const auto &suggestion : suggestions)
                {
                    input_.printf("  %s\n", suggestion.c_str());
                }
            }
        }
    }
}
