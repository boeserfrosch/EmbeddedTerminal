#include "Terminal.h"
#include "Lexer.h"
#include "TerminalParser.h"
#include "TerminalExecutor.h"
#include <algorithm>

namespace EmbeddedTerminal
{
    namespace
    {
        ETString replaceAll_(const ETString &input, const ETString &needle, const ETString &replacement)
        {
            if (needle.empty())
            {
                return input;
            }

            ETString result = input;
            size_t pos = 0;
            while ((pos = result.find(needle, pos)) != ETString::npos)
            {
                ETString left = result.substr(0, pos);
                ETString right = result.substr(pos + needle.length());
                result = left + replacement + right;
                pos += replacement.length();
            }

            return result;
        }

        ETString substituteLoopVariable_(const ETString &input, const ETString &variableName, const ETString &value)
        {
            ETString result = input;
            result = replaceAll_(result, "${" + variableName + "}", value);
            result = replaceAll_(result, "$" + variableName, value);
            return result;
        }

    }

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

    class BufferedInputChannel : public IInputChannel
    {
    public:
        explicit BufferedInputChannel(const ETString &buffer) : buffer_(buffer), consumed_(false)
        {
        }

        bool available() override
        {
            return !consumed_ && !buffer_.empty();
        }

        ETString readAll() override
        {
            if (consumed_)
            {
                return "";
            }

            consumed_ = true;
            return buffer_;
        }

    private:
        ETString buffer_;
        bool consumed_;
    };

    class BufferedOutputChannel : public IOutputChannel
    {
    public:
        void print(const ETString &s) override
        {
            buffer_ += s;
        }

        const ETString &buffer() const
        {
            return buffer_;
        }

    private:
        ETString buffer_;
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

    void Terminal::setFileSystem(IFileSystem *fileSystem)
    {
        fileSystem_ = fileSystem;
    }

    void Terminal::reportLexerError_(LexerError error)
    {
        lastExitCode_ = 2;

        if (error == LexerError::TOKEN_ARRAY_EXHAUSTED)
        {
            input_.printTo(TerminalChannel::StdErr, "lexer error: token array exhausted\n");
        }
        else if (error == LexerError::UNTERMINATED_SINGLE_QUOTE)
        {
            input_.printTo(TerminalChannel::StdErr, "lexer error: unterminated single quote\n");
        }
        else if (error == LexerError::UNTERMINATED_DOUBLE_QUOTE)
        {
            input_.printTo(TerminalChannel::StdErr, "lexer error: unterminated double quote\n");
        }
        else
        {
            input_.printTo(TerminalChannel::StdErr, "lexer error: invalid command syntax\n");
        }
    }

    void Terminal::executeParsedCommand_(const ParsedCommand &command)
    {
        bool shouldRunPipeline =
            (command.keywords.size() != 1) ||
            (!command.redirectOutPath.empty()) ||
            (!command.redirectInPath.empty());

        if (shouldRunPipeline)
        {
            executePipeline_(command.keywords, command.arguments, command.redirectOutPath, command.appendRedirect, command.redirectInPath);
        }
        else
        {
            call(command.keywords[0], command.arguments[0]);
        }
    }

    void Terminal::continueActiveCommandIfNeeded_()
    {
        if (!hasActiveCommand_)
        {
            return;
        }

        bool shouldContinue = (activeState_ == CommandExecutionState::Running) ||
                              ((activeState_ == CommandExecutionState::WaitingForInput) && input_.available());

        if (shouldContinue)
        {
            executeCommand_(activeCommand_, activeKeyword_, activeArguments_);
        }
    }

    bool Terminal::ingestInputAndHandleAutoCompletion_()
    {
        if (!input_.available())
        {
            return false;
        }

        ETString inputLine = input_.readAll();

        bool hasTab = inputLine.contains('\t');
        if (hasTab)
        {
            size_t tabPos = inputLine.find('\t');
            while (tabPos != ETString::npos)
            {
                inputLine.erase(tabPos, 1);
                tabPos = inputLine.find('\t');
            }
        }

        input_.print(inputLine);
        buffer += inputLine;

        if (hasTab)
        {
            handleAutoCompletion_();
        }

        return true;
    }

    void Terminal::processBufferedLine_(const ETString &line)
    {
        ETString cleanedLine = line.cleanupString().trim();
        if (cleanedLine.empty())
        {
            return;
        }

        TerminalParser parser;

        ETVector<token_t> tokens;
        LexerError lexerError = LexerError::NONE;
        if (!parser.tokenizeLine(cleanedLine, tokens, lexerError))
        {
            reportLexerError_(lexerError);
            return;
        }

        ParsedAst ast;
        if (!parser.parseTokens(tokens, ast))
        {
            reportLexerError_(LexerError::NONE);
            return;
        }

        TerminalExecutor executor(
            [this](const ParsedCommand &command)
            {
                executeParsedCommand_(command);
            },
            [this]()
            {
                return lastExitCode_;
            },
            [](const ETString &input, const ETString &variableName, const ETString &value)
            {
                return substituteLoopVariable_(input, variableName, value);
            });

        executor.execute(ast);
    }

    void Terminal::processBufferedCommands_()
    {
        size_t delimPosition;
        while ((delimPosition = buffer.find(lineDelimiter)) != ETString::npos)
        {
            ETString line = buffer.substr(0, delimPosition);
            buffer.erase(0, delimPosition + 1);
            processBufferedLine_(line);
        }
    }

