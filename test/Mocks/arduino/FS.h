#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

using uint8_t = std::uint8_t;

namespace EmbeddedTerminalMock
{
    struct MockFSEntry
    {
        bool isDirectory = false;
        std::string content;
    };

    struct MockFSState
    {
        std::map<std::string, MockFSEntry> entries{{"/", {true, ""}}};
    };

    inline std::string normalizePath(const std::string &input)
    {
        if (input.empty())
            return "/";

        std::string path = input;
        std::replace(path.begin(), path.end(), '\\', '/');
        if (path.front() != '/')
            path = "/" + path;
        while (path.size() > 1 && path.back() == '/')
            path.pop_back();
        return path;
    }

    inline bool isDirectChild(const std::string &dir, const std::string &candidate)
    {
        if (dir == "/")
        {
            if (candidate.size() <= 1 || candidate.front() != '/')
                return false;
            return candidate.find('/', 1) == std::string::npos;
        }

        const std::string prefix = dir + "/";
        if (candidate.rfind(prefix, 0) != 0)
            return false;
        const auto rest = candidate.substr(prefix.size());
        return !rest.empty() && rest.find('/') == std::string::npos;
    }
}

class File
{
public:
    File() = default;

    explicit File(std::shared_ptr<EmbeddedTerminalMock::MockFSState> state,
                  const std::string &path,
                  bool isDirectory,
                  bool readable,
                  bool writable,
                  bool append = false)
        : state_(std::move(state)),
          path_(EmbeddedTerminalMock::normalizePath(path)),
          readable_(readable),
          writable_(writable),
          isDirectory_(isDirectory),
          isOpen_(state_ != nullptr)
    {
        if (append && state_ && !isDirectory_)
        {
            auto it = state_->entries.find(path_);
            if (it != state_->entries.end())
            {
                position_ = it->second.content.size();
            }
        }
    }

    explicit operator bool() const { return isOpen_; }

    int read()
    {
        if (!isOpen_ || !readable_ || isDirectory_)
            return -1;
        auto it = state_->entries.find(path_);
        if (it == state_->entries.end())
            return -1;
        if (position_ >= it->second.content.size())
            return -1;
        return static_cast<unsigned char>(it->second.content[position_++]);
    }

    size_t read(uint8_t *buffer, size_t size)
    {
        if (!isOpen_ || !readable_ || isDirectory_)
            return 0;
        auto it = state_->entries.find(path_);
        if (it == state_->entries.end())
            return 0;

        const size_t availableBytes = it->second.content.size() > position_ ? it->second.content.size() - position_ : 0;
        const size_t toRead = std::min(size, availableBytes);
        for (size_t index = 0; index < toRead; ++index)
        {
            buffer[index] = static_cast<uint8_t>(it->second.content[position_ + index]);
        }
        position_ += toRead;
        return toRead;
    }

    size_t write(uint8_t value)
    {
        return write(&value, 1);
    }

    size_t write(const uint8_t *buffer, size_t size)
    {
        if (!isOpen_ || !writable_ || isDirectory_)
            return 0;
        auto &entry = state_->entries[path_];
        if (entry.isDirectory)
            return 0;

        if (position_ > entry.content.size())
            entry.content.resize(position_, '\0');
        if (position_ + size > entry.content.size())
            entry.content.resize(position_ + size, '\0');

        for (size_t index = 0; index < size; ++index)
        {
            entry.content[position_ + index] = static_cast<char>(buffer[index]);
        }
        position_ += size;
        return size;
    }

    size_t print(const char *text)
    {
        if (!text)
            return 0;
        return write(reinterpret_cast<const uint8_t *>(text), std::string(text).size());
    }

    bool seek(size_t position)
    {
        if (!isOpen_ || isDirectory_)
            return false;
        auto it = state_->entries.find(path_);
        if (it == state_->entries.end())
            return false;
        if (position > it->second.content.size())
            return false;
        position_ = position;
        return true;
    }

    size_t position() const { return position_; }

    size_t size() const
    {
        if (!isOpen_ || isDirectory_)
            return 0;
        auto it = state_->entries.find(path_);
        if (it == state_->entries.end())
            return 0;
        return it->second.content.size();
    }

