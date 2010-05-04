/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * display.c - display class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#include <stdlib.h>

#include <algorithm>

#include <glcddrivers/config.h>
#include <glcddrivers/drivers.h>
#include <glcdskin/parser.h>

#include "display.h"
#include "global.h"
#include "i18n.h"
#include "setup.h"
#include "state.h"
#include "strfct.h"

#include <vdr/tools.h>
#include <vdr/menu.h>

cGraphLCDDisplay::cGraphLCDDisplay()
:   cThread("glcd_display"),
    mLcd(NULL),
    mScreen(NULL),
    mSkin(NULL),
    mSkinConfig(NULL),
    mGraphLCDState(NULL)
{
    mUpdate = false;
    mUpdateAt = 0;
    mLastTimeMs = 0;

    mState = StateNormal;
    mLastState = StateNormal;

    mShowVolume = false;

    nCurrentBrightness = -1;
    LastTimeBrightness = 0;
    bBrightnessActive = true;
}

cGraphLCDDisplay::~cGraphLCDDisplay()
{
    Cancel(3);

    delete mSkin;
    delete mSkinConfig;
    delete mScreen;
    delete mGraphLCDState;
}

bool cGraphLCDDisplay::Initialise(GLCD::cDriver * Lcd, const std::string & CfgPath, const std::string & SkinsPath, const std::string & SkinName)
{
    std::string skinsPath;

    if (!Lcd)
        return false;
    mLcd = Lcd;

    mGraphLCDState = new cGraphLCDState(this);
    if (!mGraphLCDState)
        return false;

    skinsPath = SkinsPath;
    if (skinsPath == "")
        skinsPath = CfgPath + "/skins";

    mSkinConfig = new cGraphLCDSkinConfig(this, CfgPath, skinsPath, SkinName, mGraphLCDState);
    if (!mSkinConfig)
    {
        esyslog("graphlcd plugin: ERROR creating skin config\n");
        return false;
    }

    Start();
    return true;
}

void cGraphLCDDisplay::Tick(void)
{
    if (mGraphLCDState)
        mGraphLCDState->Tick();
}

void cGraphLCDDisplay::Action(void)
{
    std::string skinFileName;

    if (mLcd->Init() != 0)
    {
        esyslog("graphlcd plugin: ERROR: Failed initializing display\n");
        return;
    }

    mScreen = new GLCD::cBitmap(mLcd->Width(), mLcd->Height());
    if (!mScreen)
    {
        esyslog("graphlcd plugin: ERROR creating drawing bitmap\n");
        return;
    }

    skinFileName = mSkinConfig->SkinPath() + "/" + mSkinConfig->SkinName() + ".skin";
    mSkin = GLCD::XmlParse(*mSkinConfig, mSkinConfig->SkinName(), skinFileName);
    if (!mSkin)
    {
        esyslog("graphlcd plugin: ERROR loading skin\n");
        return;
    }
    mSkin->SetBaseSize(mScreen->Width(), mScreen->Height());

    mLcd->Clear();
    mLcd->Refresh(true);
    mUpdate = true;

    while (Running())
    {
        if (GraphLCDSetup.PluginActive)
        {
            uint64_t currTimeMs = cTimeMs::Now();

            if (mUpdateAt != 0)
            {
                // timed Update enabled
                if (currTimeMs > mUpdateAt)
                {
                    mUpdateAt = 0;
                    mUpdate = true;
                }
            }

            if (GraphLCDSetup.ShowVolume)
            {
                tVolumeState volume;
                volume = mGraphLCDState->GetVolumeState();

                if (volume.lastChange > 0)
                {
                    if (!mShowVolume)
                    {
                        if (currTimeMs - volume.lastChange < 2000)
                        {
                            mShowVolume = true;
                            mUpdate = true;
                        }
                    }
                    else
                    {
                        if (currTimeMs - volume.lastChange > 2000)
                        {
                            mShowVolume = false;
                            mUpdate = true;
                        }
                    }
                }
            }

            // update Display every minute
            if (mState == StateNormal && currTimeMs/60000 != mLastTimeMs/60000)
            {
                mUpdate = true;
            }

            // update Display every second in replay state
            if (mState == StateReplay && currTimeMs/1000 != mLastTimeMs/1000)
            {
                mUpdate = true;
            }

            // update display if BrightnessDelay is exceeded
            if ((nCurrentBrightness == GraphLCDSetup.BrightnessActive) && 
                ((cTimeMs::Now() - LastTimeBrightness) > (uint64_t) (GraphLCDSetup.BrightnessDelay*1000))) 
            {
                mUpdate = true;
            }

            // external service changed (check each second)
            if ( (currTimeMs/1000 != mLastTimeMs/1000) && mGraphLCDState->CheckServiceEventUpdate())
            {
                mUpdate = true;
            }

            if (mUpdate)
            {
                mUpdateAt = 0;
                mUpdate = false;

                mGraphLCDState->Update();

                mScreen->Clear();
                GLCD::cSkinDisplay * display = NULL;

                if (mState == StateNormal)
                    display = mSkin->GetDisplay("normal");
                else if (mState == StateReplay)
                    display = mSkin->GetDisplay("replay");
                else if (mState == StateMenu)
                    display = mSkin->GetDisplay("menu");
                if (display)
                    display->Render(mScreen);
                if (mShowVolume)
                {
                    display = mSkin->GetDisplay("volume");
                    if (display)
                        display->Render(mScreen);
                }
                if (GraphLCDSetup.ShowMessages && mGraphLCDState->ShowMessage())
                {
                    display = mSkin->GetDisplay("message");
                    if (display)
                        display->Render(mScreen);
                }
                mLcd->SetScreen(mScreen->Data(), mScreen->Width(), mScreen->Height(), mScreen->LineSize());
                mLcd->Refresh(false);
                mLastTimeMs = currTimeMs;
                SetBrightness();
            }
            else
            {
                cCondWait::SleepMs(100);
            }
        }
        else
        {
            cCondWait::SleepMs(100);
        }
    }
}

