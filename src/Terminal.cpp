#include "Terminal.h"
// #include "TerminalParser.h"
// #include "TerminalExecutor.h"
// #include "scripting/Parser.h"
#include "scripting/Runner.h"
#include "lang/TokenUtils.h"
#include <algorithm>
#include <memory>

#if defined(ARDUINO)
#include <Arduino.h>
#elif defined(ESP_PLATFORM) || defined(ESP_32)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
#include <chrono>
#endif

namespace EmbeddedTerminal
{
    using namespace Lang;

    namespace
    {
        TerminalPredefinedResultCodes runnerErrorToPredefinedResultCodes(Scripting::Runner::ErrorCode error)
        {
            switch (error)
            {
            case Scripting::Runner::ErrorCode::None:
                return TerminalPredefinedResultCodes::SUCCESS;
            case Scripting::Runner::ErrorCode::EmptyCondition:
                return TerminalPredefinedResultCodes::EXECUTION_ERROR;
            case Scripting::Runner::ErrorCode::UndefinedVariable:
                return TerminalPredefinedResultCodes::UNDEFINED_VARIABLE;
            case Scripting::Runner::ErrorCode::UnterminatedStringLiteral:
                return TerminalPredefinedResultCodes::PARSER_ERROR;
            case Scripting::Runner::ErrorCode::CommandNotFound:
                return TerminalPredefinedResultCodes::COMMAND_NOT_FOUND;
            default:
                return TerminalPredefinedResultCodes::EXECUTION_ERROR;
            }
        }
        // TerminalPredefinedResultCodes TerminalExecutorErrorCodeToPredefinedCode(TerminalExecutor::ErrorCode errorCode)
        // {
        //     switch (errorCode)
        //     {
        //     case TerminalExecutor::ErrorCode::None:
        //         return TerminalPredefinedResultCodes::SUCCESS;
        //     case TerminalExecutor::ErrorCode::CommandNotFound:
        //         return TerminalPredefinedResultCodes::COMMAND_NOT_FOUND;
        //     case TerminalExecutor::ErrorCode::UndefinedVariable:
        //         return TerminalPredefinedResultCodes::UNDEFINED_VARIABLE;
        //     case TerminalExecutor::ErrorCode::ExecutionError:
        //         return TerminalPredefinedResultCodes::EXECUTION_ERROR;
        //     case TerminalExecutor::ErrorCode::CompoundOperatorNotSupported:
        //         return TerminalPredefinedResultCodes::COMPOUND_OPERATOR_NOT_SUPPORTED;
        //     case TerminalExecutor::ErrorCode::PipelineInvalid:
        //         return TerminalPredefinedResultCodes::PIPELINE_INVALID;
        //     case TerminalExecutor::ErrorCode::InvalidCommandKeyword:
        //         return TerminalPredefinedResultCodes::INVALID_COMMAND_KEYWORD;
        //     default:
        //         return TerminalPredefinedResultCodes::EXECUTION_ERROR;
        //     }
        // }

        struct ShellCommandLine
        {
            ETVector<ETString> keywords;
            ETVector<ETString> arguments;
            ETString redirectOutPath;
            bool appendRedirect = false;
            ETString redirectInPath;
        };

        ETString tokenToShellText(const token_t &token)
        {
            if (token.type == TokenType::VARIABLE)
            {
                return "$" + token.text;
            }

            return token.text;
        }

        ETVector<ETString> tokensToArguments_(const token_list_t &tokens)
        {
            ETVector<ETString> result;
            for (const auto &token : tokens)
            {
                if (token.type == TokenType::END_OF_FILE || token.type == TokenType::NEWLINE)
                {
                    continue;
                }

                result.push_back(tokenToShellText(token));
            }
            return result;
        }

    }
    class StreamInputChannel : public IInputChannel
    {
    public:
        StreamInputChannel(ITerminalStream &stream) : stream_(stream)
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
        BufferedInputChannel(const ETString &buffer) : buffer_(buffer), consumed_(false)
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
        defaultInputChannel_ = std::make_unique<StreamInputChannel>(input_);
        defaultOutputChannel_ = std::make_unique<StreamOutputChannel>(input_, TerminalChannel::StdOut);
        defaultErrorChannel_ = std::make_unique<StreamOutputChannel>(input_, TerminalChannel::StdErr);
        currentInputChannel_ = defaultInputChannel_.get();
        currentOutputChannel_ = defaultOutputChannel_.get();
        currentErrorChannel_ = defaultErrorChannel_.get();
        buffer.reserve(BUFFER_RESERVE_SIZE);
    }

#if defined(ARDUINO)
    Terminal::Terminal(Stream &stream)
        : input_(*(ownedStream_ = new ArduinoStream(stream)))
    {
        defaultInputChannel_ = std::make_unique<StreamInputChannel>(input_);
        defaultOutputChannel_ = std::make_unique<StreamOutputChannel>(input_, TerminalChannel::StdOut);
        defaultErrorChannel_ = std::make_unique<StreamOutputChannel>(input_, TerminalChannel::StdErr);
        currentInputChannel_ = defaultInputChannel_.get();
        currentOutputChannel_ = defaultOutputChannel_.get();
        currentErrorChannel_ = defaultErrorChannel_.get();
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
        continueActiveCommandIfNeeded_();
        continueActiveScriptIfNeeded_();
        ingestInput_();
        if (!state_.hasRunningCommand && activeScriptRunner_ == nullptr)
        {
            processBufferedCommands_();
        }
    }

