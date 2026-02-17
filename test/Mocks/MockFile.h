#ifndef MOCKFILE_H
#define MOCKFILE_H
#include "../../src/interfaces/IFile.h"
#include "../../src/ETTypes.h"
#include <cstring>
#include <algorithm>

class MockFile : public EmbeddedTerminal::IFile
{
public:
    ETString _name;
    ETString _content;
    size_t _size = 0;
    size_t pos = 0;
    bool open = true;
    bool _mockSize = false;

    bool _isDirectory = false;

    MockFile(const ETString &name = "", const ETString &content = "", const size_t size = 0) : _name(name), _content(content), _size(size), pos(0), open(true), _mockSize(size > 0 && size != _content.length())
    {
    }

    virtual ~MockFile() {}

    int read() override
    {
        if (!open || pos >= _content.length())
            return -1;
        return _content[pos++];
    }

    size_t read(void *buffer, size_t size) override
    {
        if (!open)
            return 0;
        size_t toRead = std::min(size, _content.length() - pos);
        memcpy(buffer, _content.c_str() + pos, toRead);
        pos += toRead;
        return toRead;
    }

    size_t write(unsigned char c) override
    {
        if (!open)
            return 0;
        _content += static_cast<char>(c);
        pos = _content.length();
        return 1;
    }

    size_t write(const void *buffer, size_t size) override
    {
        if (!open)
            return 0;

        auto b = static_cast<const char *>(buffer);
        _content += ETString(b);
        pos = _content.length();
        return size;
    }

    ETString readString() override
    {
        return readAll();
    }
    ETString readAll() override
    {
        if (!open)
            return "";
        return _content;
    }
    bool writeAll(const ETString &str) override
    {
        if (!open)
            return false;
        _content = str;
        pos = _content.length();
        return true;
    }
    bool seek(size_t position) override
    {
        if (!open || position > _content.length())
            return false;
        pos = position;
        return true;
    }
    size_t position() const override
    {
        return pos;
    }
    size_t size() const override
    {
        if (_mockSize)
            return _size;
        return _content.length();
    }
    void close() override
    {
        open = false;
    }
    bool isOpen() const override
    {
        return open;
    }
    bool isDirectory() const override
    {
        return _isDirectory;
    }
    ETString name() const override
    {
        return _name;
    }

    ETString path() const override
    {
        return _name;
    }
};

#endif // MOCKFILE_H
