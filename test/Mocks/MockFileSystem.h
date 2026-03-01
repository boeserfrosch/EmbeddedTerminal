#ifndef MOCKFILESYSTEM_H
#define MOCKFILESYSTEM_H

#include "../../src/interfaces/IFileSystem.h"
#include "MockFile.h"

#include <algorithm>
#include <map>
#include <memory>

namespace EmbeddedTerminal
{
    class MockFileSystem : public IFileSystem
    {
    private:
        struct Node
        {
            bool isDirectory = false;
            std::shared_ptr<ETString> content = std::make_shared<ETString>("");
            size_t openReadHandles = 0;
            bool openWriteHandle = false;
        };

        ETMap<Path, std::shared_ptr<Node>> nodes_;

        Path normalizePath_(const Path &path) const
        {
            if (path.isEmpty() || path.isRoot())
            {
                return Path::root();
            }
            if (path.isAbsolute())
            {
                return path;
            }
            return Path::root() + path;
        }

        bool isWriteMode_(const char *mode) const
        {
            const ETString modeString = mode ? ETString(mode) : ETString(FILE_MODE_READ);
            return modeString == FILE_MODE_WRITE || modeString == FILE_MODE_APPEND;
        }

        bool parentDirectoryExists_(const Path &path) const
        {
            if (path == Path::root())
            {
                return true;
            }

            const Path parent = path.getBasePath();
            if (parent.isEmpty())
            {
                return true;
            }

            const auto parentIt = nodes_.find(parent);
            return parentIt != nodes_.end() && parentIt->second->isDirectory;
        }

        bool ensureDirectoryRecursive_(const Path &directoryPath)
        {
            const Path normalized = normalizePath_(directoryPath);
            if (normalized == Path::root())
            {
                return true;
            }

            const auto existing = nodes_.find(normalized);
            if (existing != nodes_.end())
            {
                return existing->second->isDirectory;
            }

            const Path parent = normalized.getBasePath();
            if (!ensureDirectoryRecursive_(parent))
            {
                return false;
            }

            auto dirNode = std::make_shared<Node>();
            dirNode->isDirectory = true;
            nodes_[normalized] = dirNode;
            return true;
        }

        ETFile makeFileHandle_(const std::shared_ptr<Node> &node, bool writeMode)
        {
            auto fileHandle = std::make_shared<MockFile>(node->content, true);
            fileHandle->isDirectory_ = false;
            fileHandle->pos_ = 0;

            std::weak_ptr<Node> weakNode(node);
            fileHandle->setCloseCallback([weakNode, writeMode]()
                                         {
                if (auto lockedNode = weakNode.lock())
                {
                    if (writeMode)
                    {
                        lockedNode->openWriteHandle = false;
                    }
                    else if (lockedNode->openReadHandles > 0)
                    {
                        --lockedNode->openReadHandles;
                    }
                } });

            return ETFile(fileHandle);
        }

    public:
        MockFileSystem()
        {
            auto root = std::make_shared<Node>();
            root->isDirectory = true;
            nodes_[Path::root()] = root;
        }

        ETFile open(const Path &path, const char *mode = FILE_MODE_READ, const bool create = false) override
        {
            const Path normalized = normalizePath_(path);
            const bool writeMode = isWriteMode_(mode);

            auto it = nodes_.find(normalized);
            if (it == nodes_.end())
            {
                if (!create)
                {
                    return ETFile();
                }

                const Path parent = normalized.getBasePath();
                if (!ensureDirectoryRecursive_(parent))
                {
                    return ETFile();
                }

                auto fileNode = std::make_shared<Node>();
                fileNode->isDirectory = false;
                nodes_[normalized] = fileNode;
                it = nodes_.find(normalized);
            }

            if (it->second->isDirectory)
            {
                return ETFile();
            }

            if (writeMode && it->second->openWriteHandle)
            {
                return ETFile();
            }

            if (writeMode)
            {
                it->second->openWriteHandle = true;
            }
            else
            {
                ++it->second->openReadHandles;
            }

            return makeFileHandle_(it->second, writeMode);
        }

        bool exists(const Path &path) override
        {
            return nodes_.find(normalizePath_(path)) != nodes_.end();
        }

        bool isDirectory(const Path &path) override
        {
            const auto it = nodes_.find(normalizePath_(path));
            return it != nodes_.end() && it->second->isDirectory;
        }

        bool isEmpty(const Path &path) override
        {
            const Path normalized = normalizePath_(path);
            const auto it = nodes_.find(normalized);
            if (it == nodes_.end())
            {
                return false;
            }

            if (!it->second->isDirectory)
            {
                return it->second->content->empty();
            }

            const Path parentPath = normalized;
            for (const auto &entry : nodes_)
            {
                if (entry.first == normalized)
                {
                    continue;
                }

                if (entry.first.isFirstOrderChildOf(parentPath))
                {
                    return false;
                }
            }

            return true;
        }

        bool remove(const Path &path) override
        {
            const Path normalized = normalizePath_(path);
            const auto it = nodes_.find(normalized);
            if (it == nodes_.end() || it->second->isDirectory)
            {
                return false;
            }

            nodes_.erase(it);
            return true;
        }

        bool mkdir(const Path &path) override
        {
            const Path normalized = normalizePath_(path);
            if (nodes_.find(normalized) != nodes_.end())
            {
                return false;
            }

            if (!ensureDirectoryRecursive_(normalized))
            {
                return false;
            }

            return true;
        }

        bool rmdir(const Path &path) override
        {
            const Path normalized = normalizePath_(path);
            if (normalized == Path::root())
            {
                return false;
            }

            const auto it = nodes_.find(normalized);
            if (it == nodes_.end() || !it->second->isDirectory)
            {
                return false;
            }

            if (!isEmpty(normalized))
            {
                return false;
            }

            nodes_.erase(it);
            return true;
        }

        ETVector<Path> list(const Path &path, const ETString &prefix = "") const override
        {
            ETVector<Path> result;

            const Path normalized = normalizePath_(path);
            const auto it = nodes_.find(normalized);
            if (it == nodes_.end())
            {
                return result;
            }

            if (!it->second->isDirectory)
            {
                const ETString fileName = normalized.getName();
                if (prefix.empty() || fileName.startsWith(prefix))
                {
                    result.push_back(Path(fileName));
                }
                return result;
            }

            const Path parentPath = normalized;
            ETVector<ETString> names;
            for (const auto &entry : nodes_)
            {
                if (entry.first == normalized)
                {
                    continue;
                }

                const Path &entryPath = entry.first;
                if (!entryPath.isFirstOrderChildOf(parentPath))
                {
                    continue;
                }

                const ETString childName = entryPath.getName();
                if (!prefix.empty() && !childName.startsWith(prefix))
                {
                    continue;
                }

                names.push_back(childName);
            }

            std::sort(names.begin(), names.end());
            for (const auto &name : names)
            {
                result.push_back(Path(name));
            }

            return result;
        }
    };
}

#endif // MOCKFILESYSTEM_H