    bool Terminal::error()
    {
        return error_ != ErrorCode::None;
    }

    bool Terminal::existsFunction(const ETString &keyword) const
    {
        return functions_.find(keyword) != functions_.end();
    }

    const ETVector<TokenText> Terminal::getFunctionNames() const
    {
        ETVector<TokenText> names;
        for (const auto &pair : functions_)
        {
            names.push_back(pair.first);
        }
        return names;
    }

    const ETMap<TokenText, Scripting::FunctionDefinitionExpression> &Terminal::getFunctions() const
    {
        return functions_;
    }

    const Scripting::FunctionDefinitionExpression *Terminal::getFunction(const ETString &keyword) const
    {
        auto it = functions_.find(TokenText(keyword));
        return (it != functions_.end()) ? &(it->second) : nullptr;
    }

    void Terminal::registerFunction(const TokenText &keyword, const Scripting::FunctionDefinitionExpression &def)
    {
        functions_[keyword] = def;
    }

    bool Terminal::existsCommand(const ETString &keyword) const
    {
        return commands_.find(keyword) != commands_.end();
    }

    ICommand *Terminal::getCommand(const ETString &keyword) const
    {
        auto it = commands_.find(keyword);
        return (it != commands_.end()) ? it->second : nullptr;
    }

    void Terminal::setFileSystem(IFileSystem *fileSystem)
    {
        fileSystem_ = fileSystem;
    }

    void Terminal::continueActiveCommandIfNeeded_()
    {
        if (!state_.hasRunningCommand || state_.activeCommand == nullptr)
        {
            return;
        }

        if (state_.activeCommandState == CommandExecutionState::WaitingForInput && !input_.available())
        {
            return;
        }

        executeCommand_(state_.activeCommand, state_.activeCommandKeyword, state_.activeCommandArguments, true);
    }

    void Terminal::continueActiveScriptIfNeeded_()
    {
        if (activeScriptRunner_ == nullptr || activeScriptAst_ == nullptr)
        {
            return;
        }

        auto state = activeScriptRunner_->execute(*activeScriptAst_, sessionVariables_);
        if (activeScriptRunner_->error())
        {
            error_ = ErrorCode::RunnerError;
            lastExitCode_ = runnerErrorToPredefinedResultCodes(activeScriptRunner_->getLastError());
            if (lastExitCode_ == TerminalPredefinedResultCodes::COMMAND_NOT_FOUND)
            {
                input_.printTo(TerminalChannel::StdErr, "command not found\n");
            }
            else
            {
                input_.printTo(TerminalChannel::StdErr, "script execution error\n");
            }
            activeScriptRunner_.reset();
            activeScriptAst_.reset();
            resetRunningCommandInState_();
            buffer = "";
            return;
        }

        if (state == Scripting::Runner::State::Running)
        {
            state_.hasRunningCommand = true;
            state_.activeCommandState = CommandExecutionState::Running;
            return;
        }

        activeScriptRunner_.reset();
        activeScriptAst_.reset();
        resetRunningCommandInState_();
    }

    bool Terminal::ingestInput_()
    {
        if (!input_.available())
        {
            return false;
        }

        ETString rawInputLine = input_.readAll();
        ETString inputLine;

        bool result = handleCommandKeys_(rawInputLine);
        return result;
    }

