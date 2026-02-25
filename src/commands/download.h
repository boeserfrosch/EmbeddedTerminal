#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include "DirectoryNavigator.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        namespace errorCodes
        {
            enum DownloadCmdErrorCode
            {
                DOWNLOAD_CMD_ERROR_NONE = 0,
                DOWNLOAD_CMD_ERROR_INVALID_PATH = 1,
                DOWNLOAD_CMD_ERROR_FILE_NOT_FOUND = 2,
                DOWNLOAD_CMD_ERROR_IS_DIRECTORY = 3,
                DOWNLOAD_CMD_ERROR_FAILED_TO_OPEN_FILE = 4,
                DOWNLOAD_CMD_ERROR_FAILED_TO_SEEK = 5,
                DOWNLOAD_CMD_ERROR_FAILED_TO_READ = 6
            };
        }

        class download : public ICommand
        {

        public:
            download(DirectoryNavigator &dir) : _dir(dir)
            {
            }
            ETString usage(const ETString &keyword);
            CommandResult execute(CommandInvocation &invocation) override;
            ETString trigger(const ETString &keyword, const ETString &additional) override;
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        protected:
            static constexpr const char *SESSION_KEY_PATH = "__download_path";
            static constexpr const char *SESSION_KEY_POS = "__download_pos";

        private:
            struct downloadState
            {
                ETString path;
                size_t position = 0;
            };

            struct processChunkResult
            {
                bool hasMore = false;
                bool error = false;
                size_t errorCode = 0;

                processChunkResult(bool hasMore, bool error = false, size_t errorCode = 0) : hasMore(hasMore), error(error), errorCode(errorCode) {}
            };

            downloadState _handleState(CommandInvocation &invocation);
            unsigned char _checkState(const downloadState &state, CommandInvocation &invocation);
            processChunkResult _processChunk(ETFile &file, size_t filePos, CommandInvocation &invocation);
            CommandResult _error(size_t errorCode, CommandInvocation &invocation);
            CommandResult _success(CommandInvocation &invocation);

            DirectoryNavigator &_dir;
        };
    };
};
#endif // DOWNLOAD_H
