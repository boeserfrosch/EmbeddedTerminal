#ifndef NATIVE_FILE_H
#define NATIVE_FILE_H

#if defined(__cplusplus) && __cplusplus >= 201703L

#include "interfaces/IFile.h"
#include <fstream>
#include <filesystem>
#include <vector>
#include "Path.h"

namespace EmbeddedTerminal
{
    class NativeFile : public IFile
    {
    protected:
        mutable std::fstream stream_;
        bool directory_;

    public:
        NativeFile(std::fstream stream, bool isDirectory = false) : stream_(std::move(stream)), directory_(isDirectory)
        {
        }

        ~NativeFile()
        {
            close();
        }

        int read() override
        {
            if (!stream_.is_open() || !stream_.good())
                return -1;
            char c;
            stream_.read(&c, 1);
            if (stream_.gcount() == 0)
                return -1;
            return static_cast<unsigned char>(c);
        }

        size_t read(void *buffer, size_t size) override
        {
            if (!stream_.is_open() || !stream_.good())
                return 0;
            stream_.read(static_cast<char *>(buffer), size);
            return static_cast<size_t>(stream_.gcount());
        }

        size_t write(unsigned char c) override
        {
            if (!stream_.is_open() || !stream_.good())
                return 0;
            stream_.write(reinterpret_cast<const char *>(&c), 1);
            return stream_.good() ? 1 : 0;
        }

        size_t write(const void *buffer, size_t size) override
        {
            if (!stream_.is_open() || !stream_.good())
                return 0;
            stream_.write(static_cast<const char *>(buffer), size);
            return stream_.good() ? size : 0;
        }

        ETString readString() override
        {
            return readAll();
        }

        ETString readAll() override
        {
            if (!stream_.is_open())
                return "";

            std::stringstream ss;
            ss << stream_.rdbuf();
            return ss.str();
        }

        bool writeAll(const ETString &content) override
        {
            if (!stream_.is_open())
                return false;
            stream_.write(content.c_str(), static_cast<std::streamsize>(content.length()));
            return stream_.good();
        }

        bool seek(size_t position) override
        {
            if (!stream_.is_open())
                return false;
            stream_.seekg(static_cast<std::streamoff>(position));
            stream_.seekp(static_cast<std::streamoff>(position));
            return stream_.good();
        }

        size_t position() const override
        {
            if (!stream_.is_open())
                return 0;
            return static_cast<size_t>(stream_.tellg());
        }

        size_t size() const override
        {
            namespace fs = std::filesystem;
            if (isDirectory())
                return 0;
            auto pos = stream_.tellp();
            stream_.seekp(0, std::ios::end);
            size_t fileSize = static_cast<size_t>(stream_.tellp());
            stream_.seekp(pos);
            return fileSize;
        }

        void close() override
        {
            if (stream_.is_open())
                stream_.close();
        }

        bool isOpen() const override
        {
            return stream_.is_open();
        }

        bool isDirectory() const override
        {
            return directory_;
        }
    };
}
#endif
#endif
