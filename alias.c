/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * alias.c - alias class for converting channel id to its alias name
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <fstream>

#include "alias.h"
#include "strfct.h"

#include <vdr/tools.h>

const char * kChannelAliasFileName = "channels.alias";

bool cChannelAliasList::Load(const std::string & CfgPath)
{
    std::fstream file;
    char readLine[1000];
    std::string line;
    std::string aliasFileName;
    std::string::size_type pos;
    std::string id;
    std::string alias;

    aliasFileName = CfgPath + "/" + kChannelAliasFileName;

#if (__GNUC__ < 3)
    file.open(aliasFileName.c_str(), std::ios::in);
#else
    file.open(aliasFileName.c_str(), std::ios_base::in);
#endif
    if (!file.is_open())
    {
        esyslog("graphlcd: Error opening channel alias file %s!", aliasFileName.c_str());
        return false;
    }

    while (!file.eof())
    {
        file.getline(readLine, 1000);
        line = trim(readLine);
        if (line.length() == 0)
            continue;
        if (line[0] == '#')
            continue;
        pos = line.find(":");
        if (pos == std::string::npos)
            continue;
        id = trim(line.substr(0, pos));
        alias = trim(line.substr(pos + 1));
        mAliases.insert(std::make_pair(id, alias));
    }
    file.close();

    return true;
}

std::string cChannelAliasList::GetAlias(const std::string & ChannelID) const
{
    std::map<std::string, std::string>::const_iterator pos;

    pos = mAliases.find(ChannelID);
    if (pos != mAliases.end())
        return pos->second;
    return "";
}
