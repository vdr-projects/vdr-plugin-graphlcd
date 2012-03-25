/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * display.c - display class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 * (c) 2004-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2012 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#include <stdlib.h>

#include <algorithm>

#include <glcddrivers/config.h>
#include <glcddrivers/drivers.h>
#include <glcdskin/parser.h>

#include "display.h"
#include "global.h"
#include "setup.h"
#include "state.h"
#include "strfct.h"

#include <vdr/tools.h>
#include <vdr/menu.h>

#include <vdr/remote.h>

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
    
    mDisplayMode = DisplayModeNormal;
    LastTimeDisplayMode = 0;

    mShowVolume = false;
    mShowAudio = false;

    nCurrentBrightness = -1;
    LastTimeBrightness = 0;
    bBrightnessActive = true;

    mService = NULL; /* cannot be initialised here (mGraphLCDState not yet available) */
}

cGraphLCDDisplay::~cGraphLCDDisplay()
{
    Cancel(3);

    delete mSkin;
    delete mSkinConfig;
    delete mScreen;
    delete mGraphLCDState;

    delete mService;
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

    // must be initialised before cGraphLCDSkinConfig (else: seg fault)
    mService = new cGraphLCDService(mGraphLCDState);

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
        GraphLCDSetup.PluginActive = 0;
        Cancel(-1); // cancel display thread
        return;
    }

    mScreen = new GLCD::cBitmap(mLcd->Width(), mLcd->Height());
    if (!mScreen)
    {
        esyslog("graphlcd plugin: ERROR creating drawing bitmap\n");
        return;
    }

    skinFileName = mSkinConfig->SkinPath() + "/" + mSkinConfig->SkinName() + ".skin";
    std::string errorString = "";
    mSkin = GLCD::XmlParse(*mSkinConfig, mSkinConfig->SkinName(), skinFileName, errorString);
    if (!mSkin)
    {
        int skipx = mLcd->Width() >> 3;
        int skipy = mLcd->Height() >> 3;

        esyslog("graphlcd plugin: ERROR loading skin '%s'\n", mSkinConfig->SkinName().c_str());

        mLcd->Clear();
        // clear the screen with a filled rectangle using a defined colour to guarantee visible text or 'X'
#ifdef GRAPHLCD_CBITMAP_ARGB
        mScreen->DrawRectangle(0, 0, mLcd->Width()-1, mLcd->Height()-1, GLCD::cColor::Black, true);
#else
        mScreen->DrawRectangle(0, 0, mLcd->Width()-1, mLcd->Height()-1, GLCD::clrBlack, true);
#endif

        GLCD::cFont * font = new GLCD::cFont();
        if ( font->LoadFNT( mSkinConfig->FontPath() + "/" + "f8n.fnt") ) {
          std::vector <std::string> lines;
          font->WrapText(mLcd->Width()-10, mLcd->Height()-1, errorString, lines);
          int lh = font->LineHeight();
          for (size_t i = 0; i < lines.size(); i++) {
#ifdef GRAPHLCD_CBITMAP_ARGB
              mScreen->DrawText(3, 1+i*lh, mLcd->Width()-1, lines[i], font, GLCD::cColor::White);
#else
              mScreen->DrawText(3, 1+i*lh, mLcd->Width()-1, lines[i], font, GLCD::clrWhite);
#endif
          }
        } else {
          // draw an 'X' to inform the user that there was a problem with loading the skin
          // (better than just leaving an empty screen ...)
#ifdef GRAPHLCD_CBITMAP_ARGB
          mScreen->DrawLine(skipx, skipy, mLcd->Width()-1-skipx, mLcd->Height()-1-skipy, GLCD::cColor::White);
          mScreen->DrawLine(mLcd->Width()-1-skipx, skipy, skipx, mLcd->Height()-1-skipy, GLCD::cColor::White);
#else
          mScreen->DrawLine(skipx, skipy, mLcd->Width()-1-skipx, mLcd->Height()-1-skipy, GLCD::clrWhite);
          mScreen->DrawLine(mLcd->Width()-1-skipx, skipy, skipx, mLcd->Height()-1-skipy, GLCD::clrWhite);
#endif
        }
#ifdef GRAPHLCD_CBITMAP_ARGB
        mLcd->SetScreen(mScreen->Data(), mScreen->Width(), mScreen->Height());
#else
        mLcd->SetScreen(mScreen->Data(), mScreen->Width(), mScreen->Height(), mScreen->LineSize());
#endif
        mLcd->Refresh(true);
        return;
    }
    mSkin->SetBaseSize(mScreen->Width(), mScreen->Height());

    mLcd->Clear();
    mLcd->Refresh(true);
    mUpdate = true;

    uint64_t lastEventMs = 0;
