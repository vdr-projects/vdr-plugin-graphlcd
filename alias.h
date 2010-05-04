/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * alias.h - alias class for converting channel id to its alias name
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GRAPHLCD_ALIAS_H_
#define _GRAPHLCD_ALIAS_H_

#include <map>
#include <string>

class cChannelAliasList
{
private:
    std::map<std::string, std::string> mAliases;
public:
    bool Load(const std::string & CfgPath);
    std::string GetAlias(const std::string & ChannelID) const;
};

#endif
