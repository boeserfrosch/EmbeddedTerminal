#ifndef TERMINAL_H
#define TERMINAL_H

#include "ETTypes.h"
#include "lang/LangAPI.h"
#include "scripting/Parser.h"
#include "interfaces/ITerminalStream.h"
#include "interfaces/ICommand.h"
#include "interfaces/ICommandRuntime.h"
#include "interfaces/IAutoCompleter.h"
#include "interfaces/IFileSystem.h"
#include "interfaces/IExecutionContext.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include "hal/arduino/ArduinoStream.h"
#endif

namespace EmbeddedTerminal
{
    using namespace Lang;

    // Forward declarations
    class DirectoryNavigator;
    class IFileSystem;
    class INetworkInterface;
    namespace Scripting
    {
        class Runner;
        struct Expression;
        using ExpressionChain = ETVector<Expression>;
    }

    enum TerminalPredefinedResultCodes
    {
        SUCCESS = 0,
        COMMAND_NOT_FOUND = 127,
        PIPELINE_ASYNC_NOT_SUPPORTED,
        PIPELINE_INVALID,
        FILESYSTEM_NOT_CONFIGURED,
        FILESYSTEM_ERROR,
        REDIRECTION_FAILED,
        LEXER_ERROR,
        INTERUPTED,
        PARSER_ERROR,
        VALIDATION_ERROR,
        EXECUTION_ERROR,
        UNDEFINED_VARIABLE,
        COMPOUND_OPERATOR_NOT_SUPPORTED,
        INVALID_COMMAND_KEYWORD
    };

    struct TerminalState
    {
        bool hasRunningCommand = false;
        bool hasPendingBufferedCommand = false;
        CommandExecutionState activeCommandState = CommandExecutionState::Completed;
        ICommand *activeCommand = nullptr;
        ETString activeCommandKeyword;
        ETVector<ETString> activeCommandArguments;
    };

    class Terminal : public IExecutionContext
    {
        friend class Scripting::Runner;

    public:
        enum ErrorCode
        {
            None = 0,
            CommandNotFound,
            RunnerError,
            ParserError,
        };

        Terminal(ITerminalStream &input);
#if defined(ARDUINO)
        Terminal(Stream &stream);
#endif
        ~Terminal();

        // Delete copy operations (class manages dynamic memory)
        Terminal(const Terminal &) = delete;
        Terminal &operator=(const Terminal &) = delete;

        // Command registration (for custom commands)
        // Note: Terminal does NOT take ownership of ANY commands.
        // Caller must ensure the command object remains valid for the lifetime
        // of the Terminal or until it is deregistered.
        // Caller is responsible for deleting command objects.
        void registerCommand(const ETString &keyword, ICommand *const command);
        void deregisterCommand(const ETString &keyword);
        void setFileSystem(IFileSystem *fileSystem);

        void loop();
        bool error();
        ErrorCode getError() const
        {
            return error_;
        }
        void clearError()
        {
            error_ = ErrorCode::None;
        }

        bool existsFunction(const ETString &keyword) const;
        const ETVector<TokenText> getFunctionNames() const;
        const ETMap<TokenText, Scripting::FunctionDefinitionExpression> &getFunctions() const;
        const Scripting::FunctionDefinitionExpression *getFunction(const ETString &keyword) const;
        void registerFunction(const TokenText &keyword, const Scripting::FunctionDefinitionExpression &def);

        bool existsCommand(const ETString &keyword) const override;
        ICommand *getCommand(const ETString &keyword) const override;
        const ETMap<ETString, ICommand *> &getCommands() const override;
        ETMap<ETString, ETString> &getVariables();
        const ETMap<ETString, ETString> &getVariables() const;
        void setVariable(const ETString &key, const ETString &value);
        void clearVariables();
        int32_t getLastExitCode() const override;
        IFileSystem *getFileSystem() const override;
        IInputChannel *getInputStream() override;
        IOutputChannel *getOutputStream() override;
        IOutputChannel *getErrorStream() override;
        void setInputStream(IInputChannel *input) override;
        void setOutputStream(IOutputChannel *output) override;
        void setErrorStream(IOutputChannel *error) override;
        StreamBundle getStreamBundle() override;
        void setStreamBundle(StreamBundle &&bundle) override;
        void setRunningCommandForScript(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments);
        void resetRunningCommandForScript();
        void setRunningCommandStateForScript(CommandExecutionState state);
        bool isActiveCommand(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments) const;
        void setLastExitCode(int32_t code);
        uint64_t currentTimeMs() const;
        void interruptActiveCommand();
        // CommandResult executeParsedCommandForScript(const ParsedCommand &command);
        CommandResult resumeCommandForScript(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments);

