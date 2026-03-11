#ifndef SCRIPT_RUNNER_H
#define SCRIPT_RUNNER_H

#include "ETTypes.h"
#include "TerminalAst.h"
#include <functional>

namespace EmbeddedTerminal
{
    class ScriptRunner
    {
    public:
        using CommandDispatcher = std::function<void(const ParsedCommand &)>;
        using ExitCodeProvider = std::function<int(void)>;
        using CommandBusyProvider = std::function<bool(void)>;
        using TimeProvider = std::function<uint64_t(void)>;
        using VariableSubstituter = std::function<ETString(const ETString &, const ETString &, const ETString &)>;

        ScriptRunner(CommandDispatcher commandDispatcher,
                     ExitCodeProvider exitCodeProvider,
                     CommandBusyProvider commandBusyProvider,
                     TimeProvider timeProvider,
                     VariableSubstituter substituter);

        bool enqueueScript(const ETString &scriptText, ETString &errorMessage);
        void tick();
        void reset();
        bool isActive() const;

    private:
        enum class IfPhase
        {
            None,
            Condition,
            Branch
        };

        static constexpr size_t kMaxCallDepth_ = 8;

        struct CallFrame_
        {
            ParsedChain chain;
            size_t index = 0;
        };

        bool expandAstToChain_(const ParsedAst &ast, ParsedChain &out) const;
        bool parseDelayMs_(const ParsedCommand &command, uint32_t &outMs) const;
        bool executeChainStep_(const ParsedChain &chain, size_t &index, bool &completed);
        void drainCallStack_();

        CommandDispatcher commandDispatcher_;
        ExitCodeProvider exitCodeProvider_;
        CommandBusyProvider commandBusyProvider_;
        TimeProvider timeProvider_;
        VariableSubstituter substituter_;

        bool hasActiveScript_ = false;
        bool isWhileLoop_ = false;
        bool whileHasLiteralCondition_ = false;
        bool whileLiteralCondition_ = false;
        ParsedChain whileConditionChain_;
        size_t whileConditionIndex_ = 0;
        bool whileInConditionPhase_ = false;

        bool isIfBlock_ = false;
        ETVector<ParsedChain> ifConditionChains_;
        ETVector<size_t> ifConditionIndices_;
        ETVector<ParsedChain> ifBranchChains_;
        ParsedChain ifThenChain_;
        ParsedChain ifElseChain_;
        bool ifHasElseChain_ = false;
        size_t ifCurrentCondition_ = 0;
        IfPhase ifPhase_ = IfPhase::None;

        ETMap<ETString, ParsedChain> functionRegistry_;
        ETVector<CallFrame_> callStack_;

        ParsedChain activeScriptChain_;
        size_t activeScriptIndex_ = 0;
        bool scriptDelayPending_ = false;
        uint64_t scriptResumeAtMs_ = 0;
    };
}

#endif // SCRIPT_RUNNER_H
