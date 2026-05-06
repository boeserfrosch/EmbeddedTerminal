#include "script.h"
#include "OptionParser.h"

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
    return keyword + " <path> - Execute script from file\n";
}

CommandResult cmd::script::execute(CommandInvocation &invocation)
{

    if (!hasStarted_)
    {
        ETString errorMessage;
        if (!startScript_(invocation.arguments, errorMessage))
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
    OptionParser parser;
    return usage(keyword);
}

bool cmd::script::startScript_(const ETString &arguments, ETString &errorMessage)
{
    OptionParser parser;
    parser.addOption("-f", "--file", "Path to script file", true);
    parser.addOptionalRemainingArgument("file");

    auto parseResult = parser.parse(arguments);
    if (!parseResult.success)
    {
        errorMessage = "script error: " + parseResult.errorMessage;
        return false;
    }

    ETString scriptPath;
    auto fileOpt = parseResult.options.find("--file");
    if (fileOpt != parseResult.options.end() && !fileOpt->second.empty())
    {
        scriptPath = fileOpt->second[0];
    }
    else
    {
        auto positionalFile = parseResult.options.find("file");
        if (positionalFile != parseResult.options.end() && !positionalFile->second.empty())
        {
            scriptPath = positionalFile->second[0];
        }
    }

    if (scriptPath.empty())
    {
        errorMessage = "script error: Missing required argument: file";
        return false;
    }

    return startScriptFile_(scriptPath, errorMessage);
}

bool cmd::script::startScriptFile_(const ETString &path, ETString &errorMessage)
{
    IFileSystem *fileSystem = terminal_.getFileSystem();
    if (fileSystem == nullptr)
    {
        errorMessage = "script error: filesystem not configured";
        return false;
    }

    ETFile scriptFile = fileSystem->open(path, FILE_MODE_READ, false);
    if (!scriptFile.isOpen())
    {
        errorMessage = "script error: failed to open script file: " + path;
        return false;
    }

    ETString content = scriptFile.readAll();
    scriptFile.close();

    if (content.empty())
    {
        errorMessage = "script error: script file is empty: " + path;
        return false;
    }

    if (!runner_.enqueueScript(content, errorMessage))
    {
        errorMessage = errorMessage + " [file: " + path + "]";
        return false;
    }
    return true;
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