    bool Terminal::handleCommandKeys_(const ETString &rawInputLine)
    {
        auto insertCharacterAtCursor = [&](char ch)
        {
            exitHistoryNavigation_();
            if (inputCursor_ == inputLine_.length())
            {
                inputLine_ += ch;
                ++inputCursor_;
                input_.print(ETString(ch));
                return;
            }

            size_t previousCursor = inputCursor_;
            ETString updatedLine = inputLine_;
            updatedLine.insert(inputCursor_, ch);
            redrawCurrentInputLine_(previousCursor, inputLine_, updatedLine, previousCursor + 1);
        };

        size_t index = 0;
        while (index < rawInputLine.length())
        {
            unsigned char current = static_cast<unsigned char>(rawInputLine[index]);

            if (current == 0x03) // Ctrl+C
            {
                interruptActiveExecution_();
                clearCurrentInputLine_();
                input_.printTo(TerminalChannel::StdErr, "^C\n");
                return false; // Signal that an interrupt was requested
            }

            if (current == '\r' || current == '\n')
            {
                commitCurrentInputLine_();
                if (current == '\r' && index + 1 < rawInputLine.length() && rawInputLine[index + 1] == '\n')
                {
                    ++index;
                }
                ++index;
                continue;
            }

            if (current == '\t')
            {
                if (!isCursorInsideQuotes_(inputCursor_) && inputCursor_ == inputLine_.length())
                {
                    exitHistoryNavigation_();
                    handleAutoCompletion_();
                }
                else
                {
                    insertCharacterAtCursor('\t');
                }
                ++index;
                continue;
            }

            if (current == 0x1b && index + 2 < rawInputLine.length() && rawInputLine[index + 1] == '[')
            {
                char key = rawInputLine[index + 2];
                switch (key)
                {
                case 'A':
                    recallHistory_(true);
                    break;
                case 'B':
                    recallHistory_(false);
                    break;
                case 'C':
                    if (inputCursor_ < inputLine_.length())
                    {
                        redrawCurrentInputLine_(inputCursor_, inputLine_, inputLine_, inputCursor_ + 1);
                    }
                    break;
                case 'D':
                    if (inputCursor_ > 0)
                    {
                        redrawCurrentInputLine_(inputCursor_, inputLine_, inputLine_, inputCursor_ - 1);
                    }
                    break;
                default:
                    break;
                }

                index += 3;
                continue;
            }

            if (current == 0x7f || current == 0x08)
            {
                if (inputCursor_ > 0)
                {
                    exitHistoryNavigation_();
                    size_t previousCursor = inputCursor_;
                    ETString updatedLine = inputLine_;
                    updatedLine.erase(inputCursor_ - 1, 1);
                    redrawCurrentInputLine_(previousCursor, inputLine_, updatedLine, previousCursor - 1);
                }
                ++index;
                continue;
            }

            if (current >= ' ')
            {
                insertCharacterAtCursor(static_cast<char>(current));
                ++index;
                continue;
            }

            ++index;
        }

        return true; // Signal that input was ingested and should be processed
    }

    bool hasCommandsToExecute(const token_list_t &tokens)
    {
        auto startIndex = getNextTokenIndexNotOfType(tokens, TokenType::NEWLINE, 0);
        if (startIndex == ETString::npos)
        {
            return false;
        }
        auto nexlineIndex = getNextTokenIndex(tokens, TokenType::NEWLINE, startIndex);
        return nexlineIndex != ETString::npos; // If there is a newline after the start index, it means we have at least one complete command to execute
    }

    token_list_t getNextTokensToExecute(const token_list_t &tokens)
    {
        auto startIndex = getNextTokenIndexNotOfType(tokens, TokenType::NEWLINE, 0);
        if (startIndex == ETString::npos)
        {
            return token_list_t(); // No non-newline tokens, return empty list
        }
        auto newlineIndex = getNextTokenIndex(tokens, TokenType::NEWLINE, startIndex);
        if (newlineIndex == ETString::npos)
        {
            return token_list_t(); // No complete command (no newline), return empty list
        }
        return token_list_t(tokens.begin() + startIndex, tokens.begin() + newlineIndex); // Return the tokens for the next command to execute (excluding leading newlines and the newline at the end)
    }

