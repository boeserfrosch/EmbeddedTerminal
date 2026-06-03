#ifndef XXD_H
#define XXD_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"
#include "interfaces/IAutoCompleter.h"
#include "DefaultAutoCompleters.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class xxd : public ICommand
        {

        public:
            enum ErrorCode
            {
                NONE = 0,
                INVALID_OPTIONS = 1,
                INVALID_PATH = 2,
                FILE_NOT_FOUND = 3,
                IS_DIRECTORY = 4,
                FAILED_TO_OPEN_FILE = 5,
                FAILED_TO_SEEK = 6,
                FAILED_TO_READ = 7,
                FAILED_TO_ALLOCATE_BUFFER = 8
            };

            xxd(DirectoryNavigator &dir) : dir_(dir)
            {
            }
            ETString usage(const ETString &keyword) const override;
            CommandResult invoke(CommandInvocation &invocation) override;
            CommandResult resume(CommandInvocation &invocation) override;

            // Auto completion - suggest files and directories
            ETVector<ETString> getSuggestions(const ETString &partial) const override;

        private:
            struct HandleKeyStrokesResult
            {
                bool hasChanges = false;
                bool exitCommand = false;
            };

            HandleKeyStrokesResult handleKeyStrokes_(CommandInvocation &invocation);
            ErrorCode checkFile_(ETFile &file);

            CommandResult error_(ErrorCode errorCode, CommandInvocation &invocation);
            void reset(CommandInvocation &invocation);
            CommandResult streamHexDump_(CommandInvocation &invocation);

            static constexpr const char *SESSION_KEY_PATH = "xxd__path";
            static constexpr const char *SESSION_KEY_POS = "xxd__pos";
            static constexpr size_t CHUNK_SIZE = 256; // Number of bytes to read and display in each chunk, we can enhance this later to make it configurable or at least a central constant

            ETString pendingOffset_;
            bool collectingOffset_ = false;
            unsigned char *buffer = nullptr;

            EmbeddedTerminal::DirectoryNavigator dir_;
        };
    };
};
#endif // XXD_H
