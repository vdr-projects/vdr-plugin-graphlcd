/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * skinconfig.c - skin config class that implements all the callbacks
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2011 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#include <glcdskin/config.h>
#include <glcdskin/type.h>
#include <glcdskin/string.h>

#include "common.h"
#include "display.h"
#include "state.h"
#include "skinconfig.h"
#include "service.h"

typedef enum _eTokenId
{
    // current channel
    tokPrivateChannelStart,
    tokChannelNumber,
    tokChannelName,
    tokChannelShortName,
    tokChannelProvider,
    tokChannelPortal,
    tokChannelSource,
    tokChannelID,
    tokHasTeletext,
    tokChannelHasTeletext,
    tokHasMultilang,
    tokChannelHasMultilang,
    tokHasDolby,
    tokChannelHasDolby,
    tokIsEncrypted,
    tokChannelIsEncrypted,
    tokIsRadio,
    tokChannelIsRadio,
    tokChannelAlias,
    tokPrivateChannelEnd,

    tokPrivateRecordingStart,
    tokIsRecording,
    tokRecordings,
    tokPrivateRecordingEnd,

    // present event
    tokPrivatePresentStart,
    tokPresentValid,
    tokPresentStartDateTime,
    tokPresentVpsDateTime,
    tokPresentEndDateTime,
    tokPresentDuration,
    tokPresentProgress,
    tokPresentRemaining,
    tokPresentTitle,
    tokPresentShortText,
    tokPresentDescription,
    tokPrivatePresentEnd,

    // following event
    tokPrivateFollowingStart,
    tokFollowingValid,
    tokFollowingStartDateTime,
    tokFollowingVpsDateTime,
    tokFollowingEndDateTime,
    tokFollowingDuration,
    tokFollowingTitle,
    tokFollowingShortText,
    tokFollowingDescription,
    tokPrivateFollowingEnd,

    // volume display
    tokPrivateVolumeStart,
    tokVolumeCurrent,
    tokVolumeTotal,
    tokIsMute,
    tokVolumeIsMute,
    tokPrivateVolumeEnd,

    tokPrivateReplayStart,
    tokReplayTitle,
    tokReplayPositionIndex,
    tokReplayDurationIndex,
    tokIsPlaying,
    tokReplayIsPlaying,
    tokIsFastForward,
    tokReplayIsFastForward,
    tokIsFastRewind,
    tokReplayIsFastRewind,
    tokIsSlowForward,
    tokReplayIsSlowForward,
    tokIsSlowRewind,
    tokReplayIsSlowRewind,
    tokIsPausing,
    tokReplayIsPausing,
    tokReplayPosition,
    tokReplayDuration,
    tokReplayRemaining,
    tokReplayMode,
    tokReplayIsShuffle,
    tokReplayIsLoop,
    tokPrivateReplayEnd,

    tokPrivateOsdStart,
    tokMessage,
    tokMenuTitle,
    tokMenuItem,
    tokMenuCurrent,
    tokIsMenuCurrent,
    tokButtonRed,
    tokButtonGreen,
    tokButtonYellow,
    tokButtonBlue,
    tokPrivateOsdEnd,

    tokDateTime,
    tokConfigPath,
    tokSkinPath,
    tokScreenWidth,
    tokScreenHeight,
    tokDefaultForegroundColor,
    tokDefaultBackgroundColor,

    tokPrivateSettingStart,
    tokSettingShowChannelLogo,
    tokSettingShowReplayLogo,
    tokSettingShowSymbols,
    tokSettingShowTimebar,

    tokScrollMode,
    tokScrollSpeed,
    tokScrollTime,
    tokBrightnessActive,
    tokBrightnessIdle,
    tokBrightnessDelay,
    tokDisplayMode,
    tokPrivateSettingEnd,

    // external services and data
    tokPrivateServiceStart,
    tokServiceIsAvailable,
    tokServiceItem,
    tokExtDataIsAvailable,
    tokExtDataItem,
    tokPrivateServiceEnd,

    tokCountToken
} eTokenId;

