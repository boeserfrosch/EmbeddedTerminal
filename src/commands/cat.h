#ifndef CAT_H
#define CAT_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"
#include "interfaces/ICommandRuntime.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class cat : public ICommand
        {
        public:
            enum ErrorCode
            {
                NONE = 0,
                INVALID_PATH = 1,
                FILE_NOT_FOUND = 2,
                IS_DIRECTORY = 3,
                FAILED_TO_OPEN_FILE = 4,
                FAILED_TO_SEEK = 5,
                FAILED_TO_READ = 6,
            };

            cat(DirectoryNavigator &dir) : dir_(dir), completer_(dir)
            {
            }
            ETString usage(const ETString &keyword) const override;
            CommandResult invoke(CommandInvocation &invocation) override;
            CommandResult resume(CommandInvocation &invocation) override;

            // Auto completion - suggest files and directories
            ETVector<ETString> getSuggestions(const ETString &partial) const override;

        private:
            ErrorCode checkFilePath_(CommandInvocation &invocation);
            CommandResult processExecution_(CommandInvocation &invocation);
            void reset(CommandInvocation &invocation);

            static constexpr size_t CHUNK_SIZE = 512;
            static constexpr const char *SESSION_KEY_PATH = "cat__path";
            static constexpr const char *SESSION_KEY_POS = "cat__pos";

            // ETString readFileForTrigger(const ETString &additional);
            // CommandResult executeStream(CommandInvocation &invocation);
            EmbeddedTerminal::DirectoryNavigator dir_;
            FilePathCompleter completer_;
        };
    };
};
#endif // CAT_H
