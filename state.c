/**
 *  GraphLCD plugin for the Video Disk Recorder
 *
 *  status.c  -  status monitor class
 *
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online de>
 **/

#include <ctype.h>

#include <algorithm>

#include "display.h"
#include "state.h"
#include "strfct.h"

#include <vdr/eitscan.h>
#include <vdr/i18n.h>

#include "compat.h"

cGraphLCDState::cGraphLCDState(cGraphLCDDisplay * Display)
:   mDisplay(Display),
    first(true),
    tickUsed(false)
{
    channel.number = 0;
    channel.str = "";
    channel.strTmp = "";

    event.presentTime = 0;
    event.presentTitle = "";
    event.presentSubtitle = "";
    event.followingTime = 0;
    event.followingTitle = "";
    event.followingSubtitle = "";

    replay.name = "";
    replay.loopmode = "";
    replay.control = NULL;
    replay.mode = eReplayNormal;
    replay.current = 0;
#if VDRVERSNUM >= 10701
    replay.currentLast = DEFAULTFRAMESPERSECOND;
#else
    replay.currentLast = FRAMESPERSEC;
#endif
    replay.total = 0;
    replay.totalLast = 1;

    for (int i = 0; i < MAXDEVICES; i++)
    {
        card[i].recordingCount = 0;
        card[i].recordingName = "";
    }

    osd.currentItem = "";
    osd.title = "";
    for (int i = 0; i < 4; i++)
        osd.colorButton[i] = "";
    osd.message = "";
    osd.textItem = "";
    osd.currentItemIndex = 0;

    volume.value = -1;
    volume.lastChange = 0;

    SetChannel(cDevice::CurrentChannel());
}

cGraphLCDState::~cGraphLCDState()
{
}

void cGraphLCDState::ChannelSwitch(const cDevice * Device, int ChannelNumber)
{
    //printf("graphlcd plugin: cGraphLCDState::ChannelSwitch %d %d\n", Device->CardIndex(), ChannelNumber);
    if (GraphLCDSetup.PluginActive)
    {
        if (ChannelNumber > 0 && Device->IsPrimaryDevice() && !EITScanner.UsesDevice(Device))
        {
            if (ChannelNumber == cDevice::CurrentChannel())
            {
                SetChannel(ChannelNumber);
            }
        }
    }
}

#if VDRVERSNUM < 10338
void cGraphLCDState::Recording(const cDevice * Device, const char * Name)
#else
void cGraphLCDState::Recording(const cDevice * Device, const char * Name, const char *FileName, bool On)
#endif
{
    //printf("graphlcd plugin: cGraphLCDState::Recording %d %s\n", Device->CardIndex(), Name);
    if (GraphLCDSetup.PluginActive)
    {
        mutex.Lock();
#if VDRVERSNUM < 10338
        if (Name)
#else
        if (On)
#endif
        {
            card[Device->DeviceNumber()].recordingCount++;
#if VDRVERSNUM < 10338
            card[Device->DeviceNumber()].recordingName = Name;
#else
            card[Device->DeviceNumber()].recordingName = Name ? Name : "";
#endif
        }
        else
        {
            if (card[Device->DeviceNumber()].recordingCount > 0)
                card[Device->DeviceNumber()].recordingCount--;
            card[Device->DeviceNumber()].recordingName = "";
        }
        mutex.Unlock();

        mDisplay->Update();
    }
}

