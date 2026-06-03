#ifndef MOCK_TERMINAL_H
#define MOCK_TERMINAL_H

#include "Terminal.h"
#include "../Mocks/MockStream.h"

class MockTerminal : public Terminal
{
public:
    MockStream mockStream;

    MockTerminal() : Terminal(mockStream) {}

    void clearOutput()
    {
        mockStream.outputBuffer = "";
    }

    void clearInput()
    {
        mockStream.inputBuffer = "";
        mockStream.inputPos = 0;
    }

    void clearErrorStream()
    {
        mockStream.stderrBuffer = "";
    }

    const ETString &getOutput() const
    {
        return mockStream.outputBuffer;
    }

    const ETString &getErrorStream() const
    {
        return mockStream.stderrBuffer;
    }

    ETString getInput() const
    {
        return mockStream.inputBuffer;
    }

    void reset()
    {
        clearOutput();
        clearInput();
        clearErrorStream();
        clearVariables();
    }
};

#endif // MOCK_TERMINAL_H