#define MIN_EVENT_DELAY 500

    while (Running())
    {
        if (GraphLCDSetup.PluginActive)
        {
            uint64_t currTimeMs = cTimeMs::Now();
            GLCD::cSkinDisplay * display = NULL;

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

            if (GraphLCDSetup.ShowMenu)
            {
                tAudioState audio;
                audio = mGraphLCDState->GetAudioState();

                if (audio.tracks.size() == 0) {
                    mShowAudio = false;
                } else
                if (audio.lastChange > 0)
                {
                    if (!mShowAudio)
                    {
                        if (currTimeMs - audio.lastChange < 5000)
                        {
                            mShowAudio = true;
                            mUpdate = true;
                        }
                    }
                    else
                    {
                        if (currTimeMs - audio.lastChange > 5000)
                        {
                            mShowAudio = false;
                            mUpdate = true;
                        }
                    }
                }
            }

            /* display mode (normal or interactive): reset after 10 secs if not normal */
            if ( (mDisplayMode != DisplayModeNormal) && ( (uint32_t)(currTimeMs - LastTimeDisplayMode) > (uint32_t)(10000)) ) {
                mDisplayMode = DisplayModeNormal;
                LastTimeDisplayMode = currTimeMs;
                mUpdate = true;
            }

            if (mState == StateNormal)
                display = mSkin->GetDisplay("normal");
            else if (mState == StateReplay)
                display = mSkin->GetDisplay("replay");
            else if (mState == StateMenu)
                display = mSkin->GetDisplay("menu");
            if (display && display->NeedsUpdate(currTimeMs ) )
               mUpdate = true;


            // update Display every minute
            if (mState == StateNormal && (currTimeMs/60000 != mLastTimeMs/60000))
            {
                mUpdate = true;
            }

            // update Display every second in replay state
            if (mState == StateReplay && (currTimeMs/1000 != mLastTimeMs/1000))
            {
                mUpdate = true;
            }


            bool bActive = bBrightnessActive
                   || (mState != StateNormal)
                   || (GraphLCDSetup.ShowVolume && mShowVolume)
                   || (GraphLCDSetup.ShowMenu && mShowAudio)
                   || (GraphLCDSetup.ShowMessages && mGraphLCDState->ShowMessage())
                   || (GraphLCDSetup.BrightnessDelay == 900);

            // update display if BrightnessDelay is exceeded
            if (bActive && (nCurrentBrightness == GraphLCDSetup.BrightnessActive) && 
                ((uint32_t)((/*cTimeMs::Now()*/currTimeMs - LastTimeBrightness)) > (uint32_t)(GraphLCDSetup.BrightnessDelay*1000))) 
            {
                mUpdate = true;
            }

            // external service changed (check each 1/10th second)
            if ( (currTimeMs/100 != mLastTimeMs/100) && mService->NeedsUpdate(currTimeMs))
            {
                mUpdate = true;
            }


            // check event
            GLCD::cGLCDEvent * ev = mSkin->Config().GetDriver()->GetEvent();
            if (ev && display && ((uint32_t)(currTimeMs-lastEventMs) > (uint32_t)(MIN_EVENT_DELAY) )) {
                std::string rv = "";

                if ( (rv = display->CheckAction(ev) ) != "") {
                    if (rv.substr(0,4) == "Key.") {
                        cRemote::Put(cKey::FromString(rv.substr(4).c_str()), true);
                        LastTimeDisplayMode = currTimeMs; /* if interactive mode: extend it */ 
                    } else if (rv.substr(0,5) == "Mode.") {
                        if (rv.substr(5) == "Interactive") {
                            mDisplayMode = DisplayModeInteractive;
                            LastTimeDisplayMode = currTimeMs;
                        } else if (rv.substr(5) == "Normal") {
                            mDisplayMode = DisplayModeNormal;
                            LastTimeDisplayMode = currTimeMs;
                        }
                        mUpdate = true;
                    }
                    lastEventMs = currTimeMs;
                }
            }

            if (mUpdate)
            {
                mUpdateAt = 0;
                mUpdate = false;

                mGraphLCDState->Update();

                mScreen->Clear(mSkin->GetBackgroundColor());

                mSkin->SetTSEvalTick(currTimeMs);

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
                if (mShowAudio)
                {
                    display = mSkin->GetDisplay("audio");
                    if (display)
                        display->Render(mScreen);
                }
                if (GraphLCDSetup.ShowMessages && mGraphLCDState->ShowMessage())
                {
                    display = mSkin->GetDisplay("message");
                    if (display)
                        display->Render(mScreen);
                }
#ifdef GRAPHLCD_CBITMAP_ARGB
                mLcd->SetScreen(mScreen->Data(), mScreen->Width(), mScreen->Height());
#else
                mLcd->SetScreen(mScreen->Data(), mScreen->Width(), mScreen->Height(), mScreen->LineSize());
#endif
                mLcd->Refresh(false);
                mLastTimeMs = currTimeMs;
                SetBrightness();

                // flush events
                mSkin->Config().GetDriver()->GetEvent();
            }
            else
            {
#if 0
                GLCD::cGLCDEvent * ev = mSkin->Config().GetDriver()->GetEvent();
                if (ev && display && ((uint32_t)(currTimeMs-lastEventMs) > (uint32_t)(MIN_EVENT_DELAY) )) {
                    std::string rv = "";

                    if ( (rv = display->CheckAction(ev) ) != "") {
                        if (rv.substr(0,4) == "Key.") {
                            cRemote::Put(cKey::FromString(rv.substr(4).c_str()), true);
                        }
                        lastEventMs = currTimeMs;
                    }
                }
#endif
                cCondWait::SleepMs(100);
            }
        }
        else
        {
            cCondWait::SleepMs(100);
        }
    }
    // clear screen before thread is stopping
    mLcd->Clear();
    mLcd->Refresh(true);
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
    if (mSkin)
        mSkin->SetTSEvalSwitch(cTimeMs::Now());

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
                   || (GraphLCDSetup.ShowMenu && mShowAudio)
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
                || ((unsigned int)(cTimeMs::Now() - LastTimeBrightness) > (unsigned int)(GraphLCDSetup.BrightnessDelay*1000)))
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


void cGraphLCDDisplay::Clear() {
  mScreen->Clear();
#ifdef GRAPHLCD_CBITMAP_ARGB
  mLcd->SetScreen(mScreen->Data(), mScreen->Width(), mScreen->Height());
#else
  mLcd->SetScreen(mScreen->Data(), mScreen->Width(), mScreen->Height(), mScreen->LineSize());
#endif
  mLcd->Refresh(false);
}