    void Terminal::processBufferedCommands_()
    {
        state_.hasPendingBufferedCommand = false;
        token_list_t tokens = Lexer().tokenize(buffer);
        // Looking for newlines which than trigger command processing - this allows us to treat the buffer as a stream of input that may contain multiple commands separated by newlines, and process each command as soon as it is complete (when a newline is entered)
        // Jump over any leading newlines tokens, to find the start of the next command. This allows us to handle cases where multiple newlines are entered, or when the buffer starts with a newline (e.g. after processing a command, we might end up with a buffer that starts with a newline followed by the next command).
        if (!hasCommandsToExecute(tokens))
        {
            return;
        }
        // auto startIndex = getNextTokenIndexNotOfType(tokens, TokenType::NEWLINE, 0);
        // auto index = getNextTokenIndex(tokens, TokenType::NEWLINE, startIndex);
        // if (index == ETString::npos)
        // {
        //     return; // No complete command yet (no newline), wait for more input
        // }

        token_list_t commandTokens = getNextTokensToExecute(tokens);
        auto consumeProcessedInput = [&](const token_list_t &processedTokens)
        {
            if (processedTokens.empty())
            {
                return;
            }

            size_t consumedEnd = processedTokens[processedTokens.size() - 1].endIndex;
            if (consumedEnd < buffer.length() && buffer[consumedEnd] == '\n')
            {
                ++consumedEnd;
            }

            if (consumedEnd >= buffer.length())
            {
                buffer = "";
            }
            else
            {
                buffer = buffer.substr(consumedEnd);
            }
        };
        auto parser = Scripting::ScriptParser();
        size_t index = 0;
        auto ast = parser.parse(commandTokens, index);
        if (parser.error())
        {
            if (parser.getError() == Scripting::ScriptParser::ErrorCode::UnexpectedEndOfInput)
            {
                // There where a unneccesary newline at the end of the command, we should replace it with an semicolon to avoid getting stuck with a parsing error on the same command in the buffer, which would prevent us from processing any further commands in the buffer after it.
                // Otherwise we would reparse the same command over and over again!
                auto charPosToRemove = commandTokens.empty() ? 0 : commandTokens[commandTokens.size() - 1].endIndex;
                if (charPosToRemove < buffer.length() && buffer[charPosToRemove] == '\n')
                {
                    buffer.replace(charPosToRemove, 1, ";");
                }
                if (hasCommandsToExecute(Lexer().tokenize(buffer)))
                {
                    return processBufferedCommands_(); // After replacing the unneccesary newline with a space, we might have a valid command in the buffer that we can process immediately, so we check again if there are commands to execute in the buffer and if so, we call processBufferedCommands_() recursively to attempt to process the next command in the buffer. This allows us to recover from the parser error caused by the unneccesary newline and continue processing any further commands in the buffer after it.
                }
                return;
            }
            error_ = ErrorCode::ParserError;
            return;
        }

        // No parsing error, we assume the command is valid and can be processed. So we can remove the part of the buffer that corresponds to this command (up to and including the newline after it)
        // First execute, then remove from buffer

        activeScriptRunner_ = std::make_unique<Scripting::Runner>(*this);
        activeScriptAst_ = std::make_unique<Scripting::ExpressionChain>(ast);
        Scripting::Runner::State state = activeScriptRunner_->execute(*activeScriptAst_, sessionVariables_);

        if (activeScriptRunner_->error())
        {
            error_ = ErrorCode::RunnerError;
            lastExitCode_ = runnerErrorToPredefinedResultCodes(activeScriptRunner_->getLastError());
            if (lastExitCode_ == TerminalPredefinedResultCodes::COMMAND_NOT_FOUND)
            {
                input_.printTo(TerminalChannel::StdErr, "command not found\n");
            }
            else
            {
                input_.printTo(TerminalChannel::StdErr, "script execution error\n");
            }
            buffer = ""; // Clear buffer on error to avoid getting stuck with an unprocessable command in the buffer. We do this after attempting to execute the command, so that char* pointers in tokens remain valid during execution.
            activeScriptRunner_.reset();
            activeScriptAst_.reset();
        }
        else
        {
            // Remove from buffer
            // If our last command token is the same as the last token in the buffer, it means there is nothing after the command we just processed, so we can clear the buffer. Otherwise, we slice the buffer to remove the part corresponding to the command we just processed, and keep anything after it for the next processing cycle(s).
            consumeProcessedInput(commandTokens); // Remove the processed command and trailing newline from the buffer, keeping any remaining commands for subsequent processing.
        }
        if (state == Scripting::Runner::State::Running)
        {
            state_.hasRunningCommand = true;
            state_.activeCommandState = CommandExecutionState::Running;
        }
        else
        {
            activeScriptRunner_.reset();
            activeScriptAst_.reset();
        }

        state_.hasPendingBufferedCommand = hasCommandsToExecute(Lexer().tokenize(buffer)); // If there is another newline after the remaining start index, it means there is another complete command in the buffer that we can process immediately, so we set hasPendingBufferedCommand to true to trigger another processing cycle in the same loop iteration. If there is no newline, it means we have an incomplete command in the buffer, so we set hasPendingBufferedCommand to false and wait for more input to complete the command.
        return;
    }

