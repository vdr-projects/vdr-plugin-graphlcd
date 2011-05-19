/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * display.h - display class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 * (c) 2004-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2011 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#ifndef _GRAPHLCD_DISPLAY_H_
#define _GRAPHLCD_DISPLAY_H_

#include <stdint.h>

#include <string>
#include <vector>
#include <map>

#include <glcdgraphics/bitmap.h>
#include <glcddrivers/driver.h>
#include <glcdskin/skin.h>

#include "global.h"
#include "setup.h"
#include "state.h"
#include "skinconfig.h"

#include "service.h"

#include <vdr/thread.h>


enum eThreadState
{
    StateNormal,
    StateReplay,
    StateMenu
};

// state of current display mode (interesting for displays w/ touchpads)
enum eDisplayMode
{
    DisplayModeNormal,
    DisplayModeInteractive
};

// external data set via SVDRP
class cExtData
{
private:
    std::map<std::string,std::string>           data;
    std::map<std::string,std::string>::iterator it;
    std::map<std::string,uint64_t>              expData;
    std::map<std::string,uint64_t>::iterator    expDataIt;
public:
    cExtData(void) {}
    ~cExtData(void) { data.clear(); expData.clear(); }
    
    bool Set(std::string key, std::string value, uint32_t expire = 0);
    bool Unset(std::string key);
    bool IsSet(std::string key);
    std::string Get(std::string key);
};


// Display update Thread
class cGraphLCDDisplay : public cThread
{
public:
    cGraphLCDDisplay(void);
    ~cGraphLCDDisplay(void);

    bool Initialise(GLCD::cDriver * Lcd, const std::string & CfgPath, const std::string & SkinsPath, const std::string & SkinName);
    void Tick();
    void Update();
    void Clear();
    void Replaying(bool Starting);
    void SetMenuClear();
    void SetMenuTitle();
    void SetMenuCurrent();
    const GLCD::cBitmap * GetScreen() const { return mScreen; }

    void ForceUpdateBrightness();

    const cGraphLCDService * GetServiceObject() const { return mService; }

    GLCD::cDriver * GetDriver() const { return mLcd; }

    const eDisplayMode GetDisplayMode() const { return mDisplayMode; }
    cExtData * GetExtData() const { return mExtData; }
protected:
    virtual void Action();

private:
    GLCD::cDriver * mLcd;
    GLCD::cBitmap * mScreen;
    GLCD::cSkin * mSkin;
    cGraphLCDSkinConfig * mSkinConfig;

    bool mUpdate;
    uint64_t mUpdateAt;
    uint64_t mLastTimeMs;

    eThreadState mState;
    eThreadState mLastState;

    cMutex mMutex;
    cGraphLCDState * mGraphLCDState;

    bool mShowVolume;

    void UpdateIn(uint64_t msec);

    /* set brightness depending on user activity */
    void SetBrightness();
    uint64_t LastTimeBrightness;
    int nCurrentBrightness;
    bool bBrightnessActive;
    /* external services */
    cGraphLCDService * mService;
    /* display mode (normal or interactive) */
    eDisplayMode mDisplayMode;
    uint64_t LastTimeDisplayMode;
    cExtData * mExtData;
};

#endif
