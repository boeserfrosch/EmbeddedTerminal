#ifndef TAIL_H
#define TAIL_H

#include "DirectoryNavigator.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"
#include "Terminal.h"
namespace EmbeddedTerminal
{

    namespace cmd
    {
        namespace errorCodes
        {
            enum TailCmdErrorCode
            {
                TAIL_CMD_ERROR_NONE = 0,
                TAIL_CMD_ERROR_INVALID_OPTIONS = 1,
                TAIL_CMD_ERROR_INVALID_NUMBER_OF_LINES = 2,
                TAIL_CMD_ERROR_FILE_NOT_FOUND = 3,
                TAIL_CMD_ERROR_IS_DIRECTORY = 4,
                TAIL_CMD_ERROR_FAILED_TO_OPEN_FILE = 5,
                TAIL_CMD_ERROR_FAILED_TO_SEEK = 6,
                TAIL_CMD_ERROR_FAILED_TO_READ = 7
            };

        } // namespace errorCodes

        class tail : public ICommand
        {

        public:
            tail(DirectoryNavigator &dir) : _dir(dir), _completer(dir)
            {
            }
            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;

            // Streaming execution to support large files without blocking
            CommandResult execute(CommandInvocation &invocation) override;

            // Auto completion - suggest file paths to tail
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            enum class TailState
            {
                Initial,
                FindingStartPosition,
                Streaming
            };

            TailState _getState(CommandInvocation &invocation);

            CommandResult _parseOptions(CommandInvocation &invocation);
            CommandResult _findStartPosition(CommandInvocation &invocation);
            CommandResult _streamFile(CommandInvocation &invocation);

            CommandResult _error(size_t errorCode, CommandInvocation &invocation);
            CommandResult _success(CommandInvocation &invocation);

            DirectoryNavigator &_dir;
            FilePathCompleter _completer;

        protected:
            const char *SESSION_KEY_PATH = "__tail_path";
            const char *SESSION_KEY_POS = "__tail_pos";
            const char *SESSION_KEY_STATE = "__tail_state";
            const char *SESSION_KEY_LINES_TO_FIND = "__tail_lines_to_find";
            const char *SESSION_KEY_LINES_FOUND = "__tail_lines_found";
        };
    };
};
#endif // TAIL_H
