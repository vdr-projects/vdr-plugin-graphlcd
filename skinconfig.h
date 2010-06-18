/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * skinconfig.h - skin config class that implements all the callbacks
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GRAPHLCD_SKINCONFIG_H_
#define _GRAPHLCD_SKINCONFIG_H_

#include "alias.h"

class cGraphLCDSkinConfig : public GLCD::cSkinConfig
{
private:
    const cGraphLCDDisplay * mDisplay;
    std::string mConfigPath;
    std::string mSkinPath;
    std::string mSkinName;
    cGraphLCDState * mState;
    cChannelAliasList mAliasList;
    std::vector <int> mTabs;

public:
    cGraphLCDSkinConfig(const cGraphLCDDisplay * Display, const std::string & CfgPath, const std::string & SkinsPath, const std::string & SkinName, cGraphLCDState * State);
    virtual ~cGraphLCDSkinConfig();
    void SetMenuClear();

    virtual std::string SkinPath(void);
    virtual std::string FontPath(void);
    virtual std::string CharSet(void);
    virtual std::string Translate(const std::string & Text);
    virtual GLCD::cType GetToken(const GLCD::tSkinToken & Token);
    virtual int GetTokenId(const std::string & Name);
    virtual int GetTabPosition(int Index, int MaxWidth, const GLCD::cFont & Font);
    virtual uint64_t Now(void);

    const std::string & SkinName(void) const { return mSkinName; }
};

#endif
