#pragma once
#ifndef ET_PATH_H
#define ET_PATH_H

#include "ETTypes.h"
#include <unity.h>

namespace EmbeddedTerminal
{
    class Path
    {
    protected:
        ETVector<ETString> pathParts_;
        bool isAbsolute_ = false;
        mutable ETString cachedString_;
        mutable bool cacheDirty_ = true;

        const ETString &asString_() const
        {
            if (!cacheDirty_)
            {
                return cachedString_;
            }

            if (isRoot())
            {
                cachedString_ = "/";
            }
            else if (pathParts_.empty())
            {
                cachedString_ = isAbsolute_ ? "/" : "";
            }
            else
            {
                cachedString_ = ::join(pathParts_, "/");
                if (isAbsolute_)
                {
                    cachedString_ = "/" + cachedString_;
                }
            }

            cacheDirty_ = false;
            return cachedString_;
        }

        ETString cleanUpStringPath_(const ETString &path) const
        {
            ETString cleaned = path.trim();
            // Replace backslashes with slashes
            for (size_t i = 0; i < cleaned.length(); ++i)
            {
                if (cleaned[i] == '\\')
                    cleaned[i] = '/';
            }
            return cleaned;
        }

        void normalize_()
        {
            ETVector<ETString> normalizedParts;
            for (const auto &part : pathParts_)
            {
                if (part.empty())
                {
                    continue; // Skip empty parts caused by consecutive slashes
                }
                if (part == "." && !normalizedParts.empty())
                {
                    // Skip "." , but only if it's not the first part (to allow for absolute paths)
                    continue;
                }
                // Handle ".." by popping the last part if possible, but only if the last part is not also ".." (to avoid removing valid parent directory references in relative paths)
                else if (part == ".." && !normalizedParts.empty() && (normalizedParts.back() != ".." && normalizedParts.back() != "."))
                {
                    normalizedParts.pop_back();
                }
                else
                {
                    normalizedParts.push_back(part);
                }
            }
            pathParts_ = normalizedParts;
            cacheDirty_ = true;
        }

    public:
        /**
         * @brief A utility class for handling file paths. It can parse a path string into its components, resolve relative paths, and provide utilities for working with paths.
         * The Path class can be used to manipulate file paths in a platform-independent way, handling both absolute and relative paths, and normalizing them by resolving "." and ".." components.
         */
        Path(const ETString &path = "")
        {
            ETString cleanedPath = cleanUpStringPath_(path);
            if (cleanedPath.empty() || cleanedPath == "/")
            {
                isAbsolute_ = true;
                pathParts_.clear(); // Represent root as empty parts
                return;
            }
            isAbsolute_ = (cleanedPath.length() > 0 && cleanedPath[0] == '/');
            pathParts_ = ::split(cleanedPath, "/");
            normalize_();
        }

        Path(const char *path) : Path(ETString(path)) {}

        /**
         * @brief Converts the Path object back to a string representation. The path components will be joined with "/" as the separator.
         * @return The string representation of the path.
         */
        operator ETString() const
        {
            return asString_();
        }

        operator const char *() const
        {
            return c_str();
        }

        bool operator==(const Path &other) const
        {
            return isAbsolute_ == other.isAbsolute_ && pathParts_ == other.pathParts_;
        }

        bool operator==(const char *other) const
        {
            return *this == Path(other);
        }

        bool operator==(const ETString &other) const
        {
            return *this == Path(other);
        }

        bool operator<(const Path &other) const
        {
            if (isAbsolute_ != other.isAbsolute_)
                return isAbsolute_ < other.isAbsolute_; // Absolute paths are considered "less" than relative paths
            return ETString(*this) < ETString(other);
        }

        bool operator!=(const Path &other) const
        {
            return isAbsolute_ != other.isAbsolute_ || ETString(*this) != ETString(other);
        }

        bool operator!=(const char *other) const
        {
            return !(*this == other);
        }

        bool operator!=(const ETString &other) const
        {
            return !(*this == other);
        }

        Path operator+(const char *other) const
        {
            Path p = *this;
            auto otherParts = ::split(ETString(other), "/");
            p.pathParts_ += otherParts;
            p.normalize_();
            return p;
        }

        Path &operator+=(const char *other)
        {
            *this = *this + other;
            cacheDirty_ = true;
            return *this;
        }