        bool isRunning() const
        {
            return (state_.hasRunningCommand || state_.hasPendingBufferedCommand);
        }

        bool hasRunningExternalCommand() const
        {
            return (state_.hasRunningCommand && state_.activeCommand != nullptr);
        }

        bool injectScriptInput(const ETString &input)
        {
            pendingScriptInput_ += input;
            return true;
        }

        // Auto completion support
        const ETString &getBuffer() const;

    protected:
        void call(const ETString &keyword, const ETString &additional);
        void call(const ETString &keyword, const ETVector<ETString> &arguments);

    protected:
        ErrorCode error_ = ErrorCode::None;
        ETMap<ETString, ETString> sessionVariables_;

    private:
        // void processCommandLine_(const token_list_t &line, const ETString &sourceBuffer);

        void resetRunningCommandInState_();
        void resetRunningScriptInState_();
        void setRunningCommandInState_(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments);

        CommandResult executeCommandInternal_(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments,
                                              IInputChannel &input, IOutputChannel &output, IOutputChannel &error);
        void executeCommand_(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments, bool resume = false);
        void executePipeline_(const token_list_t &keywords, const ETVector<token_list_t> &arguments,
                              const token_t &redirectOutPath, bool appendRedirect, const token_t &redirectInPath);
        void continueActiveCommandIfNeeded_();
        void continueActiveScriptIfNeeded_();
        // bool continueStatementExecution_();
        bool ingestInput_();
        bool handleCommandKeys_(const ETString &rawInputLine);
        void processBufferedCommands_();
        // void processCommandLine_(const ETString &line);

        // void executeParsedCommand_(const ParsedCommand &command);
        // void executeInlineScript_(const ETString &scriptText);
        void interruptActiveExecution_();
        uint64_t nowMs_() const;
        void handleAutoCompletionOfCommand_(const ETString &keywordPart, const ETString &argumentPart);
        void outputAutoCompletionSuggestions_(const ETString &partial, const ETVector<ETString> &suggestions);

        // void reportLexerError_(LexerError error);

        ETMap<ETString, ICommand *> commands_;
        ITerminalStream &input_;
        IFileSystem *fileSystem_ = nullptr;
        std::unique_ptr<IInputChannel> defaultInputChannel_;
        std::unique_ptr<IOutputChannel> defaultOutputChannel_;
        std::unique_ptr<IOutputChannel> defaultErrorChannel_;
        IInputChannel *currentInputChannel_ = nullptr;
        IOutputChannel *currentOutputChannel_ = nullptr;
        IOutputChannel *currentErrorChannel_ = nullptr;
        std::unique_ptr<StreamBundle> activeStreamBundle_;
#if defined(ARDUINO)
        ArduinoStream *ownedStream_ = nullptr;
#endif

        // Constants
        static constexpr size_t BUFFER_RESERVE_SIZE = 256;

        // token_t tokensBuffer_[64];

        ETString buffer;
        ETString pendingScriptInput_;
        ETString lineDelimiter = "\n";
        int32_t lastExitCode_ = 0;

        TerminalState state_;
        std::unique_ptr<Scripting::Runner> activeScriptRunner_;
        std::unique_ptr<Scripting::ExpressionChain> activeScriptAst_;
        ETMap<TokenText, Scripting::FunctionDefinitionExpression> functions_;

        // ScriptRunner inlineScriptRunner_;

        // Auto completion helper
        void handleAutoCompletion_();
    };
}
#endif // TERMINAL_H