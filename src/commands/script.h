#ifndef SCRIPT_H
#define SCRIPT_H

#include "ScriptRunner.h"
#include "Terminal.h"

namespace EmbeddedTerminal
{
    namespace cmd
    {
        class script : public ICommand
        {
        public:
            explicit script(Terminal &terminal) : terminal_(terminal)
            {
            }

            ETString usage(const ETString &keyword) override;
            CommandResult execute(CommandInvocation &invocation) override;
            void onInterrupt() override;

        protected:
            ETString trigger(const ETString &keyword, const ETString &additional) override;

        private:
            bool startScript_(const ETString &arguments, ETString &errorMessage);
            bool startScriptFile_(const ETString &path, ETString &errorMessage);
            void tick_();
            ETString substitute_(const ETString &input, const ETString &variableName, const ETString &value) const;

            Terminal &terminal_;
            ScriptRunner runner_{
                [this](const ParsedCommand &command)
                {
                    activeParsedCommand_ = command;
                    lastDispatchResult_ = terminal_.executeParsedCommandForScript(command);

                    bool singleCommand = (command.keywords.size() == 1) &&
                                         command.redirectOutPath.empty() &&
                                         command.redirectInPath.empty();
                    if (singleCommand && lastDispatchResult_.state != CommandExecutionState::Completed)
                    {
                        ETString keyword = command.keywords[0];
                        keyword.trim();
                        const auto &commands = terminal_.getCommands();
                        auto it = commands.find(keyword);
                        if (it != commands.end())
                        {
                            activeSubcommand_ = it->second;
                            activeKeyword_ = keyword;
                            activeArguments_ = command.arguments[0];
                            activeSubcommandState_ = lastDispatchResult_.state;
                            hasActiveSubcommand_ = true;
                        }
                    }
                },
                [this]()
                {
                    return terminal_.getLastExitCode();
                },
                [this]()
                {
                    return hasActiveSubcommand_;
                },
                [this]()
                {
                    return terminal_.currentTimeMs();
                },
                [this](const ETString &input, const ETString &variableName, const ETString &value)
                {
                    return substitute_(input, variableName, value);
                }};
            bool hasStarted_ = false;
            bool hasActiveSubcommand_ = false;
            ICommand *activeSubcommand_ = nullptr;
            ETString activeKeyword_;
            ETString activeArguments_;
            ParsedCommand activeParsedCommand_;
            CommandExecutionState activeSubcommandState_ = CommandExecutionState::Completed;
            CommandResult lastDispatchResult_ = CommandResult::completed(0);
        };
    }
}

#endif // SCRIPT_H