static const std::string Tokens[tokCountToken] =
{
    "privateChannelStart",
    "ChannelNumber",
    "ChannelName",
    "ChannelShortName",
    "ChannelProvider",
    "ChannelPortal",
    "ChannelSource",
    "ChannelID",
    "HasTeletext",
    "ChannelHasTeletext",
    "HasMultilang",
    "ChannelHasMultilang",
    "HasDolby",
    "ChannelHasDolby",
    "IsEncrypted",
    "ChannelIsEncrypted",
    "IsRadio",
    "ChannelIsRadio",
    "ChannelAlias",
    "privateChannelEnd",

    "privateRecordingStart",
    "IsRecording",
    "Recordings",
    "privateRecordingEnd",

    "privatePresentStart",
    "PresentValid",
    "PresentStartDateTime",
    "PresentVpsDateTime",
    "PresentEndDateTime",
    "PresentDuration",
    "PresentProgress",
    "PresentRemaining",
    "PresentTitle",
    "PresentShortText",
    "PresentDescription",
    "privatePresentEnd",

    "privateFollowingStart",
    "FollowingValid",
    "FollowingStartDateTime",
    "FollowingVpsDateTime",
    "FollowingEndDateTime",
    "FollowingDuration",
    "FollowingTitle",
    "FollowingShortText",
    "FollowingDescription",
    "privateFollowingEnd",

    "privateVolumeStart",
    "VolumeCurrent",
    "VolumeTotal",
    "IsMute",
    "VolumeIsMute",
    "privateVolumeEnd",

    "privateReplayStart",
    "ReplayTitle",
    "ReplayPositionIndex",
    "ReplayDurationIndex",
    "IsPlaying",
    "ReplayIsPlaying",
    "IsFastForward",
    "ReplayIsFastForward",
    "IsFastRewind",
    "ReplayIsFastRewind",
    "IsSlowForward",
    "ReplayIsSlowForward",
    "IsSlowRewind",
    "ReplayIsSlowRewind",
    "IsPausing",
    "ReplayIsPausing",
    "ReplayPosition",
    "ReplayDuration",
    "ReplayRemaining",
    "ReplayMode",
    "ReplayIsShuffle",
    "ReplayIsLoop",
    "privateReplayEnd",

    "privateOsdStart",
    "Message",
    "MenuTitle",
    "MenuItem",
    "MenuCurrent",
    "IsMenuCurrent",
    "ButtonRed",
    "ButtonGreen",
    "ButtonYellow",
    "ButtonBlue",
    "privateOsdEnd",

    "DateTime",
    "ConfigPath",
    "SkinPath",
    "ScreenWidth",
    "ScreenHeight",
    "DefaultForegroundColor",
    "DefaultBackgroundColor",

    "privateSettingStart",
    "SettingShowChannelLogo",
    "SettingShowReplayLogo",
    "SettingShowSymbols",
    "SettingShowTimebar",

    "ScrollMode",
    "ScrollSpeed",
    "ScrollTime",
    "BrightnessActive",
    "BrightnessIdle",
    "BrightnessDelay",
    "DisplayMode",
    "privateSettingEnd",

    // external services
    "privateServiceStart",
    "ServiceIsAvailable",
    "ServiceItem",
    "ExtDataIsAvailable",
    "ExtDataItem",
    "privateServiceEnd"
};

cGraphLCDSkinConfig::cGraphLCDSkinConfig(const cGraphLCDDisplay * Display, const std::string & CfgPath, const std::string & SkinsPath, const std::string & SkinName, cGraphLCDState * State)
{
    mDisplay = Display;
    mConfigPath = CfgPath;
    mSkinPath = SkinsPath + "/" + SkinName;
    mSkinName = SkinName;
    mState = State;
    mAliasList.Load(CfgPath);
}

cGraphLCDSkinConfig::~cGraphLCDSkinConfig()
{
}

std::string cGraphLCDSkinConfig::SkinPath(void)
{
    return mSkinPath;
}

std::string cGraphLCDSkinConfig::FontPath(void)
{
    return mConfigPath + "/fonts";
}

