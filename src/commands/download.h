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
            download(DirectoryNavigator &dir) : dir_(dir)
            {
            }
            ETString usage(const ETString &keyword);
            CommandResult execute(CommandInvocation &invocation) override;
            ETString trigger(const ETString &keyword, const ETString &additional) override;
            ETVector<ETString> getSuggestions(const ETString &partial) override;

        protected:
            static constexpr const char *SESSION_KEY_PATH = "download__path";
            static constexpr const char *SESSION_KEY_POS = "download__pos";

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

            downloadState handleState_(CommandInvocation &invocation);
            unsigned char checkState_(const downloadState &state, CommandInvocation &invocation);
            processChunkResult processChunk_(ETFile &file, size_t filePos, CommandInvocation &invocation);
            CommandResult error_(size_t errorCode, CommandInvocation &invocation);
            CommandResult success_(CommandInvocation &invocation);

            DirectoryNavigator dir_;
        };
    };
};
#endif // DOWNLOAD_H
