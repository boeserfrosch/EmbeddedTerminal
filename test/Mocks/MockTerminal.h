#ifndef MOCK_TERMINAL_H
#define MOCK_TERMINAL_H

#include "terminal/Terminal.h"
#include "../Mocks/MockStream.h"
#include <string>
#include <map>

class MockTerminal : public Terminal
{
public:
    MockStream mockStream;

    MockTerminal() : Terminal(mockStream) {}

    void clearOutput()
    {
        mockStream.outputBuffer.clear();
    }
};

#endif // MOCK_TERMINAL_H
