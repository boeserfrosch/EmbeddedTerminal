#ifndef MOCKCOMMAND_H
#define MOCKCOMMAND_H

#include "../../src/interfaces/ICommand.h"
#include "../../src/ETTypes.h"

class MockCommand : public EmbeddedTerminal::ICommand
{
public:
    ETString lastKeyword;
    ETString lastAdditional;
    ETString usage(const ETString &keyword) override
    {
        lastKeyword = keyword;
        return "Usage: " + keyword;
    }

protected:
    ETString trigger(const ETString &keyword, const ETString &additional) override
    {
        lastKeyword = keyword;
        lastAdditional = additional;
        return "Triggered: " + keyword + " " + additional;
    }
};

#endif // MOCKCOMMAND_H
