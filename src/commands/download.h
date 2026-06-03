#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include "DirectoryNavigator.h"
#include "interfaces/ICommand.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {

        class download : public ICommand
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
                INVALID_INVOKE = 7,
                INVALID_INVOKE_EXPECTED_RESUME = 8
            };
            download(DirectoryNavigator &dir) : dir_(dir)
            {
            }
            ETString usage(const ETString &keyword) const override;
            CommandResult invoke(CommandInvocation &invocation) override;
            CommandResult resume(CommandInvocation &invocation) override;
            ETVector<ETString> getSuggestions(const ETString &partial) const override;

        protected:
            static constexpr const char *SESSION_KEY_PATH = "download__path";
            static constexpr const char *SESSION_KEY_POS = "download__pos";

        private:
            struct State
            {
                Path path;
                size_t position = 0;
                bool initialized = false;
            };
            struct processChunkResult
            {
                bool hasMore = false;
                bool error = false;
                size_t errorCode = 0;

                processChunkResult(bool hasMore, bool error = false, size_t errorCode = 0) : hasMore(hasMore), error(error), errorCode(errorCode) {}
            };

            static State initState_(CommandInvocation &invocation);
            static State getState_(CommandInvocation &invocation);
            ErrorCode checkState_(const State &state, CommandInvocation &invocation);
            CommandResult processDownload(CommandInvocation &invocation, const State &state);
            CommandResult error_(size_t errorCode, CommandInvocation &invocation);
            CommandResult success_(CommandInvocation &invocation);
            void reset(CommandInvocation &invocation);

            DirectoryNavigator dir_;
        };
    };
};
#endif // DOWNLOAD_H
