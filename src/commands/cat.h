#ifndef CAT_H
#define CAT_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"
#include "interfaces/ICommandRuntime.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        namespace errorCodes
        {
            enum CatCmdErrorCode
            {
                CAT_CMD_ERROR_NONE = 0,
                CAT_CMD_ERROR_INVALID_PATH = 1,
                CAT_CMD_ERROR_FILE_NOT_FOUND = 2,
                CAT_CMD_ERROR_IS_DIRECTORY = 3,
                CAT_CMD_ERROR_FAILED_TO_OPEN_FILE = 4,
                CAT_CMD_ERROR_FAILED_TO_SEEK = 5,
                CAT_CMD_ERROR_FAILED_TO_READ = 6
            };
        }

        class cat : public ICommand
        {

        public:
            cat(DirectoryNavigator &dir) : dir_(dir), completer_(dir)
            {
            }
            ETString usage(const ETString &keyword);
            CommandResult execute(CommandInvocation &invocation) override;
            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Auto completion - suggest files and directories
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            struct CatState
            {
                ETString path;
                size_t position = 0;
            };

            CatState handleState_(CommandInvocation &invocation);
            errorCodes::CatCmdErrorCode checkState_(const CatState &state, CommandInvocation &invocation);
            bool processChunk_(ETFile &file, size_t filePos, CommandInvocation &invocation);

            static constexpr size_t CHUNK_SIZE = 512;
            static constexpr const char *SESSION_KEY_PATH = "cat__path";
            static constexpr const char *SESSION_KEY_POS = "cat__pos";

            ETString readFileForTrigger(const ETString &additional);
            CommandResult executeStream(CommandInvocation &invocation);
            EmbeddedTerminal::DirectoryNavigator dir_;
            FilePathCompleter completer_;
        };
    };
};
#endif // CAT_H
