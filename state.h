/**
 *  GraphLCD plugin for the Video Disk Recorder
 *
 *  state.h  -  status monitor class
 *
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online de>
 *  (c)      2010 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 **/

#ifndef _GRAPHLCD_STATE_H_
#define _GRAPHLCD_STATE_H_

#include <map>
#include <string>

#include <vdr/status.h>


struct tChannel
{
    tChannelID id;
    int number;
    std::string name;
    std::string shortName;
    std::string provider;
    std::string portal;
    std::string source;
    bool hasTeletext;
    bool hasMultiLanguage;
    bool hasDolby;
    bool isEncrypted;
    bool isRadio;
};

struct tEvent
{
    bool valid;
    time_t startTime;
    time_t vpsTime;
    int duration;
    std::string title;
    std::string shortText;
    std::string description;
};

enum eReplayMode
{
    eReplayNormal,
    eReplayMusic,
    eReplayDVD,
    eReplayFile,
    eReplayImage,
    eReplayAudioCD
};

struct tReplayState
{
    std::string name;
    std::string loopmode;
    cControl * control;
    eReplayMode mode;
    int current;
    int total;
    bool play;
    bool forward;
    int speed;
};

struct tRecording
{
    int deviceNumber;
    std::string name;
    std::string fileName;
};

struct tOsdState
{
    std::string currentItem;
    std::vector <std::string> items;
    std::string title;
    std::string redButton;
    std::string greenButton;
    std::string yellowButton;
    std::string blueButton;
    std::string textItem;
    std::string message;
    int currentItemIndex;
};

struct tVolumeState
{
    int value;
    uint64_t lastChange;
};

// Radiotext
struct RadioTextService_v1_0 {
  int rds_info;
  int rds_pty;
  char *rds_text;
  char *rds_title;
  char *rds_artist;
  struct tm *title_start;
};

// LcrData
struct LcrService_v1_0 {
  cString destination;
  cString price;
  cString pulse;
};

class cGraphLCDDisplay;

class cGraphLCDState : public cStatus
{
private:
    cGraphLCDDisplay * mDisplay;
    bool first;
    bool tickUsed;

    cMutex mutex;

    tChannel mChannel;
    tEvent mPresent;
    tEvent mFollowing;
    tReplayState mReplay;
    std::vector <tRecording> mRecordings;
    tOsdState mOsd;
    tVolumeState mVolume;

    RadioTextService_v1_0  checkRTSData, currRTSData;
    LcrService_v1_0        checkLcrData, currLcrData;
    bool  rtsChanged, rtsActive;
    bool  lcrChanged, lcrActive;

    void SetChannel(int ChannelNumber);
    void UpdateChannelInfo(void);
    void UpdateEventInfo(void);
    void UpdateReplayInfo(void);
protected:
    virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber);
    virtual void Recording(const cDevice *Device, const char *Name, const char *FileName, bool On);
    virtual void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On);
    virtual void SetVolume(int Volume, bool Absolute);
    virtual void OsdClear();
    virtual void OsdTitle(const char *Title);
    virtual void OsdStatusMessage(const char *Message);
    virtual void OsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue);
    virtual void OsdItem(const char *Text, int Index);
    virtual void OsdCurrentItem(const char *Text);
    virtual void OsdTextItem(const char *Text, bool Scroll);
    virtual void OsdChannel(const char *Text);
    virtual void OsdProgramme(time_t PresentTime, const char *PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle);

public:
    cGraphLCDState(cGraphLCDDisplay * Display);
    virtual ~cGraphLCDState();

    void Update();
    void Tick();
    tChannel GetChannelInfo();
    tEvent GetPresentEvent();
    tEvent GetFollowingEvent();
    tReplayState GetReplayState();
    bool IsRecording(int CardNumber);
    std::string Recordings(int CardNumber);
    tOsdState GetOsdState();
    tVolumeState GetVolumeState();
    bool ShowMessage();

    bool CheckServiceEventUpdate();
};

#endif
