#ifndef EMBEDDEDTERMINAL_IEXECUTIONCONTEXT_H
#define EMBEDDEDTERMINAL_IEXECUTIONCONTEXT_H
#include "interfaces/ICommandRuntime.h"
#include "interfaces/IFileSystem.h"
#include "interfaces/ITerminalStream.h"
#include "interfaces/ICommand.h"
#include "scripting/Parser.h"

namespace EmbeddedTerminal
{
    // Minimal interface required by exec pipeline to interact with the host runtime.
    class IExecutionContext
    {
    public:
        virtual ~IExecutionContext() = default;

        virtual IFileSystem *getFileSystem() const = 0;
        virtual IInputChannel *getInputStream() = 0;
        virtual IOutputChannel *getOutputStream() = 0;
        virtual IOutputChannel *getErrorStream() = 0;
        virtual void setInputStream(IInputChannel *input) = 0;
        virtual void setOutputStream(IOutputChannel *output) = 0;
        virtual void setErrorStream(IOutputChannel *error) = 0;
        virtual StreamBundle getStreamBundle() = 0;
        virtual void setStreamBundle(StreamBundle &&bundle) = 0;

        // Function definitions owned by the execution context. Scripts can define functions
        // during execution; the context stores and exposes them to the Runner.
        virtual bool existsFunction(const ETString &keyword) const = 0;
        virtual const Scripting::FunctionDefinitionExpression *getFunction(const ETString &keyword) const = 0;
        virtual const ETMap<TokenText, Scripting::FunctionDefinitionExpression> &getFunctions() const = 0;
        virtual void registerFunction(const TokenText &keyword, const Scripting::FunctionDefinitionExpression &def) = 0;

        virtual bool existsCommand(const ETString &keyword) const = 0;
        virtual ICommand *getCommand(const ETString &keyword) const = 0;
        virtual const ETMap<ETString, ICommand *> &getCommands() const = 0;

        virtual int32_t getLastExitCode() const = 0;
        virtual void setLastExitCode(int32_t code) = 0;

        // Inform the host about running command lifecycle for cooperative scripts
        virtual void setRunningCommandStateForScript(CommandExecutionState state) = 0;
        virtual void resetRunningCommandForScript() = 0;
        virtual void interruptActiveCommand() = 0;

        virtual bool isActiveCommand(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments) const = 0;
        virtual CommandResult resumeCommandForScript(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments) = 0;
        virtual void setRunningCommandForScript(ICommand *const command, const ETString &keyword, const ETVector<ETString> &arguments) = 0;
    };

}
#endif // EMBEDDEDTERMINAL_IEXECUTIONCONTEXT_H
