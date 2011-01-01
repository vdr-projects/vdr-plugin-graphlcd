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
 */

#ifndef GRAPHLCD_DISPLAY_H
#define GRAPHLCD_DISPLAY_H

#include <stdint.h>

#include <string>
#include <vector>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/font.h>

#include "global.h"
#include "layout.h"
#include "logolist.h"
#include "setup.h"
#include "state.h"
#include "widgets.h"

#include <vdr/thread.h>
#include <vdr/player.h>


#define LCDMAXCARDS 4
static const int kMaxTabCount = 10;

enum ThreadState
{
    Normal,
    Replay,
    Menu
};

// Display update Thread
class cGraphLCDDisplay : public cThread
{
public:
    cGraphLCDDisplay(void);
    ~cGraphLCDDisplay(void);

    int Init(GLCD::cDriver * Lcd, const char * CfgDir);
    void Tick(void);

    void SetChannel(int ChannelNumber);
    void SetClear();
    void SetOsdTitle();
    void SetOsdItem(const char * Text);
    void SetOsdCurrentItem();
    void Recording(const cDevice * Device , const char * Name);
    void Replaying(bool starting, eReplayMode replayMode);
    //void SetStatusMessage(const char * Msg);
    void SetOsdTextItem(const char * Text, bool Scroll);
    //void SetColorButtons(const char * Red, const char * Green, const char * Yellow, const char * Blue);
    void SetVolume(int Volume, bool Absolute);

    void Update();
    void Clear();

protected:
    virtual void Action();

private:
    bool update;
    bool active;
    GLCD::cDriver * mLcd;

    cFontList fontList;
    GLCD::cBitmap * bitmap;
    const GLCD::cFont * largeFont;
    const GLCD::cFont * normalFont;
    const GLCD::cFont * smallFont;
    const GLCD::cFont * symbols;
    std::string cfgDir;
    std::string fontDir;
    std::string logoDir;

    ThreadState State;
    ThreadState LastState;

    cMutex mutex;
    cGraphLCDState * GraphLCDState;

    int menuTop;
    int menuCount;
    int tabCount;
    int tab[kMaxTabCount];
    int tabMax[kMaxTabCount];

    std::vector <std::string> textItemLines;
    int textItemTop;
    int textItemVisibleLines;

    bool showVolume;

    time_t CurrTime;
    time_t LastTime;
    time_t LastTimeCheckSym;
    time_t LastTimeModSym;
    struct timeval CurrTimeval;
    struct timeval UpdateAt;

    std::vector<cScroller> scroller;

    cGraphLCDLogoList * logoList;
    cGraphLCDLogo * logo;

    char szETSymbols[32];

    void DisplayChannel();
    void DisplayTime();
    void DisplayLogo();
    void DisplaySymbols();
    void DisplayProgramme();
    void DisplayReplay(tReplayState & replay);
    void DisplayMenu();
    void DisplayMessage();
    void DisplayTextItem();
    void DisplayColorButtons();
    void DisplayVolume();

    void UpdateIn(long usec);
    bool CheckAndUpdateSymbols();

#if VDRVERSNUM >= 10701
    /** Check if replay index bigger as one hour */
    bool IndexIsGreaterAsOneHour(int Index, double framesPerSecond) const;
    /** Translate replay index to string with minute and second MM:SS */
    const char *IndexToMS(int Index, double framesPerSecond) const;
#else
    bool IndexIsGreaterAsOneHour(int Index) const;
    const char *IndexToMS(int Index) const;
#endif
    /** Compare Scroller with new Textbuffer*/
    bool IsScrollerTextChanged(const std::vector<cScroller> & scroller, const std::vector <std::string> & lines) const;
    /** Returns true if Logo loaded and active*/
    bool IsLogoActive() const;
    /** Returns true if Symbols loaded and active*/
    bool IsSymbolsActive() const;

    /** Set Brightness depends user activity */
    void SetBrightness();
    uint64_t LastTimeBrightness;
    int nCurrentBrightness;
    bool bBrightnessActive;

    cCharSetConv *conv;
    const char * Convert(const char *s);
};

#endif
