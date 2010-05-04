/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * display.h - display class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#ifndef _GRAPHLCD_DISPLAY_H_
#define _GRAPHLCD_DISPLAY_H_

#include <stdint.h>

#include <string>
#include <vector>

#include <glcdgraphics/bitmap.h>
#include <glcddrivers/driver.h>
#include <glcdskin/skin.h>

#include "global.h"
#include "setup.h"
#include "state.h"
#include "skinconfig.h"

#include <vdr/thread.h>


enum eThreadState
{
    StateNormal,
    StateReplay,
    StateMenu
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
    void Replaying(bool Starting);
    void SetMenuClear();
    void SetMenuTitle();
    void SetMenuCurrent();
    const GLCD::cBitmap * GetScreen() const { return mScreen; }

    void ForceUpdateBrightness();

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

    /** set brightness depending on user activity */
    void SetBrightness();
    uint64_t LastTimeBrightness;
    int nCurrentBrightness;
    bool bBrightnessActive;
};

#endif
