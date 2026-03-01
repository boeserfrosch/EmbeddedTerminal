#pragma once

#if defined(ARDUINO)
#include "FS.h"
#include <algorithm>
#include <cstring>
#include <ctime>
#include <map>
#include <memory>
#include <string>

class MockArduinoFileImpl : public fs::FileImpl
{
public:
    MockArduinoFileImpl(const char *path, bool isDirectory = false)
        : path_(path ? path : ""), position_(0), directory_(isDirectory), closed_(false), lastWrite_(time(nullptr))
    {
    }

    size_t write(const uint8_t *buf, size_t size) override
    {
        if (closed_ || directory_)
            return 0;

        if (position_ > content_.size())
            content_.resize(position_, '\0');

        if (position_ + size > content_.size())
            content_.resize(position_ + size);

        memcpy(&content_[position_], buf, size);
        position_ += size;
        lastWrite_ = time(nullptr);
        return size;
    }

    size_t read(uint8_t *buf, size_t size) override
    {
        if (closed_ || directory_ || position_ >= content_.size())
            return 0;

        size_t toRead = std::min(size, content_.size() - position_);
        memcpy(buf, content_.data() + position_, toRead);
        position_ += toRead;
        return toRead;
    }

    void flush() override {}

    bool seek(uint32_t pos, fs::SeekMode mode) override
    {
        if (closed_ || directory_)
            return false;

        size_t newPos = 0;
        if (mode == fs::SeekSet)
            newPos = pos;
        else if (mode == fs::SeekCur)
            newPos = position_ + pos;
        else
            newPos = content_.size() + pos;

        if (newPos > content_.size())
            return false;

        position_ = newPos;
        return true;
    }

    size_t position() const override { return position_; }
    size_t size() const override { return content_.size(); }
    bool setBufferSize(size_t) override { return true; }
    void close() override { closed_ = true; }
    const char *path() const override { return path_.c_str(); }

    const char *name() const override
    {
        size_t pos = path_.find_last_of('/');
        if (pos == std::string::npos)
            return path_.c_str();
        return path_.c_str() + pos + 1;
    }

    time_t getLastWrite() override { return lastWrite_; }
    boolean isDirectory(void) override { return directory_; }

private:
    std::string path_;
    std::string content_;
    size_t position_;
    bool directory_;
    bool closed_;
    time_t lastWrite_;
};

class MockArduinoFSImpl : public fs::FSImpl
{
public:
    fs::FileImplPtr open(const char *path, const char *mode, const bool create) override
    {
        std::string key = path ? path : "";
        auto it = files_.find(key);
        if (it != files_.end())
            return it->second;

        if (!create && (!mode || strcmp(mode, "r") == 0))
            return nullptr;

        auto file = std::make_shared<MockArduinoFileImpl>(key.c_str(), false);
        files_[key] = file;
        return file;
    }

    bool exists(const char *path) override
    {
        return files_.find(path ? path : "") != files_.end();
    }

    bool rename(const char *pathFrom, const char *pathTo) override
    {
        std::string from = pathFrom ? pathFrom : "";
        std::string to = pathTo ? pathTo : "";
        auto it = files_.find(from);
        if (it == files_.end() || files_.find(to) != files_.end())
            return false;

        files_[to] = it->second;
        files_.erase(it);
        return true;
    }

    bool remove(const char *path) override
    {
        return files_.erase(path ? path : "") > 0;
    }

    bool mkdir(const char *path) override
    {
        std::string key = path ? path : "";
        if (files_.find(key) != files_.end())
            return false;

        files_[key] = std::make_shared<MockArduinoFileImpl>(key.c_str(), true);
        return true;
    }

    bool rmdir(const char *path) override
    {
        std::string key = path ? path : "";
        auto it = files_.find(key);
        if (it == files_.end() || !it->second->isDirectory())
            return false;

        files_.erase(it);
        return true;
    }

private:
    std::map<std::string, std::shared_ptr<MockArduinoFileImpl>> files_;
};

inline FS MockArduinoFS = FS(std::make_shared<MockArduinoFSImpl>());

#endif