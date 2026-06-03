#ifndef SCRIPT_H
#define SCRIPT_H

#include "interfaces/IExecutionContext.h"
#include "scripting/Runner.h"
#include <memory>

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class script : public ICommand
        {
        public:
            enum ErrorCode
            {
                None = 0,
                InvalidArguments,
                FileNotFound,
                FilesystemNotAvailable,
                FileError,
                ParseError,
                RuntimeError
            };

            script(IExecutionContext &terminal) : runner_(terminal), terminal_(terminal)
            {
            }

            ETString usage(const ETString &keyword) const override;
            CommandResult invoke(CommandInvocation &invocation) override;
            CommandResult resume(CommandInvocation &invocation) override;
            void onInterrupt() override;

            bool error() const
            {
                return error_ != ErrorCode::None;
            }

        protected:
            // ETString trigger(const ETString &keyword, const ETString &additional) override;

            ETString getErrorMessage_(ErrorCode errorCode) const;
            void tryFetchAndParseScript(CommandInvocation &invocation);
            ETString tryFetchScriptPathFromArguments(CommandInvocation &invocation);
            void tryFetchScriptFromPath(const ETString &path, CommandInvocation &invocation);
            void tryTokenizeAndParseScript(CommandInvocation &invocation);
            void reset();
            CommandResult executeRunner_(CommandInvocation &invocation);

        private:
            ETString scriptContent_;
            Scripting::ExpressionChain scriptAst_;
            ETMap<ETString, ETString> runnerVariables_;
            EmbeddedTerminal::Scripting::Runner runner_;
            bool scriptLoaded_ = false;

            IExecutionContext &terminal_;
            ErrorCode error_ = ErrorCode::None;
        };

    }
}

#endif // SCRIPT_H
