/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * skinconfig.h - skin config class that implements all the callbacks
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2011 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#ifndef _GRAPHLCD_SKINCONFIG_H_
#define _GRAPHLCD_SKINCONFIG_H_

#include "alias.h"

#include <glcddrivers/driver.h>

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

    const std::string & SkinName(void) const { return mSkinName; }

    virtual uint64_t Now(void);
    virtual GLCD::cDriver * GetDriver(void) const;
};

#endif
