#ifndef XXD_H
#define XXD_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        namespace errorCodes
        {
            enum XXDCmdErrorCode
            {
                XXD_CMD_ERROR_NONE = 0,
                XXD_CMD_ERROR_INVALID_OPTIONS = 1,
                XXD_CMD_ERROR_INVALID_PATH = 2,
                XXD_CMD_ERROR_FILE_NOT_FOUND = 3,
                XXD_CMD_ERROR_IS_DIRECTORY = 4,
                XXD_CMD_ERROR_FAILED_TO_OPEN_FILE = 5,
                XXD_CMD_ERROR_FAILED_TO_SEEK = 6,
                XXD_CMD_ERROR_FAILED_TO_READ = 7,
                XXD_CMD_ERROR_FAILED_TO_ALLOCATE_BUFFER = 8
            };
        } // namespace errorCodes

        class xxd : public ICommand
        {

        public:
            xxd(DirectoryNavigator &dir) : dir_(dir)
            {
            }
            ETString usage(const ETString &keyword);
            ETString trigger(const ETString &keyword, const ETString &additional) override;
            CommandResult execute(CommandInvocation &invocation) override;

            // Auto completion - suggest files and directories
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        private:
            struct XXDState
            {
                ETString path;
                size_t position = 0;
                bool initialState = true;
            };

            struct HandleKeyStrokesResult
            {
                bool hasChanges = false;
                bool exitCommand = false;
            };

            XXDState handleCommandState_(CommandInvocation &invocation);
            errorCodes::XXDCmdErrorCode checkCommandState_(const XXDState &state, CommandInvocation &invocation);
            HandleKeyStrokesResult handleKeyStrokes_(CommandInvocation &invocation);
            errorCodes::XXDCmdErrorCode checkFile_(ETFile &file);

            ETString generateHexDump_(const char *content, size_t length, size_t startOffset = 0, size_t bytesPerLine = 16);

            CommandResult error_(errorCodes::XXDCmdErrorCode errorCode, CommandInvocation &invocation);
            CommandResult streamHexDump_(CommandInvocation &invocation, const XXDState &state);

            static constexpr const char *SESSION_KEY_PATH = "xxd__path";
            static constexpr const char *SESSION_KEY_POS = "xxd__pos";
            static constexpr size_t CHUNK_SIZE = 256;

            unsigned char *buffer = nullptr;

            EmbeddedTerminal::DirectoryNavigator dir_;
        };
    };
};
#endif // XXD_H