#if VDRVERSNUM < 10338
void cGraphLCDState::Replaying(const cControl * Control, const char * Name)
#else
void cGraphLCDState::Replaying(const cControl * Control, const char * Name, const char *FileName, bool On)
#endif
{
    //printf("graphlcd plugin: cGraphLCDState::Replaying %s\n", Name);
    if (GraphLCDSetup.PluginActive)
    {
#if VDRVERSNUM < 10338
        if (Name)
#else
        if (On)
#endif
        {
            mutex.Lock();
            replay.control = (cControl *) Control;
            replay.mode = eReplayNormal;
            replay.name = "";
            replay.loopmode = "";
#if VDRVERSNUM < 10338
            if (!isempty(Name))
#else
            if (Name && !isempty(Name))
#endif
            {
                if (GraphLCDSetup.IdentifyReplayType)
                {
                    int slen = strlen(Name);
                    bool bFound = false;
                    ///////////////////////////////////////////////////////////////////////
                    //Looking for mp3-Plugin Replay : [LS] (444/666) title
                    //
                    if (slen > 6 &&
                        *(Name+0)=='[' &&
                        *(Name+3)==']' &&
                        *(Name+5)=='(')
                    {
                        unsigned int i;
                        for (i=6; *(Name + i) != '\0'; ++i) //search for [xx] (xxxx) title
                        {
                            if (*(Name+i)==' ' && *(Name+i-1)==')')
                            {
                                bFound = true;
                                break;
                            }
                        }
                        if (bFound) //found MP3-Plugin replaymessage
                        {
                            unsigned int j;
                            // get loopmode
                            replay.loopmode = Name;
                            replay.loopmode = replay.loopmode.substr (0, 5);
                            if (replay.loopmode[2] == '.')
                                replay.loopmode.erase (2, 1);
                            if (replay.loopmode[1] == '.')
                                replay.loopmode.erase (1, 1);
                            if (replay.loopmode[1] == ']')
                                replay.loopmode = "";
                            //printf ("loopmode=<%s>\n", replay.loopmode.c_str ());
                            for (j=0;*(Name+i+j) != '\0';++j) //trim name
                            {
                                if (*(Name+i+j)!=' ')
                                    break;
                            }

                            if (strlen(Name+i+j) > 0)
                            { //if name isn't empty, then copy
                                replay.name = Name + i + j;
                            }
                            else
                            { //if Name empty, set fallback title
                                replay.name = tr("Unknown title");
                            }
                            replay.mode = eReplayMusic;
                        }
                    }
                    ///////////////////////////////////////////////////////////////////////
                    //Looking for DVD-Plugin Replay : 1/8 4/28,  de 2/5 ac3, no 0/7,  16:9, VOLUMENAME
                    //cDvdPlayerControl::GerDisplayHeaderLine
                    //                         titleinfo, audiolang,  spulang, aspect, title
                    if (!bFound)
                    {
                        if (slen>7)
                        {
                            unsigned int i,n;
                            for (n=0,i=0;*(Name+i) != '\0';++i)
                            { //search volumelabel after 4*", " => xxx, xxx, xxx, xxx, title
                                if (*(Name+i)==' ' && *(Name+i-1)==',')
                                {
                                    if (++n == 4)
                                    {
                                        bFound = true;
                                        break;
                                    }
                                }
                            }
                            if (bFound) //found DVD replaymessage
                            {
                                unsigned int j;bool b;
                                for (j=0;*(Name+i+j) != '\0';++j) //trim name
                                {
                                    if (*(Name+i+j)!=' ')
                                        break;
                                }

                                if (strlen(Name+i+j) > 0)
                                { // if name isn't empty, then copy
                                    replay.name = Name + i + j;
                                    // replace all '_' with ' '
                                    replace(replay.name.begin(), replay.name.end(), '_', ' ');
                                    for (j = 0, b = true; j < replay.name.length(); ++j)
                                    {
                                        // KAPITALIZE -> Kaptialize
                                        if (replay.name[j] == ' ')
                                            b = true;
                                        else if (b)
                                            b = false;
                                        else replay.name[j] = tolower(replay.name[j]);
                                    }
                                }
                                else
                                { //if Name empty, set fallback title
                                    replay.name = tr("Unknown title");
                                }
                                replay.mode = eReplayDVD;
                            }
                        }
                    }
                    if (!bFound)
                    {
                        int i;
                        for (i=slen-1;i>0;--i)
                        { //Reversesearch last Subtitle
                            // - filename contains '~' => subdirectory
                            // or filename contains '/' => subdirectory
                            switch (*(Name+i))
                            {
                                case '/':
                                {
                                    // look for file extentsion like .xxx or .xxxx
                                    if (slen>5 && ((*(Name+slen-4) == '.') || (*(Name+slen-5) == '.')))
                                    {
                                        replay.mode = eReplayFile;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                case '~':
                                {
                                    replay.name = Name + i + 1;
                                    bFound = true;
                                    i = 0;
                                }
                                default:
                                    break;
                            }
                        }
                    }

                    if (0 == strncmp(Name,"[image] ",8))
                    {
                        if (replay.mode != eReplayFile) //if'nt already Name stripped-down as filename
                            replay.name = Name + 8;
                        replay.mode = eReplayImage;
                        bFound = true;
                    }
                    else if (0 == strncmp(Name,"[audiocd] ",10))
                    {
                        replay.name = Name + 10;
                        replay.mode = eReplayAudioCD;
                        bFound = true;
                    }
                    if (!bFound || !GraphLCDSetup.ModifyReplayString)
                    {
                        replay.name = Name;
                    }
                }
                else
                {
                    replay.name = Name;
                }
            }
#if VDRVERSNUM >= 10701
            replay.currentLast = DEFAULTFRAMESPERSECOND;
#else
            replay.currentLast = FRAMESPERSEC;
#endif
            replay.totalLast = 1;
            mutex.Unlock();
        }
        else
        {
            mutex.Lock();
            replay.control = NULL;
            mutex.Unlock();
            SetChannel(channel.number);
        }
#if VDRVERSNUM < 10338
        mDisplay->Replaying(Name ? true : false, replay.mode);
#else
        mDisplay->Replaying(On, replay.mode);
#endif
    }
}

void cGraphLCDState::SetVolume(int Volume, bool Absolute)
{
    //printf("graphlcd plugin: cGraphLCDState::SetVolume %d %d\n", Volume, Absolute);
    if (GraphLCDSetup.PluginActive)
    {
        mutex.Lock();

#if VDRVERSNUM < 10402
        volume.value = Volume;
#else
        if (!Absolute)
        {
            volume.value += Volume;
        }
        else
        {
            volume.value = Volume;
        }
#endif
        if (!first)
        {
            volume.lastChange = cTimeMs::Now();
            mutex.Unlock();
            mDisplay->Update();
        }
        else
        {
            // first time
            first = false;
            mutex.Unlock();
        }
    }
}

void cGraphLCDState::Tick()
{
    //printf("graphlcd plugin: cGraphLCDState::Tick\n");
    if (GraphLCDSetup.PluginActive)
    {
        mutex.Lock();

        tickUsed = true;

        if (replay.control)
        {
            if (replay.control->GetIndex(replay.current, replay.total, false))
            {
                replay.total = (replay.total == 0) ? 1 : replay.total;
            }
            else
            {
                replay.control = NULL;
            }
        }

        mutex.Unlock();
    }
}

void cGraphLCDState::OsdClear()
{
    //printf("graphlcd plugin: cGraphLCDState::OsdClear\n");
    if (GraphLCDSetup.PluginActive)
    {
        mutex.Lock();

        channel.strTmp = "";

        osd.title = "";
        osd.items.clear();
        for (int i = 0; i < 4; i++)
            osd.colorButton[i] = "";
        osd.message = "";
        osd.textItem = "";

        mutex.Unlock();
        mDisplay->SetClear();
    }
}

void cGraphLCDState::OsdTitle(const char * Title)
{
    //printf("graphlcd plugin: cGraphLCDState::OsdTitle '%s'\n", Title);
    if (GraphLCDSetup.PluginActive)
    {
        mutex.Lock();

        osd.message = "";
        osd.title = "";
        if (Title)
        {
            osd.title = Title;
            // remove the time
            std::string::size_type pos = osd.title.find('\t');
            if (pos != std::string::npos)
                osd.title.resize(pos);
            osd.title = compactspace(osd.title);
        }

        mutex.Unlock();
        mDisplay->SetOsdTitle();
    }
}

void cGraphLCDState::OsdStatusMessage(const char * Message)
{
    //printf("graphlcd plugin: cGraphLCDState::OsdStatusMessage '%s'\n", Message);
    if (GraphLCDSetup.PluginActive)
    {
        if (GraphLCDSetup.ShowMessages)
        {
            mutex.Lock();

            if (Message)
                osd.message = compactspace(Message);
            else
                osd.message = "";

            mutex.Unlock();
            mDisplay->Update();
        }
    }
}

void cGraphLCDState::OsdHelpKeys(const char * Red, const char * Green, const char * Yellow, const char * Blue)
{
    //printf("graphlcd plugin: cGraphLCDState::OsdHelpKeys %s - %s - %s - %s\n", Red, Green, Yellow, Blue);
    if (GraphLCDSetup.PluginActive)
    {
        if (GraphLCDSetup.ShowColorButtons)
        {
            mutex.Lock();

            for (int i = 0; i < 4; i++)
                osd.colorButton[i] = "";

            if (Red)
                osd.colorButton[0] = compactspace(Red);
            if (Green)
                osd.colorButton[1] = compactspace(Green);
            if (Yellow)
                osd.colorButton[2] = compactspace(Yellow);
            if (Blue)
                osd.colorButton[3] = compactspace(Blue);

            mutex.Unlock();
        }
    }
}

void cGraphLCDState::OsdItem(const char * Text, int Index)
{
    //printf("graphlcd plugin: cGraphLCDState::OsdItem %s, %d\n", Text, Index);
    if (GraphLCDSetup.PluginActive)
    {
        if (GraphLCDSetup.ShowMenu)
        {
            mutex.Lock();

            osd.message = "";

            if (Text)
                osd.items.push_back(Text);

            mutex.Unlock();
            if (Text)
                mDisplay->SetOsdItem(Text);
        }
    }
}

void cGraphLCDState::OsdCurrentItem(const char * Text)
{
    //printf("graphlcd plugin: cGraphLCDState::OsdCurrentItem %s\n", Text);
    if (GraphLCDSetup.PluginActive)
    {
        if (GraphLCDSetup.ShowMenu)
        {
            int tabs;
            std::string::size_type pos;

            mutex.Lock();

            osd.message = "";
            osd.currentItem = "";
            if (Text)
            {
                osd.currentItem = Text;

                // count nr of tabs in text
                tabs = 0;
                for (unsigned int i = 0; i < osd.currentItem.length(); i++)
                {
                    if (osd.currentItem[i] == '\t')
                        tabs++;
                }
                if (tabs == 1)
                {
                    // only one tab => prob. Setup Menu
                    pos = osd.currentItem.find('\t');
                    osd.currentItemIndex = 0;
                    if (pos != std::string::npos)
                    {
                        for (unsigned int i = 0; i < osd.items.size(); i++)
                        {
                            if (osd.items[i].find(osd.currentItem.c_str(), 0, pos) == 0)
                            {
                                osd.currentItemIndex = i;
                                osd.items[i] = osd.currentItem;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    osd.currentItemIndex = 0;
                    for (unsigned int i = 0; i < osd.items.size(); i++)
                    {
                        if (osd.items[i].compare(osd.currentItem) == 0)
                        {
                            osd.currentItemIndex = i;
                            break;
                        }
                    }
                }
            }
            mutex.Unlock();
            if (Text)
                mDisplay->SetOsdCurrentItem();
        }
    }
}

void cGraphLCDState::OsdTextItem(const char * Text, bool Scroll)
{
    //printf("graphlcd plugin: cGraphLCDState::OsdTextItem %s %d\n", Text, Scroll);
    if (GraphLCDSetup.PluginActive)
    {
        mutex.Lock();
        if (Text)
        {
            osd.textItem = trim(Text);
#if 0
            // replace '\n' with ' '
            for (unsigned int i = 0; i < osd.textItem.length(); i++)
                if (osd.textItem[i] == '\n' && (i + 1) < osd.textItem.length() && osd.textItem[i + 1] != '\n')
                    osd.textItem[i] = ' ';
#endif
        }
        mutex.Unlock();
        mDisplay->SetOsdTextItem(Text, Scroll);
    }
}


void cGraphLCDState::OsdChannel(const char * Text)
{
    //printf("graphlcd plugin: cGraphLCDState::OsdChannel %s\n", Text);
    if (GraphLCDSetup.PluginActive)
    {
        mutex.Lock();
        if (Text)
        {
            channel.strTmp = Text;
            channel.strTmp = compactspace(channel.strTmp);
        }
        else
        {
            channel.strTmp = "";
        }
        mutex.Unlock();

        if (Text)
            mDisplay->Update();
    }
}

void cGraphLCDState::OsdProgramme(time_t PresentTime, const char * PresentTitle, const char * PresentSubtitle, time_t FollowingTime, const char * FollowingTitle, const char * FollowingSubtitle)
{
    //printf("graphlcd plugin: cGraphLCDState::OsdProgramme PT : %s\n", PresentTitle);
    //printf("graphlcd plugin: cGraphLCDState::OsdProgramme PST: %s\n", PresentSubtitle);
    //printf("graphlcd plugin: cGraphLCDState::OsdProgramme FT : %s\n", FollowingTitle);
    //printf("graphlcd plugin: cGraphLCDState::OsdProgramme FST: %s\n", FollowingSubtitle);
    if (GraphLCDSetup.PluginActive)
    {
        mutex.Lock();
        event.presentTime = PresentTime;
        event.presentTitle = "";
        if (!isempty(PresentTitle))
            event.presentTitle = PresentTitle;
        event.presentSubtitle = "";
        if (!isempty(PresentSubtitle))
            event.presentSubtitle = PresentSubtitle;

        event.followingTime = FollowingTime;
        event.followingTitle = "";
        if (!isempty(FollowingTitle))
            event.followingTitle = FollowingTitle;
        event.followingSubtitle = "";
        if (!isempty(FollowingSubtitle))
            event.followingSubtitle = FollowingSubtitle;
        mutex.Unlock();
        mDisplay->Update();
    }
}

void cGraphLCDState::SetChannel(int ChannelNumber)
{
    char tmp[16];

    if (ChannelNumber == 0)
        return;

    mutex.Lock();

    channel.number = ChannelNumber;
    cChannel * ch = Channels.GetByNumber(channel.number);
    channel.id = ch->GetChannelID();
    sprintf(tmp, "%d ", channel.number);
    channel.str = tmp;
    channel.str += ch->Name();
    event.presentTime = 0;
    event.followingTime = 0;

    mutex.Unlock();

    mDisplay->SetChannel(ChannelNumber);
}

void cGraphLCDState::GetProgramme()
{
    mutex.Lock();
#if VDRVERSNUM < 10300
    const cEventInfo * present = NULL, * following = NULL;
    cMutexLock mutexLock;
    const cSchedules * schedules = cSIProcessor::Schedules(mutexLock);
    if (channel.id.Valid())
    {
        if (schedules)
        {
            const cSchedule * schedule = schedules->GetSchedule(channel.id);
            if (schedule)
            {
                if ((present = schedule->GetPresentEvent()) != NULL)
                {
                    event.presentTime = present->GetTime();
                    event.presentTitle = "";
                    if (!isempty(present->GetTitle()))
                        event.presentTitle = present->GetTitle();
                    event.presentSubtitle = "";
                    if (!isempty(present->GetSubtitle()))
                        event.presentSubtitle = present->GetSubtitle();
                }
                if ((following = schedule->GetFollowingEvent()) != NULL)
                {
                    event.followingTime = following->GetTime();
                    event.followingTitle = "";
                    if (!isempty(following->GetTitle()))
                        event.followingTitle = following->GetTitle();
                    event.followingSubtitle = "";
                    if (!isempty(following->GetSubtitle()))
                        event.followingSubtitle = following->GetSubtitle();
                }
            }
        }
    }
#else
    const cEvent * present = NULL, * following = NULL;
    cSchedulesLock schedulesLock;
    const cSchedules * schedules = cSchedules::Schedules(schedulesLock);
    if (channel.id.Valid())
    {
        if (schedules)
        {
            const cSchedule * schedule = schedules->GetSchedule(channel.id);
            if (schedule)
            {
                if ((present = schedule->GetPresentEvent()) != NULL)
                {
                    event.presentTime = present->StartTime();
                    event.presentTitle = "";
                    if (!isempty(present->Title()))
                        event.presentTitle = present->Title();
                    event.presentSubtitle = "";
                    if (!isempty(present->ShortText()))
                        event.presentSubtitle = present->ShortText();
                }
                if ((following = schedule->GetFollowingEvent()) != NULL)
                {
                    event.followingTime = following->StartTime();
                    event.followingTitle = "";
                    if (!isempty(following->Title()))
                        event.followingTitle = following->Title();
                    event.followingSubtitle = "";
                    if (!isempty(following->ShortText()))
                        event.followingSubtitle = following->ShortText();
                }
            }
        }
    }
#endif
    mutex.Unlock();
}

tChannelState cGraphLCDState::GetChannelState()
{
    tChannelState ret;

    mutex.Lock();
    ret = channel;
    mutex.Unlock();

    return ret;
}

tEventState cGraphLCDState::GetEventState()
{
    tEventState ret;

    GetProgramme();
    mutex.Lock();
    ret = event;
    mutex.Unlock();

    return ret;
}

tReplayState cGraphLCDState::GetReplayState()
{
    tReplayState ret;

    mutex.Lock();

    if (tickUsed)
    {
        if (replay.control)
        {
            ret = replay;
            replay.currentLast = replay.current;
            replay.totalLast = replay.total;
        }
        else
        {
            ret = replay;
        }
    }
    else
    {
        if (replay.control)
        {
            if (replay.control->GetIndex(replay.current, replay.total, false))
            {
                replay.total = (replay.total == 0) ? 1 : replay.total;
                ret = replay;
                replay.currentLast = replay.current;
                replay.totalLast = replay.total;
            }
            else
            {
                replay.control = NULL;
                ret = replay;
            }
        }
        else
        {
            ret = replay;
        }
    }
    mutex.Unlock();

    return ret;
}

tCardState cGraphLCDState::GetCardState(int number)
{
    tCardState ret;

    mutex.Lock();
    ret = card[number];
    mutex.Unlock();

    return ret;
}

tOsdState cGraphLCDState::GetOsdState()
{
    tOsdState ret;

    mutex.Lock();
    ret = osd;
    mutex.Unlock();

    return ret;
}

tVolumeState cGraphLCDState::GetVolumeState()
{
    tVolumeState ret;

    mutex.Lock();
    ret = volume;
    mutex.Unlock();

    return ret;
}