    bool available() const
    {
        if (!isOpen_ || isDirectory_)
            return false;
        return position_ < size();
    }

    void close() { isOpen_ = false; }

    bool isDirectory() const { return isOpen_ && isDirectory_; }

    File openNextFile()
    {
        if (!isDirectory())
            return File();
        ensureChildren_();
        if (childCursor_ >= children_.size())
            return File();

        const auto childPath = children_[childCursor_++];
        auto it = state_->entries.find(childPath);
        if (it == state_->entries.end())
            return File();
        return File(state_, childPath, it->second.isDirectory, !it->second.isDirectory, !it->second.isDirectory);
    }

    std::string getNextFileName()
    {
        if (!isDirectory())
            return "";
        ensureChildren_();
        if (nameCursor_ >= children_.size())
            return "";
        return children_[nameCursor_++];
    }

private:
    void ensureChildren_()
    {
        if (childrenBuilt_)
            return;

        childrenBuilt_ = true;
        for (const auto &item : state_->entries)
        {
            if (EmbeddedTerminalMock::isDirectChild(path_, item.first))
            {
                children_.push_back(item.first);
            }
        }
    }

private:
    std::shared_ptr<EmbeddedTerminalMock::MockFSState> state_;
    std::string path_;
    size_t position_ = 0;
    bool readable_ = false;
    bool writable_ = false;
    bool isDirectory_ = false;
    bool isOpen_ = false;

    bool childrenBuilt_ = false;
    std::vector<std::string> children_;
    size_t childCursor_ = 0;
    size_t nameCursor_ = 0;
};

class FS
{
public:
    FS() : state_(std::make_shared<EmbeddedTerminalMock::MockFSState>()) {}

    File open(const char *path, const char *mode = "r")
    {
        const std::string normalized = EmbeddedTerminalMock::normalizePath(path ? path : "");
        const std::string openMode = mode ? mode : "r";

        auto it = state_->entries.find(normalized);
        if (it != state_->entries.end() && it->second.isDirectory)
        {
            return File(state_, normalized, true, false, false);
        }

        if (openMode == "r")
        {
            if (it == state_->entries.end() || it->second.isDirectory)
                return File();
            return File(state_, normalized, false, true, false);
        }

        if (openMode == "r+")
        {
            if (it == state_->entries.end() || it->second.isDirectory)
                return File();
            return File(state_, normalized, false, true, true);
        }

        if (openMode == "w")
        {
            state_->entries[normalized] = {false, ""};
            return File(state_, normalized, false, true, true);
        }

        if (openMode == "a")
        {
            auto &entry = state_->entries[normalized];
            entry.isDirectory = false;
            return File(state_, normalized, false, true, true, true);
        }

        return File();
    }

    bool exists(const char *path)
    {
        return state_->entries.find(EmbeddedTerminalMock::normalizePath(path ? path : "")) != state_->entries.end();
    }

    bool remove(const char *path)
    {
        const std::string normalized = EmbeddedTerminalMock::normalizePath(path ? path : "");
        auto it = state_->entries.find(normalized);
        if (it == state_->entries.end() || it->second.isDirectory)
            return false;
        state_->entries.erase(it);
        return true;
    }

    bool mkdir(const char *path)
    {
        const std::string normalized = EmbeddedTerminalMock::normalizePath(path ? path : "");
        if (state_->entries.find(normalized) != state_->entries.end())
            return false;
        state_->entries[normalized] = {true, ""};
        return true;
    }

    bool rmdir(const char *path)
    {
        const std::string normalized = EmbeddedTerminalMock::normalizePath(path ? path : "");
        auto it = state_->entries.find(normalized);
        if (it == state_->entries.end() || !it->second.isDirectory)
            return false;

        for (const auto &item : state_->entries)
        {
            if (EmbeddedTerminalMock::isDirectChild(normalized, item.first))
                return false;
        }

        state_->entries.erase(it);
        return true;
    }

    void clear()
    {
        state_->entries.clear();
        state_->entries["/"] = {true, ""};
    }

private:
    std::shared_ptr<EmbeddedTerminalMock::MockFSState> state_;
};
