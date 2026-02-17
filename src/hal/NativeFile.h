#ifndef NATIVE_FILE_H
#define NATIVE_FILE_H

#if __cplusplus >= 201703L

#include "interfaces/IFile.h"
#include <fstream>
#include <filesystem>
#include <vector>

namespace EmbeddedTerminal
{
    class NativeFile : public IFile
    {
    private:
        ETString _path;
        ETString _name;
        bool _directory;
        mutable std::fstream _stream;

    public:
        NativeFile(const ETString &path, const char *mode = "r", bool create = false)
            : _path(path), _name(std::filesystem::path(path.c_str()).filename().string()), _directory(false)
        {
            namespace fs = std::filesystem;
            fs::path p(path.c_str());

            if (fs::exists(p) && fs::is_directory(p))
            {
                _directory = true;
                return;
            }

            std::ios::openmode openMode = std::ios::binary;
            if (mode[0] == 'r')
                openMode |= std::ios::in;
            if (mode[0] == 'w')
                openMode |= std::ios::out | std::ios::trunc;
            if (mode[0] == 'a')
                openMode |= std::ios::out | std::ios::app;

            if (create && (mode[0] == 'w' || mode[0] == 'a'))
            {
                fs::create_directories(p.parent_path());
            }

            _stream.open(path, openMode);
        }

        ~NativeFile()
        {
            close();
        }

        int read() override
        {
            if (!_stream.is_open() || !_stream.good())
                return -1;
            char c;
            _stream.read(&c, 1);
            if (_stream.gcount() == 0)
                return -1;
            return static_cast<unsigned char>(c);
        }

        size_t read(void *buffer, size_t size) override
        {
            if (!_stream.is_open() || !_stream.good())
                return 0;
            _stream.read(static_cast<char *>(buffer), size);
            return static_cast<size_t>(_stream.gcount());
        }

        size_t write(unsigned char c) override
        {
            if (!_stream.is_open() || !_stream.good())
                return 0;
            _stream.write(reinterpret_cast<const char *>(&c), 1);
            return _stream.good() ? 1 : 0;
        }

        size_t write(const void *buffer, size_t size) override
        {
            if (!_stream.is_open() || !_stream.good())
                return 0;
            _stream.write(static_cast<const char *>(buffer), size);
            return _stream.good() ? size : 0;
        }

        ETString readString() override
        {
            return readAll();
        }

        ETString readAll() override
        {
            if (!_stream.is_open())
                return "";

            std::stringstream ss;
            ss << _stream.rdbuf();
            return ss.str();
        }

        bool writeAll(const ETString &content) override
        {
            if (!_stream.is_open())
                return false;
            _stream.write(content.c_str(), static_cast<std::streamsize>(content.length()));
            return _stream.good();
        }

        bool seek(size_t position) override
        {
            if (!_stream.is_open())
                return false;
            _stream.seekg(static_cast<std::streamoff>(position));
            _stream.seekp(static_cast<std::streamoff>(position));
            return _stream.good();
        }

        size_t position() const override
        {
            if (!_stream.is_open())
                return 0;
            return static_cast<size_t>(_stream.tellg());
        }

        size_t size() const override
        {
            namespace fs = std::filesystem;
            if (_directory)
                return 0;
            if (!fs::exists(_path.c_str()))
                return 0;
            return static_cast<size_t>(fs::file_size(_path.c_str()));
        }

        void close() override
        {
            if (_stream.is_open())
                _stream.close();
        }

        bool isOpen() const override
        {
            return _stream.is_open();
        }

        bool isDirectory() const override
        {
            namespace fs = std::filesystem;
            return fs::is_directory(_path.c_str());
        }

        ETString name() const override
        {
            return _name;
        }

        ETString path() const override
        {
            return _path;
        }
    };
}
#endif
#endif