    void Terminal::interruptActiveExecution_()
    {
        if (state_.hasRunningCommand && state_.activeCommand != nullptr)
        {
            state_.activeCommand->onInterrupt();
        }

        resetRunningCommandInState_();
        resetRunningScriptInState_();
    }

    void Terminal::resetRunningCommandInState_()
    {
        state_.hasRunningCommand = false;
        state_.activeCommandState = CommandExecutionState::Completed;
        state_.activeCommand = nullptr;
        state_.activeCommandKeyword = "";
        state_.activeCommandArguments = ETVector<ETString>();
    }

    void Terminal::resetRunningScriptInState_()
    {
        state_.hasPendingBufferedCommand = false;
    }

    void Terminal::interruptActiveCommand()
    {
        if (state_.hasRunningCommand && state_.activeCommand != nullptr)
        {
            state_.activeCommand->onInterrupt();
        }
        resetRunningCommandInState_();
        resetRunningScriptInState_();
    }

    void Terminal::setRunningCommandInState_(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments)
    {
        state_.hasRunningCommand = true;
        state_.activeCommand = command;
        state_.activeCommandKeyword = keyword;
        state_.activeCommandArguments = arguments;
    }

    bool Terminal::isActiveCommand(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments) const
    {
        return (state_.hasRunningCommand && state_.activeCommand == command && state_.activeCommandKeyword == keyword && state_.activeCommandArguments == arguments);
    }

    void Terminal::setLastExitCode(int32_t code)
    {
        lastExitCode_ = code;
    }

    uint64_t Terminal::nowMs_() const
    {
#if defined(ARDUINO)
        return static_cast<uint64_t>(millis());
#elif defined(ESP_PLATFORM) || defined(ESP_32)
        return static_cast<uint64_t>(xTaskGetTickCount()) * static_cast<uint64_t>(portTICK_PERIOD_MS);
#else
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
#endif
    }

    void Terminal::call(const ETString &keyword, const ETString &additional)
    {
        // Deprecated: this method is only used for the "run" command to call a command with a raw command line string, which is then tokenized and parsed as if it was entered directly into the terminal. This is a bit of a hack and ideally we would want to refactor the "run" command to use the same internal execution logic as the rest of the terminal, so that we can avoid having this separate code path that tokenizes and parses a raw command line string. But for now, we will keep this method as is and just use it for the "run" command.
        Lexer lexer;
        token_list_t arguments = lexer.tokenize(additional);

        call(keyword, tokensToArguments_(arguments));
    }

    void Terminal::call(const ETString &keyword, const ETVector<ETString> &arguments)
    {
        ETString trimmedKeyword = keyword;
        trimmedKeyword.trim();

        if (trimmedKeyword.empty())
            return;
        auto search = commands_.find(trimmedKeyword);
        if (search != commands_.end())
        {
            executeCommand_(search->second, trimmedKeyword, arguments);
        }
        else
        {
            lastExitCode_ = TerminalPredefinedResultCodes::COMMAND_NOT_FOUND;
            input_.printfTo(TerminalChannel::StdErr, "%s is unknown!\n", trimmedKeyword.c_str());
        }
    }

    int32_t Terminal::getLastExitCode() const
    {
        return lastExitCode_;
    }

    IFileSystem *Terminal::getFileSystem() const
    {
        return fileSystem_;
    }

    void Terminal::setVariable(const ETString &key, const ETString &value)
    {
        sessionVariables_[key] = value;
    }

    void Terminal::clearVariables()
    {
        sessionVariables_.clear();
    }

    uint64_t Terminal::currentTimeMs() const
    {
        return nowMs_();
    }

    const ETString &Terminal::getBuffer() const
    {
        return inputLine_;
    }

    void Terminal::clearCurrentInputLine_()
    {
        inputLine_ = "";
        inputCursor_ = 0;
        historyIndex_ = ETString::npos;
        historyDraft_ = "";
    }

    void Terminal::exitHistoryNavigation_()
    {
        historyIndex_ = ETString::npos;
        historyDraft_ = "";
    }

    void Terminal::commitCurrentInputLine_()
    {
        if (!inputLine_.empty() && (commandHistory_.empty() || commandHistory_.back() != inputLine_))
        {
            commandHistory_.push_back(inputLine_);
        }

        buffer += inputLine_;
        buffer += "\n";
        input_.print("\n");
        clearCurrentInputLine_();
    }

