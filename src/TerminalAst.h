#ifndef TERMINAL_AST_H
#define TERMINAL_AST_H

#include "ETTypes.h"

namespace EmbeddedTerminal
{
    enum class ChainCondition
    {
        Always,
        OnSuccess,
        OnFailure
    };

    struct ParsedCommand
    {
        ETVector<ETString> keywords;
        ETVector<ETString> arguments;
        ETString redirectOutPath;
        bool appendRedirect = false;
        ETString redirectInPath;
    };

    struct ParsedChainSegment
    {
        ParsedCommand command;
        ChainCondition condition = ChainCondition::Always;
    };

    struct ParsedChain
    {
        ETVector<ParsedChainSegment> segments;
    };

    struct ParsedForLoop
    {
        ETString variable;
        ETVector<ETString> values;
        ParsedChain body;
    };

    struct ParsedAst
    {
        bool isForLoop = false;
        ParsedChain chain;
        ParsedForLoop forLoop;
    };
}

#endif // TERMINAL_AST_H
