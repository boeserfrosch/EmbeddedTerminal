#include "script.h"

using namespace EmbeddedTerminal;

namespace
{
    ETString replaceAll_(const ETString &input, const ETString &needle, const ETString &replacement)
    {
        if (needle.empty())
        {
            return input;
        }

        ETString result = input;
        size_t pos = 0;
        while ((pos = result.find(needle, pos)) != ETString::npos)
        {
            ETString left = result.substr(0, pos);
            ETString right = result.substr(pos + needle.length());
            result = left + replacement + right;
            pos += replacement.length();
        }

        return result;
    }
}

ETString cmd::script::usage(const ETString &keyword)
{
    return keyword + " <inline-script> - Execute script text (supports for/do/done, ;, &&, ||, delay <ms>)\n" +
           keyword + " -f <path> - Execute script from file\n" +
           keyword + " file <path> - Execute script from file\n";
}

CommandResult cmd::script::execute(CommandInvocation &invocation)
{
    if (!hasStarted_)
    {
        ETString args = invocation.arguments.trim();
        if (args.empty())
        {
            invocation.stderrChannel.print("script error: missing script content\n");
            invocation.stderrChannel.print(usage(invocation.keyword));
            return CommandResult::completed(2);
        }

        ETString errorMessage;
        if (!startScript_(args, errorMessage))
        {
            invocation.stderrChannel.print(errorMessage + "\n");
            return CommandResult::completed(2);
        }

        hasStarted_ = true;
    }

    if (hasActiveSubcommand_)
    {
        if (activeSubcommandState_ == CommandExecutionState::WaitingForInput && !invocation.stdinChannel.available())
        {
            return CommandResult::waitingForInput(terminal_.getLastExitCode());
        }

        lastDispatchResult_ = terminal_.resumeCommandForScript(activeSubcommand_, activeKeyword_, activeArguments_);
        activeSubcommandState_ = lastDispatchResult_.state;
        if (lastDispatchResult_.state != CommandExecutionState::Completed)
        {
            return lastDispatchResult_;
        }

        hasActiveSubcommand_ = false;
        activeSubcommand_ = nullptr;
        activeKeyword_ = "";
        activeArguments_ = "";
        return CommandResult::running(terminal_.getLastExitCode());
    }

    tick_();

    if (hasActiveSubcommand_)
    {
        return lastDispatchResult_;
    }

    if (runner_.isActive())
    {
        return CommandResult::running(terminal_.getLastExitCode());
    }

    hasStarted_ = false;
    return CommandResult::completed(terminal_.getLastExitCode());
}

void cmd::script::onInterrupt()
{
    if (hasActiveSubcommand_ && activeSubcommand_ != nullptr)
    {
        activeSubcommand_->onInterrupt();
    }

    runner_.reset();
    hasStarted_ = false;
    hasActiveSubcommand_ = false;
    activeSubcommand_ = nullptr;
    activeKeyword_ = "";
    activeArguments_ = "";
    activeSubcommandState_ = CommandExecutionState::Completed;
    lastDispatchResult_ = CommandResult::completed(130);
}

ETString cmd::script::trigger(const ETString &keyword, const ETString &additional)
{
    (void)additional;
    return usage(keyword);
}

bool cmd::script::startScript_(const ETString &arguments, ETString &errorMessage)
{
    if (arguments.startsWith("-f "))
    {
        ETString path = arguments.substr(3).trim();
        return startScriptFile_(path, errorMessage);
    }

    if (arguments.startsWith("file "))
    {
        ETString path = arguments.substr(5).trim();
        return startScriptFile_(path, errorMessage);
    }

    return runner_.enqueueScript(arguments, errorMessage);
}

bool cmd::script::startScriptFile_(const ETString &path, ETString &errorMessage)
{
    ETString trimmedPath = path.trim();
    if (trimmedPath.empty())
    {
        errorMessage = "script error: missing script file path";
        return false;
    }

    IFileSystem *fileSystem = terminal_.getFileSystem();
    if (fileSystem == nullptr)
    {
        errorMessage = "script error: filesystem not configured";
        return false;
    }

    ETFile scriptFile = fileSystem->open(trimmedPath, FILE_MODE_READ, false);
    if (!scriptFile.isOpen())
    {
        errorMessage = "script error: failed to open script file";
        return false;
    }

    ETString content = scriptFile.readAll();
    scriptFile.close();
    return runner_.enqueueScript(content, errorMessage);
}

void cmd::script::tick_()
{
    runner_.tick();
}

ETString cmd::script::substitute_(const ETString &input, const ETString &variableName, const ETString &value) const
{
    ETString result = input;
    result = replaceAll_(result, "${" + variableName + "}", value);
    result = replaceAll_(result, "$" + variableName, value);
    return result;
}