    void Terminal::redrawCurrentInputLine_(size_t previousCursor, const ETString &oldLine, const ETString &newLine, size_t newCursor)
    {
        ETString output;
        for (size_t i = 0; i < previousCursor; ++i)
        {
            output += '\b';
        }
        output += newLine;

        size_t clearCount = oldLine.length() > newLine.length() ? oldLine.length() - newLine.length() : 0;
        for (size_t i = 0; i < clearCount; ++i)
        {
            output += ' ';
        }

        size_t targetTail = oldLine.length() > newLine.length() ? oldLine.length() : newLine.length();
        if (targetTail > newCursor)
        {
            for (size_t i = 0; i < targetTail - newCursor; ++i)
            {
                output += '\b';
            }
        }

        input_.print(output);

        inputLine_ = newLine;
        inputCursor_ = newCursor;
    }

    void Terminal::recallHistory_(bool previous)
    {
        if (commandHistory_.empty())
        {
            return;
        }

        ETString nextLine;

        if (historyIndex_ == ETString::npos)
        {
            if (!previous)
            {
                return;
            }

            historyDraft_ = inputLine_;
            historyIndex_ = commandHistory_.size() - 1;
            nextLine = commandHistory_[historyIndex_];
        }
        else if (previous)
        {
            if (historyIndex_ > 0)
            {
                --historyIndex_;
            }
            nextLine = commandHistory_[historyIndex_];
        }
        else if (historyIndex_ + 1 < commandHistory_.size())
        {
            ++historyIndex_;
            nextLine = commandHistory_[historyIndex_];
        }
        else
        {
            nextLine = historyDraft_;
            historyIndex_ = ETString::npos;
        }

        redrawCurrentInputLine_(inputCursor_, inputLine_, nextLine, nextLine.length());
    }

    bool Terminal::isCursorInsideQuotes_(size_t cursor) const
    {
        bool inSingleQuote = false;
        bool inDoubleQuote = false;
        bool escaped = false;

        if (cursor > inputLine_.length())
        {
            cursor = inputLine_.length();
        }

        for (size_t i = 0; i < cursor; ++i)
        {
            char ch = inputLine_[i];

            if (escaped)
            {
                escaped = false;
                continue;
            }

            if (ch == '\\')
            {
                escaped = true;
                continue;
            }

            if (ch == '\'' && !inDoubleQuote)
            {
                inSingleQuote = !inSingleQuote;
                continue;
            }

            if (ch == '"' && !inSingleQuote)
            {
                inDoubleQuote = !inDoubleQuote;
            }
        }

        return inSingleQuote || inDoubleQuote;
    }

    CommandResult Terminal::executeCommandInternal_(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments,
                                                    IInputChannel &input, IOutputChannel &output, IOutputChannel &error)
    {
        if (command == nullptr)
        {
            lastExitCode_ = TerminalPredefinedResultCodes::COMMAND_NOT_FOUND;
            return CommandResult::completed(TerminalPredefinedResultCodes::COMMAND_NOT_FOUND);
        }

        CommandContext context(sessionVariables_, lastExitCode_, true);
        CommandInvocation invocation{keyword, arguments, context, input, output, error};
        CommandResult result = command->invoke(invocation);
        lastExitCode_ = result.exitCode;
        return result;
    }

    void Terminal::executeCommand_(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments, bool resume)
    {
        if (command == nullptr)
        {
            lastExitCode_ = TerminalPredefinedResultCodes::COMMAND_NOT_FOUND;
            return;
        }

        StreamInputChannel input(input_);
        StreamOutputChannel output(input_, TerminalChannel::StdOut);
        StreamOutputChannel error(input_, TerminalChannel::StdErr);

        CommandResult result;
        if (resume)
        {
            result = resumeCommandForScript(command, keyword, arguments);
        }
        else
        {
            result = executeCommandInternal_(command, keyword, arguments, input, output, error);
        }

        if (result.state == CommandExecutionState::Completed)
        {
            resetRunningCommandInState_();
            return;
        }

        setRunningCommandInState_(command, keyword, arguments);
        state_.activeCommandState = result.state;
    }

    CommandResult Terminal::resumeCommandForScript(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments)
    {
        if (command == nullptr)
        {
            lastExitCode_ = TerminalPredefinedResultCodes::COMMAND_NOT_FOUND;
            return CommandResult::completed(TerminalPredefinedResultCodes::COMMAND_NOT_FOUND);
        }

        StreamInputChannel input(input_);
        StreamOutputChannel output(input_, TerminalChannel::StdOut);
        StreamOutputChannel error(input_, TerminalChannel::StdErr);
        CommandContext context(sessionVariables_, lastExitCode_, true);
        CommandInvocation invocation{keyword, arguments, context, input, output, error};
        CommandResult result = command->resume(invocation);
        lastExitCode_ = result.exitCode;
        return result;
    }

