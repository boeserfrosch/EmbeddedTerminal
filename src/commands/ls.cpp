#include "ls.h"
#include <sstream>

using namespace EmbeddedTerminal::cmd;
ETString ls::trigger(const ETString &keyword, const ETString &additional)
{
    additional.trim();
    auto param = split(additional, " ");

    auto paramCnt = param.size();
    auto path = _dir.pwd();
    if (param.size() > 0)
    {
        parseConf(param);
        if (param[paramCnt - 1].substr(0, 1) != "-")
        {
            if (!_dir.isDirectory(param.at(param.size() - 1).c_str()))
            {
                return param.at(param.size() - 1) + " is not a directory!\n";
            }
            path = _dir.pwd(param.at(param.size() - 1).c_str());
        }
    }
    auto content = _dir.ls(path);

    ETString result = path + "\n";
    for (const auto &entry : content)
    {
        if (lsConfig.longListing)
        {
            auto f = _dir.open(path + "/" + entry);
            result += toETString(f.size()) + " Bytes\t" + f.name() + "\n";
        }
        else
        {
            result += entry + "\t";
        }
    }
    result += "\n";
    return result;
}

ETString ls::usage(const ETString &keyword)
{
    return "List the conntent of directories\n\n" +
           keyword + " - List the content of the current directory\n" +
           keyword + " [path] - List the content of the specified directory\n" +
           keyword + " -l ([path]) - List the content of the optinal specified directory. Each entry gets a new line. Additional the size of each entry will be displayed\n";
}

void ls::parseConf(std::vector<ETString> params)
{
    lsConfig.longListing = false;

    for (const auto &param : params)
    {
        if (param.substr(0, 1) == "-")
        {
            if (param.contains("l"))
            {
                lsConfig.longListing = true;
            }
        }
    }

    // return lsConf();
}
