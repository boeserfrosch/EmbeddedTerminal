#ifndef TERMINAL_H
#define TERMINAL_H

#include "ETTypes.h"
#include "interfaces/ITerminalStream.h"
#include "interfaces/ICommand.h"
#include "interfaces/ICommandRuntime.h"
#include "interfaces/IAutoCompleter.h"
#include "interfaces/IFileSystem.h"
#include "BuiltinCommandFlags.h"
#include "Lexer.h"
#include "TerminalAst.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include "hal/arduino/ArduinoStream.h"
#endif

namespace EmbeddedTerminal
{
    // Forward declarations
    class DirectoryNavigator;
    class IFileSystem;
    class INetworkInterface;

    class Terminal
    {
    public:
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
        void registerCommand(const ETString &keyword, ICommand *observer);
        void deregisterCommand(const ETString &keyword);
        void setFileSystem(IFileSystem *fileSystem);

        void loop();
        const ETMap<ETString, ICommand *> &getCommands() const;
        int getLastExitCode() const;

        // Auto completion support
        const ETString &getBuffer() const;
        ETString getLastWord() const;

    protected:
        void call(const ETString &keyword, const ETString &additional);

    private:
        CommandResult executeCommandInternal_(ICommand *command, const ETString &keyword, const ETString &arguments,
                                              IInputChannel &stdinChannel, IOutputChannel &stdoutChannel, IOutputChannel &stderrChannel);
        void executeCommand_(ICommand *command, const ETString &keyword, const ETString &arguments);
        void executePipeline_(const ETVector<ETString> &keywords, const ETVector<ETString> &arguments,
                              const ETString &redirectOutPath, bool appendRedirect, const ETString &redirectInPath);
        void continueActiveCommandIfNeeded_();
        bool ingestInputAndHandleAutoCompletion_();
        void processBufferedCommands_();
        void processBufferedLine_(const ETString &line);
        void executeParsedCommand_(const ParsedCommand &command);

        void reportLexerError_(LexerError error);

        ETMap<ETString, ICommand *> observer_;
        ITerminalStream &input_;
        IFileSystem *fileSystem_ = nullptr;
#if defined(ARDUINO)
        ArduinoStream *ownedStream_ = nullptr;
#endif

        // Constants
        static constexpr size_t BUFFER_RESERVE_SIZE = 256;

        ETString buffer;
        ETString lineDelimiter = "\n";
        int lastExitCode_ = 0;
        ETMap<ETString, ETString> sessionVariables_;

        bool hasActiveCommand_ = false;
        ICommand *activeCommand_ = nullptr;
        ETString activeKeyword_;
        ETString activeArguments_;
        CommandExecutionState activeState_ = CommandExecutionState::Completed;

        // Auto completion helper
        void handleAutoCompletion_();
    };
}
#endif // TERMINAL_H