void cGraphLCDDisplay::Update()
{
    mUpdate = true;
}

void cGraphLCDDisplay::UpdateIn(uint64_t msec)
{
    if (msec == 0)
    {
        mUpdateAt = 0;
    }
    else
    {
        mUpdateAt = cTimeMs::Now() + msec;
    }
}

void cGraphLCDDisplay::Replaying(bool Starting)
{
    if (Starting)
    {
        if (mState != StateMenu)
        {
            mState = StateReplay;
        }
        else
        {
            mLastState = StateReplay;
        }
    }
    else
    {
        if (mState != StateMenu)
        {
            mState = StateNormal;
        }
        else
        {
            mLastState = StateNormal;
        }
    }
    Update();
}

void cGraphLCDDisplay::SetMenuClear()
{
    mSkinConfig->SetMenuClear();
    if (mState == StateMenu)
    {
        mState = mLastState;
        // activate delayed Update
        UpdateIn(100);
    }
    else
    {
        Update();
    }
}

void cGraphLCDDisplay::SetMenuTitle()
{
    if (mState != StateMenu)
    {
        mLastState = mState;
        mState = StateMenu;
    }
    UpdateIn(100);
}

void cGraphLCDDisplay::SetMenuCurrent()
{
    if (mState != StateMenu)
    {
        mLastState = mState;
        mState = StateMenu;
    }
    UpdateIn(100);
}

void cGraphLCDDisplay::SetBrightness()
{
    //mutex.Lock();
    bool bActive = bBrightnessActive
                   || (mState != StateNormal)
                   || (GraphLCDSetup.ShowVolume && mShowVolume)
                   || (GraphLCDSetup.ShowMessages && mGraphLCDState->ShowMessage())
                   || (GraphLCDSetup.BrightnessDelay == 900);
    if (bActive)
    {
        LastTimeBrightness = cTimeMs::Now();
        bBrightnessActive = false;
    }
    if ((bActive ? GraphLCDSetup.BrightnessActive : GraphLCDSetup.BrightnessIdle) != nCurrentBrightness)
    {
        if (bActive)
        {
            mLcd->SetBrightness(GraphLCDSetup.BrightnessActive);
            nCurrentBrightness = GraphLCDSetup.BrightnessActive;
        }
        else
        {
            if (GraphLCDSetup.BrightnessDelay < 1
                || ((cTimeMs::Now() - LastTimeBrightness) > (uint64_t) (GraphLCDSetup.BrightnessDelay*1000)))
            {
                mLcd->SetBrightness(GraphLCDSetup.BrightnessIdle);
                nCurrentBrightness = GraphLCDSetup.BrightnessIdle;
            }
        }
    }
    //mutex.Unlock();
}

void cGraphLCDDisplay::ForceUpdateBrightness() {
    bBrightnessActive = true;
    SetBrightness();
}