    IInputChannel *Terminal::getInputStream()
    {
        return currentInputChannel_ != nullptr ? currentInputChannel_ : defaultInputChannel_.get();
    }

    IOutputChannel *Terminal::getOutputStream()
    {
        return currentOutputChannel_ != nullptr ? currentOutputChannel_ : defaultOutputChannel_.get();
    }

    IOutputChannel *Terminal::getErrorStream()
    {
        return currentErrorChannel_ != nullptr ? currentErrorChannel_ : defaultErrorChannel_.get();
    }

    void Terminal::setInputStream(IInputChannel *input)
    {
        currentInputChannel_ = input;
    }

    void Terminal::setOutputStream(IOutputChannel *output)
    {
        currentOutputChannel_ = output;
    }

    void Terminal::setErrorStream(IOutputChannel *error)
    {
        currentErrorChannel_ = error;
    }

    StreamBundle Terminal::getStreamBundle()
    {
        return StreamBundle(*getInputStream(), *getOutputStream(), *getErrorStream());
    }

    void Terminal::setStreamBundle(StreamBundle &&bundle)
    {
        activeStreamBundle_ = std::make_unique<StreamBundle>(bundle);
        currentInputChannel_ = activeStreamBundle_->input.get();
        currentOutputChannel_ = activeStreamBundle_->output.get();
        currentErrorChannel_ = activeStreamBundle_->error.get();
    }

    void Terminal::setRunningCommandForScript(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments)
    {
        setRunningCommandInState_(command, keyword, arguments);
    }

    void Terminal::resetRunningCommandForScript()
    {
        resetRunningCommandInState_();
    }

    void Terminal::setRunningCommandStateForScript(CommandExecutionState state)
    {
        state_.activeCommandState = state;
    }