        Path operator+(const Path &other) const
        {
            Path result;
            if (other.isAbsolute_)
            {
                result = other;
            }
            else
            {
                result.pathParts_ = pathParts_ + other.pathParts_;
                result.isAbsolute_ = isAbsolute_;
                result.normalize_();
            }
            return result;
        }

        Path &operator+=(const Path &other)
        {
            *this = *this + other;
            cacheDirty_ = true;
            return *this;
        }

        /**
         * @brief Gets the base path (all components except the last one) from the given path string. For example, for "/a/b/c.txt", it will return "/a/b".
         * @return The base path as an ETString. If the input path does not contain any separators, it will return an empty string.
         */
        Path getBasePath() const
        {
            if (pathParts_.empty())
                return Path("");
            Path basePath;
            basePath.pathParts_ = ETVector<ETString>(pathParts_.begin(), pathParts_.end() - 1);
            basePath.isAbsolute_ = isAbsolute_;
            return basePath;
        }

        ETString getName() const
        {
            if (pathParts_.empty())
                return "";
            return pathParts_.back();
        }

        bool isEmpty() const
        {
            return pathParts_.empty();
        }

        bool empty() const
        {
            return isEmpty();
        }

        bool isAbsolute() const
        {
            return isAbsolute_;
        }

        bool isRoot() const
        {
            return isAbsolute_ && pathParts_.empty();
        }

        const char *c_str() const
        {
            return asString_().c_str();
        }

        /**
         * @brief Checks if the current path is a child of the given parent path. A path A is considered a child of path B if A starts with B followed by a path separator. For example, "/a/b/c.txt" is a child of "/a/b", but not a child of "/a/b/c".
         * @param parent The parent path to check against.
         * @return True if the current path is a child of the parent path, false otherwise.
         */
        bool isChildOf(const Path &parent) const
        {
            if (parent.isRoot()) // Special case: everything is a child of root
                return true;

            if (parent.pathParts_.size() > pathParts_.size())
                return false;
            for (size_t i = 0; i < parent.pathParts_.size(); ++i)
            {
                if (parent.pathParts_[i] != pathParts_[i])
                    return false;
            }
            return true;
        }

        /**
         * @brief Checks if the current path is a first-order child of the given parent path. A path A is considered a first-order child of path B if A is a child of B and there are no additional path separators between them. For example, "/a/b/c.txt" is a first-order child of "/a/b", but "/a/b/c/d.txt" is not a first-order child of "/a/b".
         * @param parent The parent path to check against.
         * @return True if the current path is a first-order child of the parent path, false otherwise.
         */
        bool isFirstOrderChildOf(const Path &parent) const
        {
            // First check if it could be a child at all
            if (parent.pathParts_.size() >= pathParts_.size())
                return false;

            // Special case: if parent is root, then any path with exactly one part is a first-order child
            if (parent.isRoot())
                return pathParts_.size() == 1;

            // Check if it could be a first-order child at all
            if (parent.pathParts_.size() != pathParts_.size() - 1)
                return false;

            for (size_t i = 0; i < parent.pathParts_.size(); ++i)
            {
                if (parent.pathParts_[i] != pathParts_[i])
                    return false;
            }

            return true;
        }

        /**
         * @brief Checks if the current path is a parent of the given child path. A path A is considered a parent of path B if B starts with A followed by a path separator. For example, "/a/b" is a parent of "/a/b/c.txt", but not a parent of "/a/b".
         * @param child The child path to check against.
         * @return True if the current path is a parent of the child path, false otherwise.
         */
        bool isParentOf(const Path &child) const
        {
            return child.isChildOf(*this);
        }

        /**
         * @brief Returns a new Path that is the relative path from the given parent path to the current path. For example, if the current path is "/a/b/c.txt" and the parent path is "/a/b", it will return "c.txt". If the current path is not a child of the parent path, it will return the original path.
         * @param parent The parent path to calculate the relative path from.
         */
        Path relativeTo(const Path &parent) const
        {
            if (!isChildOf(parent))
                return *this;
            Path relative;
            relative.pathParts_ = ETVector<ETString>(pathParts_.begin() + parent.pathParts_.size(), pathParts_.end());
            relative.isAbsolute_ = false;
            return relative;
        }

        static Path root()
        {
            return Path("/");
        }
    };

}
#endif