    void Terminal::loop()
    {
        continueActiveCommandIfNeeded_();
        if (!ingestInputAndHandleAutoCompletion_())
        {
            return;
        }

        processBufferedCommands_();
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
            executeCommand_(search->second, trimmedKeyword, additional);
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

    CommandResult Terminal::executeCommandInternal_(ICommand *command, const ETString &keyword, const ETString &arguments,
                                                    IInputChannel &stdinChannel, IOutputChannel &stdoutChannel, IOutputChannel &stderrChannel)
    {
        if (command == nullptr)
        {
            return CommandResult::completed(127);
        }

        CommandContext context(sessionVariables_, lastExitCode_, true);
        CommandInvocation invocation{keyword, arguments, context, stdinChannel, stdoutChannel, stderrChannel};
        CommandResult result = command->execute(invocation);
        lastExitCode_ = result.exitCode;
        return result;
    }

    void Terminal::executeCommand_(ICommand *command, const ETString &keyword, const ETString &arguments)
    {
        if (command == nullptr)
        {
            return;
        }

        StreamInputChannel stdinChannel(input_);
        StreamOutputChannel stdoutChannel(input_, TerminalChannel::StdOut);
        StreamOutputChannel stderrChannel(input_, TerminalChannel::StdErr);

        CommandResult result = executeCommandInternal_(command, keyword, arguments, stdinChannel, stdoutChannel, stderrChannel);

        if (result.state == CommandExecutionState::Completed)
        {
            hasActiveCommand_ = false;
            activeCommand_ = nullptr;
            activeKeyword_ = "";
            activeArguments_ = "";
            activeState_ = CommandExecutionState::Completed;
            return;
        }

        hasActiveCommand_ = true;
        activeCommand_ = command;
        activeKeyword_ = keyword;
        activeArguments_ = arguments;
        activeState_ = result.state;
    }

    void Terminal::executePipeline_(const ETVector<ETString> &keywords, const ETVector<ETString> &arguments,
                                    const ETString &redirectOutPath, bool appendRedirect, const ETString &redirectInPath)
    {
        if (keywords.empty() || (keywords.size() != arguments.size()))
        {
            lastExitCode_ = 2;
            input_.printTo(TerminalChannel::StdErr, "pipeline error: invalid pipeline\n");
            return;
        }

        StreamInputChannel streamInput(input_);
        StreamOutputChannel stderrChannel(input_, TerminalChannel::StdErr);
        ETString pipedStdout;
        ETString redirectedInputContent;

        if (!redirectInPath.empty())
        {
            if (fileSystem_ == nullptr)
            {
                lastExitCode_ = 2;
                input_.printTo(TerminalChannel::StdErr, "redirection error: filesystem not configured\n");
                return;
            }

            ETFile inputFile = fileSystem_->open(redirectInPath, FILE_MODE_READ, false);
            if (!inputFile.isOpen())
            {
                lastExitCode_ = 2;
                input_.printTo(TerminalChannel::StdErr, "redirection error: failed to open input\n");
                return;
            }

            redirectedInputContent = inputFile.readAll();
            inputFile.close();
        }

        hasActiveCommand_ = false;
        activeCommand_ = nullptr;
        activeKeyword_ = "";
        activeArguments_ = "";
        activeState_ = CommandExecutionState::Completed;

        for (size_t index = 0; index < keywords.size(); ++index)
        {
            ETString commandKey = keywords[index];
            commandKey.trim();
            auto search = observer_.find(commandKey);
            if (search == observer_.end())
            {
                lastExitCode_ = 127;
                input_.printfTo(TerminalChannel::StdErr, "%s is unknown!\n", commandKey.c_str());
                return;
            }

            bool isLast = (index + 1 == keywords.size());
            BufferedInputChannel bufferedInput(pipedStdout);
            BufferedInputChannel redirectedInput(redirectedInputContent);
            BufferedOutputChannel stageOutput;
            StreamOutputChannel finalOutput(input_, TerminalChannel::StdOut);

            IInputChannel &stdinChannel = (index == 0)
                                              ? (redirectInPath.empty() ? static_cast<IInputChannel &>(streamInput)
                                                                        : static_cast<IInputChannel &>(redirectedInput))
                                              : static_cast<IInputChannel &>(bufferedInput);
            bool writeToFile = isLast && !redirectOutPath.empty();
            IOutputChannel &stdoutChannel = (isLast && !writeToFile) ? static_cast<IOutputChannel &>(finalOutput)
                                                                     : static_cast<IOutputChannel &>(stageOutput);

            CommandResult result = executeCommandInternal_(search->second, commandKey, arguments[index], stdinChannel, stdoutChannel, stderrChannel);

            if (result.state != CommandExecutionState::Completed)
            {
                lastExitCode_ = 2;
                input_.printTo(TerminalChannel::StdErr, "pipeline error: async command not supported\n");
                return;
            }

            if (!isLast)
            {
                pipedStdout = stageOutput.buffer();
            }
            else if (writeToFile)
            {
                if (fileSystem_ == nullptr)
                {
                    lastExitCode_ = 2;
                    input_.printTo(TerminalChannel::StdErr, "redirection error: filesystem not configured\n");
                    return;
                }

                const char *mode = appendRedirect ? FILE_MODE_APPEND : FILE_MODE_WRITE;
                ETFile file = fileSystem_->open(redirectOutPath, mode, true);
                if (!file.isOpen())
                {
                    lastExitCode_ = 2;
                    input_.printTo(TerminalChannel::StdErr, "redirection error: failed to open target\n");
                    return;
                }

                if (!file.writeAll(stageOutput.buffer()))
                {
                    file.close();
                    lastExitCode_ = 2;
                    input_.printTo(TerminalChannel::StdErr, "redirection error: failed to write target\n");
                    return;
                }

                file.close();
            }
        }
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
