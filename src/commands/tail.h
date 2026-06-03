#ifndef TAIL_H
#define TAIL_H

#include "DirectoryNavigator.h"
#include "interfaces/IAutoCompleter.h"
#include "interfaces/ICommand.h"
#include "DefaultAutoCompleters.h"
namespace EmbeddedTerminal
{

    namespace cmd
    {

        class tail : public ICommand
        {

        public:
            enum ErrorCode
            {
                NONE = 0,
                INVALID_OPTIONS = 1,
                INVALID_NUMBER_OF_LINES = 2,
                FILE_NOT_FOUND = 3,
                IS_DIRECTORY = 4,
                FAILED_TO_OPEN_FILE = 5,
                FAILED_TO_SEEK = 6,
                FAILED_TO_READ = 7,
                INVALID_RESUME = 8
            };
            tail(DirectoryNavigator &dir) : dir_(dir), completer_(dir)
            {
            }
            ETString usage(const ETString &keyword) const override;

            CommandResult invoke(CommandInvocation &invocation) override;
            CommandResult resume(CommandInvocation &invocation) override;

            // Auto completion - suggest file paths to tail
            ETVector<ETString> getSuggestions(const ETString &partial) const override;

        private:
            enum class TailState
            {
                Initial,
                FindingStartPosition,
                Streaming
            };

            CommandResult parseOptions_(CommandInvocation &invocation);
            CommandResult findStartPosition_(CommandInvocation &invocation);
            CommandResult streamFile_(CommandInvocation &invocation);
            void reset(CommandInvocation &invocation);

            CommandResult error_(size_t errorCode, CommandInvocation &invocation);
            CommandResult success_(CommandInvocation &invocation);

            DirectoryNavigator dir_;
            FilePathCompleter completer_;

        protected:
            size_t linesToFind_ = 10; // Default to last 10 lines, this should be configurable or at least a central constant
            const char *SESSION_KEY_PATH = "tail__path";
            const char *SESSION_KEY_POS = "tail__pos";
        };
    };
};
#endif // TAIL_H
