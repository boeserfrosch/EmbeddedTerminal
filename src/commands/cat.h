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
        class cat : public ICommand
        {

        public:
            cat(DirectoryNavigator &dir) : _dir(dir), _completer(dir)
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

            CatState _handleState(CommandInvocation &invocation);
            unsigned char _checkState(const CatState &state, CommandInvocation &invocation);
            bool _processChunk(ETFile &file, size_t filePos, CommandInvocation &invocation);

            static constexpr size_t CHUNK_SIZE = 512;
            static constexpr const char *SESSION_KEY_PATH = "__cat_path";
            static constexpr const char *SESSION_KEY_POS = "__cat_pos";

            ETString readFileForTrigger(const ETString &additional);
            CommandResult executeStream(CommandInvocation &invocation);
            EmbeddedTerminal::DirectoryNavigator &_dir;
            FilePathCompleter _completer;
        };
    };
};
#endif // CAT_H