std::string cGraphLCDSkinConfig::CharSet(void)
{
#if APIVERSNUM >= 10503
    if (cCharSetConv::SystemCharacterTable()) {
      return cCharSetConv::SystemCharacterTable();
    } else {
      return "UTF-8";
    }
#else
    if (I18nCharSets()[Setup.OSDLanguage]) {
      return I18nCharSets()[Setup.OSDLanguage];
    } else {
      return "iso-8859-15";
    }
#endif
}

std::string cGraphLCDSkinConfig::Translate(const std::string & Text)
{
    return Text;
}

GLCD::cType cGraphLCDSkinConfig::GetToken(const GLCD::tSkinToken & Token)
{
    if (Token.Id > tokPrivateChannelStart && Token.Id < tokPrivateChannelEnd)
    {
        tChannel channel = mState->GetChannelInfo();
        switch (Token.Id)
        {
            case tokChannelNumber:
                return channel.number;
            case tokChannelName:
                return channel.name;
            case tokChannelShortName:
                return channel.shortName;
            case tokChannelProvider:
                return channel.provider;
            case tokChannelPortal:
                return channel.portal;
            case tokChannelSource:
                return channel.source;
            case tokChannelID:
                return (GLCD::cType) (const char *) channel.id.ToString();
            case tokHasTeletext:
            case tokChannelHasTeletext:
                return channel.hasTeletext;
            case tokHasMultilang:
            case tokChannelHasMultilang:
                return channel.hasMultiLanguage;
            case tokHasDolby:
            case tokChannelHasDolby:
                return channel.hasDolby;
            case tokIsEncrypted:
            case tokChannelIsEncrypted:
                return channel.isEncrypted;
            case tokIsRadio:
            case tokChannelIsRadio:
                return channel.isRadio;
            case tokChannelAlias:
            {
                char tmp[64];
                std::string alias;

                sprintf(tmp, "%d-%d-%d", channel.id.Nid(), channel.id.Tid(), channel.id.Sid());
                alias = mAliasList.GetAlias(tmp);
                return alias;
            }
            default:
                break;
        }
    }
    else if (Token.Id > tokPrivateRecordingStart && Token.Id < tokPrivateRecordingEnd)
    {
        switch (Token.Id)
        {
            case tokIsRecording:
            {
                if (Token.Attrib.Type == GLCD::aNumber)
                    return mState->IsRecording(Token.Attrib.Number);
                return mState->IsRecording(-1);
            }
            case tokRecordings:
            {
                if (Token.Attrib.Type == GLCD::aNumber)
                    return mState->Recordings(Token.Attrib.Number);
                return mState->Recordings(-1);
            }
            default:
                break;
        }
    }
    else if (Token.Id > tokPrivatePresentStart && Token.Id < tokPrivatePresentEnd)
    {
        tEvent event = mState->GetPresentEvent();
        switch (Token.Id)
        {
            case tokPresentValid:
                return event.valid;
            case tokPresentStartDateTime:
                return TimeType(event.startTime, Token.Attrib.Text);
            case tokPresentVpsDateTime:
                return TimeType(event.vpsTime, Token.Attrib.Text);
            case tokPresentEndDateTime:
                return TimeType(event.startTime + event.duration, Token.Attrib.Text);
            case tokPresentDuration:
#if VDRVERSNUM >= 10701
                return DurationType(event.duration * DEFAULTFRAMESPERSECOND, Token.Attrib.Text);
#else
                return DurationType(event.duration * FRAMESPERSEC, Token.Attrib.Text);
#endif
            case tokPresentProgress:
#if VDRVERSNUM >= 10701
                return DurationType((time(NULL) - event.startTime) * DEFAULTFRAMESPERSECOND, Token.Attrib.Text);
#else
                return DurationType((time(NULL) - event.startTime) * FRAMESPERSEC, Token.Attrib.Text);
#endif
            case tokPresentRemaining:
                if ((time(NULL) - event.startTime) < event.duration)
                {
#if VDRVERSNUM >= 10701
                    return DurationType((event.duration - (time(NULL) - event.startTime)) * DEFAULTFRAMESPERSECOND, Token.Attrib.Text);
#else
                    return DurationType((event.duration - (time(NULL) - event.startTime)) * FRAMESPERSEC, Token.Attrib.Text);
#endif
                }
                return false;
            case tokPresentTitle:
                return event.title;
            case tokPresentShortText:
                return event.shortText;
            case tokPresentDescription:
                return event.description;
            default:
                break;
        }
    }
    else if (Token.Id > tokPrivateFollowingStart && Token.Id < tokPrivateFollowingEnd)
    {
        tEvent event = mState->GetFollowingEvent();
        switch (Token.Id)
        {
            case tokFollowingValid:
                return event.valid;
            case tokFollowingStartDateTime:
                return TimeType(event.startTime, Token.Attrib.Text);
            case tokFollowingVpsDateTime:
                return TimeType(event.vpsTime, Token.Attrib.Text);
            case tokFollowingEndDateTime:
                return TimeType(event.startTime + event.duration, Token.Attrib.Text);
            case tokFollowingDuration:
#if VDRVERSNUM >= 10701
                return DurationType(event.duration * DEFAULTFRAMESPERSECOND, Token.Attrib.Text);
#else
                return DurationType(event.duration * FRAMESPERSEC, Token.Attrib.Text);
#endif
            case tokFollowingTitle:
                return event.title;
            case tokFollowingShortText:
                return event.shortText;
            case tokFollowingDescription:
                return event.description;
            default:
                break;
        }
    }
    else if (Token.Id > tokPrivateVolumeStart && Token.Id < tokPrivateVolumeEnd)
    {
        tVolumeState volume = mState->GetVolumeState();
        switch (Token.Id)
        {
            case tokVolumeCurrent:
                return volume.value;
            case tokVolumeTotal:
                return 255;
            case tokIsMute:
            case tokVolumeIsMute:
                return volume.value == 0;
            default:
                break;
        }
    }
    else if (Token.Id > tokPrivateReplayStart && Token.Id < tokPrivateReplayEnd)
    {
        tReplayState replay = mState->GetReplayState();
        switch (Token.Id)
        {
            case tokReplayTitle:
                return replay.name;
            case tokReplayPositionIndex:
                return DurationType(replay.current, Token.Attrib.Text);
            case tokReplayDurationIndex:
                return DurationType(replay.total, Token.Attrib.Text);
            case tokReplayPosition:
                return replay.current;
            case tokReplayDuration:
                return replay.total;
            case tokReplayRemaining:
                return DurationType(replay.total - replay.current, Token.Attrib.Text);
            case tokIsPlaying:
            case tokReplayIsPlaying:
                return replay.play && replay.speed == -1;
            case tokIsPausing:
            case tokReplayIsPausing:
                return !replay.play && replay.speed == -1;
            case tokIsFastForward:
            case tokReplayIsFastForward:
                if (replay.play && replay.forward && replay.speed != -1)
                {
                    return Token.Attrib.Type == GLCD::aNumber
                        ? (GLCD::cType) (replay.speed == Token.Attrib.Number)
                        : (GLCD::cType) true;
                }
                return false;
            case tokIsFastRewind:
            case tokReplayIsFastRewind:
                if (replay.play && !replay.forward && replay.speed != -1)
                {
                    return Token.Attrib.Type == GLCD::aNumber
                        ? (GLCD::cType) (replay.speed == Token.Attrib.Number)
                        : (GLCD::cType) true;
                }
                return false;
            case tokIsSlowForward:
            case tokReplayIsSlowForward:
                if (!replay.play && replay.forward && replay.speed != -1)
                {
                    return Token.Attrib.Type == GLCD::aNumber
                        ? (GLCD::cType) (replay.speed == Token.Attrib.Number)
                        : (GLCD::cType) true;
                }
                return false;
            case tokIsSlowRewind:
            case tokReplayIsSlowRewind:
                if (!replay.play && !replay.forward && replay.speed != -1)
                {
                    return Token.Attrib.Type == GLCD::aNumber
                        ? (GLCD::cType) (replay.speed == Token.Attrib.Number)
                        : (GLCD::cType) true;
                }
                return false;
            case tokReplayMode:
                switch (replay.mode)
                {
                    case eReplayAudioCD:
                        return "cd";
                    case eReplayDVD:
                        return "dvd";
                    case eReplayFile:
                        return "file";
                    case eReplayImage:
                        return "image";
                    case eReplayMusic:
                        return "music";
                    default:
                        return "vdr";
                }
            case tokReplayIsShuffle:
            case tokReplayIsLoop:
            default:
                break;
        }
    }
    else if (Token.Id > tokPrivateOsdStart && Token.Id < tokPrivateOsdEnd)
    {
        tOsdState osd = mState->GetOsdState();
        switch (Token.Id)
        {
            case tokMessage:
                return osd.message;
            case tokMenuTitle:
                return osd.title;
            case tokMenuItem:
            case tokMenuCurrent:
            case tokIsMenuCurrent:
            {
                if (osd.items.size() == 0
                  || osd.currentItemIndex == -1)
                {
                    return false;
                }
                int maxItems = Token.MaxItems;
                if (maxItems > (int) osd.items.size())
                    maxItems = osd.items.size();
                int currentIndex = maxItems / 2;
                if (osd.currentItemIndex < currentIndex)
                    currentIndex = osd.currentItemIndex;
                int topIndex = osd.currentItemIndex - currentIndex;
                if ((topIndex + maxItems) > (int) osd.items.size())
                {
                    currentIndex += (topIndex + maxItems) - osd.items.size();
                    topIndex = osd.currentItemIndex - currentIndex;
                }
                if (Token.Id == tokMenuItem)
                {
                    if (Token.Index < maxItems && Token.Index != currentIndex)
                        return osd.items[topIndex + Token.Index];
                }
                else if (Token.Id == tokMenuCurrent)
                {
                    if (Token.Index < maxItems && Token.Index == currentIndex)
                        return osd.items[topIndex + Token.Index];
                }
                else if (Token.Id == tokIsMenuCurrent)
                {
                    if (Token.Index < maxItems && Token.Index == currentIndex)
                        return true;
                }
                return false;
            }
            case tokButtonRed:
                return osd.redButton;
            case tokButtonGreen:
                return osd.greenButton;
            case tokButtonYellow:
                return osd.yellowButton;
            case tokButtonBlue:
                return osd.blueButton;
            default:
                break;
        }
    }
    else if (Token.Id > tokPrivateSettingStart && Token.Id < tokPrivateSettingEnd)
    {
        switch (Token.Id)
        {
            case tokSettingShowChannelLogo:
                if (GraphLCDSetup.ShowChannelLogo)
                    return true;
                return false;
            case tokSettingShowReplayLogo:
                if (GraphLCDSetup.ShowReplayLogo)
                    return true;
                return false;
            case tokSettingShowSymbols:
                if (GraphLCDSetup.ShowSymbols)
                    return true;
                return false;
            case tokSettingShowTimebar:
                if (GraphLCDSetup.ShowTimebar)
                    return true;
                return false;

            case tokScrollMode:
                return GraphLCDSetup.ScrollMode;
            case tokScrollSpeed:
                return GraphLCDSetup.ScrollSpeed;
            case tokScrollTime:
                return GraphLCDSetup.ScrollTime;
            case tokBrightnessActive:
                return GraphLCDSetup.BrightnessActive;
            case tokBrightnessIdle:
                return GraphLCDSetup.BrightnessIdle;
            case tokBrightnessDelay:
                return GraphLCDSetup.BrightnessDelay;
            case tokDisplayMode:
                switch (mDisplay->GetDisplayMode())
                {
                    case DisplayModeNormal:
                        return "Normal";
                    case DisplayModeInteractive:
                        return "Interactive";
                    default:
                        return "Normal";
                }

            default:
                break;
        }
    }
    else if (Token.Id > tokPrivateServiceStart && Token.Id < tokPrivateServiceEnd)
    {
        cGraphLCDService * s = (cGraphLCDService*)mDisplay->GetServiceObject();
        switch (Token.Id)
        {
            case tokServiceIsAvailable: {
                if (Token.Attrib.Text == "")
                    return false;

                return s->ServiceIsAvailable(Token.Attrib.Text);
            }
            break;
            case tokServiceItem: {
                size_t found = Token.Attrib.Text.find(",");
                std::string ServiceName = "";
                std::string ItemName = "";

                if (found != std::string::npos) {
                    ServiceName = Token.Attrib.Text.substr(0, found);
                    ItemName = Token.Attrib.Text.substr(found+1);
                } else {
                    ServiceName = Token.Attrib.Text;
                }
                return s->GetItem(ServiceName, ItemName);
            }
            break;
            case tokExtDataIsAvailable: {
                if (Token.Attrib.Text == "")
                    return false;

                return mDisplay->GetExtData()->IsSet( Token.Attrib.Text );
            }
            break;
            case tokExtDataItem: {
                if (Token.Attrib.Text == "")
                    return false;

                return mDisplay->GetExtData()->Get( Token.Attrib.Text );
            }
            break;
            default:
                break;
        }
    }
    else
    {
        switch (Token.Id)
        {
            case tokDateTime:
                return TimeType(time(NULL), Token.Attrib.Text);
            case tokConfigPath:
                return mConfigPath;
            case tokSkinPath:
                return mSkinPath;
            case tokScreenWidth:
            {
                const GLCD::cBitmap * bitmap = mDisplay->GetScreen();
                return bitmap->Width();
            }
            case tokScreenHeight:
            {
                const GLCD::cBitmap * bitmap = mDisplay->GetScreen();
                return bitmap->Height();
            }
            case tokDefaultForegroundColor:
            {
                char buffer[11];
                snprintf(buffer, 10, "0x%08x", (uint32_t)((mDisplay)->GetDriver()->GetForegroundColor()));
                return buffer;
            }
            case tokDefaultBackgroundColor:
            {
                char buffer[11];
                snprintf(buffer, 10, "0x%08x", (uint32_t)((mDisplay)->GetDriver()->GetBackgroundColor()));
                return buffer;
            }
            default:
                break;
        }
    }
    return "";
}