    void Terminal::executePipeline_(const token_list_t &keywords, const ETVector<token_list_t> &arguments,
                                    const token_t &redirectOutPath, bool appendRedirect, const token_t &redirectInPath)
    {
        if (keywords.empty() || (keywords.size() != arguments.size()))
        {
            lastExitCode_ = TerminalPredefinedResultCodes::PIPELINE_INVALID;
            input_.printTo(TerminalChannel::StdErr, "pipeline error: invalid pipeline\n");
            return;
        }

        StreamInputChannel streamInput(input_);
        StreamOutputChannel error(input_, TerminalChannel::StdErr);
        ETString pipedStdout;
        ETString redirectedInputContent;

        if (redirectInPath.type != TokenType::NIL)
        {
            if (fileSystem_ == nullptr)
            {
                lastExitCode_ = TerminalPredefinedResultCodes::FILESYSTEM_NOT_CONFIGURED;
                input_.printTo(TerminalChannel::StdErr, "redirection error: filesystem not configured\n");
                return;
            }

            ETFile inputFile = fileSystem_->open(Path(redirectInPath.text), FILE_MODE_READ, false);
            if (!inputFile.isOpen())
            {
                lastExitCode_ = TerminalPredefinedResultCodes::FILESYSTEM_ERROR;
                input_.printTo(TerminalChannel::StdErr, "redirection error: failed to open input\n");
                return;
            }

            redirectedInputContent = inputFile.readAll();
            inputFile.close();
        }

        resetRunningCommandInState_();

        for (size_t index = 0; index < keywords.size(); ++index)
        {
            ETString commandKey = keywords[index].text;
            auto search = commands_.find(commandKey);
            if (search == commands_.end())
            {
                lastExitCode_ = TerminalPredefinedResultCodes::COMMAND_NOT_FOUND;
                input_.printfTo(TerminalChannel::StdErr, "%s is unknown!\n", commandKey.c_str());
                return;
            }

            bool isLast = (index + 1 == keywords.size());
            BufferedInputChannel bufferedInput(pipedStdout);
            BufferedInputChannel redirectedInput(redirectedInputContent);
            BufferedOutputChannel stageOutput;
            StreamOutputChannel finalOutput(input_, TerminalChannel::StdOut);
            ETVector<ETString> stageArguments = tokensToArguments_(arguments[index]);

            IInputChannel &input = (index == 0)
                                       ? (redirectInPath.type == TokenType::NIL ? static_cast<IInputChannel &>(streamInput)
                                                                                : static_cast<IInputChannel &>(redirectedInput))
                                       : static_cast<IInputChannel &>(bufferedInput);
            bool writeToFile = isLast && redirectOutPath.type != TokenType::NIL;
            IOutputChannel &output = (isLast && !writeToFile) ? static_cast<IOutputChannel &>(finalOutput)
                                                              : static_cast<IOutputChannel &>(stageOutput);

            CommandResult result = executeCommandInternal_(search->second, commandKey, stageArguments, input, output, error);

            if (result.state != CommandExecutionState::Completed)
            {
                lastExitCode_ = TerminalPredefinedResultCodes::PIPELINE_ASYNC_NOT_SUPPORTED;
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
                    lastExitCode_ = TerminalPredefinedResultCodes::FILESYSTEM_NOT_CONFIGURED;
                    input_.printTo(TerminalChannel::StdErr, "redirection error: filesystem not configured\n");
                    return;
                }

                const char *mode = appendRedirect ? FILE_MODE_APPEND : FILE_MODE_WRITE;
                ETFile file = fileSystem_->open(Path(redirectOutPath.text), mode, true);
                if (!file.isOpen())
                {
                    lastExitCode_ = TerminalPredefinedResultCodes::REDIRECTION_FAILED;
                    input_.printTo(TerminalChannel::StdErr, "redirection error: failed to open target\n");
                    return;
                }

                if (!file.writeAll(stageOutput.buffer()))
                {
                    file.close();
                    lastExitCode_ = TerminalPredefinedResultCodes::REDIRECTION_FAILED;
                    input_.printTo(TerminalChannel::StdErr, "redirection error: failed to write target\n");
                    return;
                }

                file.close();
            }
        }
    }

    void Terminal::handleAutoCompletion_()
    {
        exitHistoryNavigation_();
        auto parts = split(inputLine_, " ");
        if (parts.empty())
        {
            return;
        }
        // Get the (partially typed) keyword (first word in buffer)
        auto keywordPart = parts[0];

        if (parts.size() == 1)
        {
            // If there is a space at the end of the buffer, treat it as if user has completed the keyword and is now typing the first argument
            if (inputLine_.endsWith(' '))
            {
                handleAutoCompletionOfCommand_(keywordPart, ""); // Re-run auto-completion logic to handle argument suggestions
            }
            // User is still typing the keyword - we will try to auto-complete the keyword itself
            // Get suggestions for the partially typed keyword
            ETVector<ETString> keywordSuggestions;
            for (const auto &entry : commands_)
            {
                if (entry.first.startsWith(keywordPart))
                {
                    keywordSuggestions.push_back(entry.first);
                }
            }
            outputAutoCompletionSuggestions_(keywordPart, keywordSuggestions);
        }

        // Get the partial argument being typed
        ETString partialArg = parts[parts.size() - 1];
        handleAutoCompletionOfCommand_(keywordPart, partialArg);
    }

    void Terminal::handleAutoCompletionOfCommand_(const ETString &keywordPart, const ETString &partialArg)
    {
        // Lookup the command and check if it has auto completion support
        auto search = commands_.find(keywordPart);
        if (search != commands_.end())
        {
            auto cmd = search->second;
            // Call getSuggestions directly - it return s empty vector if not overridden
            ETVector<ETString> suggestions = cmd->getSuggestions(partialArg);

            outputAutoCompletionSuggestions_(partialArg, suggestions);
        }
    }

    void Terminal::outputAutoCompletionSuggestions_(const ETString &partialArg, const ETVector<ETString> &suggestions)
    {
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
                inputLine_ += toAppend;
                inputCursor_ = inputLine_.length();
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
                inputLine_ += toAppend;
                inputCursor_ = inputLine_.length();
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

    void Terminal::registerCommand(const ETString &keyword, ICommand *const command)
    {
        if (command == nullptr)
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
        commands_[trimmedKeyword] = command;
    }

    void Terminal::deregisterCommand(const ETString &keyword)
    {
        ETString trimmedKeyword = keyword;
        trimmedKeyword.trim();
        auto it = commands_.find(trimmedKeyword);
        if (it != commands_.end())
        {
            // Just remove from map - Terminal does NOT own commands
            commands_.erase(it);
        }
    }

    const ETMap<ETString, ICommand *> &Terminal::getCommands() const
    {
        return commands_;
    }
    const ETMap<ETString, ETString> &Terminal::getVariables() const
    {
        return sessionVariables_;
    }

    ETMap<ETString, ETString> &Terminal::getVariables()
    {
        return sessionVariables_;
    }

}