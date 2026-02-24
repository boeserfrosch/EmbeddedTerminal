#ifndef TERMINAL_H
#define TERMINAL_H

#include "ETTypes.h"
#include "interfaces/ITerminalStream.h"
#include "interfaces/ICommand.h"
#include "interfaces/ICommandRuntime.h"
#include "interfaces/IAutoCompleter.h"
#include "BuiltinCommandFlags.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include "hal/ArduinoStream.h"
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

        void loop();
        const ETMap<ETString, ICommand *> &getCommands() const;
        int getLastExitCode() const;

        // Auto completion support
        const ETString &getBuffer() const;
        ETString getLastWord() const;

    protected:
        void call(const ETString &keyword, const ETString &additional);

    private:
        ETMap<ETString, ICommand *> _observer;
        ITerminalStream &_input;
#if defined(ARDUINO)
        ArduinoStream *_ownedStream;
#endif

        // Constants
        static constexpr size_t BUFFER_RESERVE_SIZE = 256;

        ETString buffer;
        ETString lineDelimiter = "\n";
        int _lastExitCode = 0;
        ETMap<ETString, ETString> _sessionVariables;

        // Auto completion helper
        void _handleAutoCompletion();
    };
}
#endif // TERMINAL_H