int cGraphLCDSkinConfig::GetTokenId(const std::string & Name)
{
    int i;

    for (i = 0; i < tokCountToken; i++)
    {
        if (Name == Tokens[i])
            return i;
    }
    esyslog("graphlcd: unknown token %s", Name.c_str());
    return tokCountToken;
}

int cGraphLCDSkinConfig::GetTabPosition(int Index, int MaxWidth, const GLCD::cFont & Font)
{
    if (mTabs.size() == 0)
    {
        int i;
        tOsdState osd = mState->GetOsdState();

        for (i = 0; i < (int) osd.items.size(); i++)
        {
            int iTab, t;
            std::string str;
            std::string::size_type pos1, pos2;

            str = osd.items[i];
            pos1 = 0;
            pos2 = str.find('\t');
            iTab = 0;
            while (pos1 < str.length() && pos2 != std::string::npos)
            {
                t = Font.Width(str.substr(pos1), pos2 - pos1);
                if (iTab == 0 && t > (MaxWidth * 66) / 100)
                    t = (MaxWidth * 66) / 100;
                if (iTab < (int) mTabs.size())
                {
                    if (mTabs[iTab] < t)
                        mTabs[iTab] = t;
                }
                else
                {
                    mTabs.push_back(t);
                }
                pos1 = pos2 + 1;
                pos2 = str.find('\t', pos1);
                iTab++;
            }
        }
    }

    if (Index < (int) mTabs.size())
        return mTabs[Index];
    return 0;
}

void cGraphLCDSkinConfig::SetMenuClear()
{
    mTabs.clear();
}


uint64_t cGraphLCDSkinConfig::Now(void)
{
    return cTimeMs::Now();
}

GLCD::cDriver * cGraphLCDSkinConfig::GetDriver(void) const
{
    return (mDisplay) ? (mDisplay)->GetDriver() : NULL;
}
