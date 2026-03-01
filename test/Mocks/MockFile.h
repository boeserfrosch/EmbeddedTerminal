#ifndef MOCKFILE_H
#define MOCKFILE_H
#include "../../src/interfaces/IFile.h"
#include "../../src/ETTypes.h"
#include <cstring>
#include <algorithm>
#include <functional>
#include <memory>

class MockFile : public EmbeddedTerminal::IFile
{
public:
    ETString content_;
    size_t size_ = 0;
    size_t pos_ = 0;
    bool open_ = false;
    bool mockSize_ = false;

    bool isDirectory_ = false;
    std::shared_ptr<ETString> sharedContent_;
    std::function<void()> onClose_;

    MockFile() : content_(""), size_(0), pos_(0), open_(false), mockSize_(false) {}

    MockFile(const ETString &content, const size_t size = 0) : content_(content), size_(size), pos_(0), open_(true), mockSize_(size > 0 && size != content_.length())
    {
    }

    MockFile(const std::shared_ptr<ETString> &sharedContent, bool open = true)
        : content_(""), size_(0), pos_(0), open_(open), mockSize_(false), sharedContent_(sharedContent)
    {
    }

    virtual ~MockFile()
    {
        close();
    }

    int read() override
    {
        if (!open_ || pos_ >= getContent_().length())
            return -1;
        return getContent_()[pos_++];
    }

    size_t read(void *buffer, size_t size) override
    {
        if (!open_ || buffer == nullptr)
            return 0;
        if (pos_ >= getContent_().length())
            return 0;
        size_t toRead = std::min(size, getContent_().length() - pos_);
        memcpy(buffer, getContent_().c_str() + pos_, toRead);
        pos_ += toRead;
        return toRead;
    }

    size_t write(unsigned char c) override
    {
        if (!open_)
            return 0;
        getMutableContent_() += static_cast<char>(c);
        pos_ = getContent_().length();
        return 1;
    }

    size_t write(const void *buffer, size_t size) override
    {
        if (!open_)
        {
            return 0;
        }

        auto b = static_cast<const char *>(buffer);
        for (size_t i = 0; i < size; ++i)
        {
            getMutableContent_() += b[i];
        }
        pos_ = getContent_().length();
        return size;
    }

    ETString readString() override
    {
        return readAll();
    }
    ETString readAll() override
    {
        if (!open_)
            return "";
        return getContent_();
    }
    bool writeAll(const ETString &str) override
    {
        if (!open_)
            return false;
        getMutableContent_() += str;
        pos_ = getContent_().length();
        return true;
    }
    bool seek(size_t position) override
    {
        if (!open_ || position > getContent_().length())
            return false;
        pos_ = position;
        return true;
    }
    size_t position() const override
    {
        return pos_;
    }
    size_t size() const override
    {
        if (mockSize_)
            return size_;
        return getContent_().length();
    }
    void close() override
    {
        if (!open_)
            return;
        open_ = false;
        if (onClose_)
        {
            onClose_();
            onClose_ = nullptr;
        }
    }
    bool isOpen() const override
    {
        return open_;
    }
    bool isDirectory() const override
    {
        return isDirectory_;
    }

    void setCloseCallback(const std::function<void()> &callback)
    {
        onClose_ = callback;
    }

private:
    ETString &getMutableContent_()
    {
        if (sharedContent_)
        {
            return *sharedContent_;
        }
        return content_;
    }

    const ETString &getContent_() const
    {
        if (sharedContent_)
        {
            return *sharedContent_;
        }
        return content_;
    }
};

#endif // MOCKFILE_H
