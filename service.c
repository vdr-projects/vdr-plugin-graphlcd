/**
 *  GraphLCD plugin for the Video Disk Recorder
 *
 *  service.h  -  class for events from external services
 *
 *  (c) 2010-2012 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 *
 *  mailbox-contribution: user 'Keine_Ahnung'
 **/

#include "strfct.h"
#include <limits>

#include "service.h"
#include "common.h"

#include <vdr/plugin.h>

#include <map>


const char* pluginName = "graphlcd"; // for span

cGraphLCDService::cGraphLCDService(cGraphLCDState * state)
{
    mState = state;

    /* initialise flags for services */
    radioActive    = false;    radioChanged   = false;    radioUse     = false;   radioInit   = false;
    lcrActive      = false;    lcrChanged     = false;    lcrUse       = false;   lcrInit     = false;
    femonActive    = false;    femonChanged   = false;    femonUse     = false;   femonInit   = false;
    mailboxActive  = false;    mailboxChanged = false;    mailboxUse   = false;   mailboxInit = false;
    spanActive     = false;    spanChanged    = false;    spanUse      = false;   spanInit    = false;

    radioLastChange = lcrLastChange = femonLastChange = mailboxLastChange = spanLastChange = 0;

    femonVersionChecked = femonVersionValid = false;

    // default min. change delays
    radioUpdateDelay   = 100;   // 100 ms
    lcrUpdateDelay     = 60000; // 60 sec
    femonUpdateDelay   = 2000;  // 2 sec
    mailboxUpdateDelay = 10000; // 10 sec
    spanUpdateDelay    = 500;   // 1/2 sec

    // pre-init strings
    checkRTSData.rds_text    = (char*)"";
    checkRTSData.rds_title   = (char*)"";
    checkRTSData.rds_artist  = (char*)"";

    checkLcrData.destination = (char*)"";
    checkLcrData.price       = (char*)"";
    checkLcrData.pulse       = (char*)"";

#if 0
    currFemonData.fe_name    = (char*)"";
    currFemonData.fe_status  = (char*)"";
#endif

    // pre-init span stuff (real initialisation will happen at 1st call of ServiceIsAvailable())
    checkSpanData.bands                      = 0;
    checkSpanData.barHeights                 = NULL;
    checkSpanData.barHeightsLeftChannel      = NULL;
    checkSpanData.barHeightsRightChannel     = NULL;
    checkSpanData.volumeLeftChannel          = new unsigned int[1];
    checkSpanData.volumeRightChannel         = new unsigned int[1];
    checkSpanData.volumeBothChannels         = new unsigned int[1];
    checkSpanData.name                       = pluginName;
    checkSpanData.falloff                    = 8;
    checkSpanData.barPeaksBothChannels       = NULL;
    checkSpanData.barPeaksLeftChannel        = NULL;
    checkSpanData.barPeaksRightChannel       = NULL;

    currSpanData.volumeLeftChannel           = new unsigned int[1];
    currSpanData.volumeRightChannel          = new unsigned int[1];
    currSpanData.volumeBothChannels          = new unsigned int[1];
}

cGraphLCDService::~cGraphLCDService()
{
}


bool cGraphLCDService::ServiceIsAvailable(const std::string & Name, const std::string & Options) {
    bool rvAvail = false;
    bool firstTime = false;

    if (Name == "RadioTextService-v1.0" || Name == "radio") {
        if (!radioInit) {
            radioInit = true;
            firstTime = true;
            radioUse = true;
        }
        rvAvail = (mState->GetChannelInfo().isRadio) ? radioActive : false;
    } else if (Name == "LcrService-v1.0" || Name == "lcr") {
        if (!lcrInit) {
            lcrInit = true;
            firstTime = true;
            lcrUse = true;
        }
        rvAvail = lcrActive;
    } else if (Name == "FemonService-v1.0" || Name == "femon") {
        if (!femonInit) {
            femonInit = true;
            firstTime = true;
            femonUse = true;
        }
        rvAvail = femonActive;
    } else if (Name == "MailBox-1.0" || Name == "mailbox") {
        if (!mailboxInit) {
            mailboxInit = true;
            firstTime = true;
            mailboxUse = true;
        }
        rvAvail = mailboxActive;
    } else if (Name == "span") {
        if (!spanInit) {
            spanInit = true;
            firstTime = true;
            spanUse = true;
        }
        //spanUse = true;
        rvAvail = spanActive;
    }


    if (firstTime) {
        std::map<std::string, std::string> opts;
        std::map<std::string, std::string>::iterator it;
    
        if (Options.length() > 0) {
            size_t pos = 0;
            while (pos < Options.length() && pos != std::string::npos) {
                size_t start = pos;
                pos = Options.find(",", start);

                std::string curropt = (pos != std::string::npos) ? Options.substr(start, pos-start) : Options.substr(start);
                size_t delim = curropt.find("=");
                if ( (delim != std::string::npos) && (delim < curropt.length()-1) ) {
                    opts[curropt.substr(0, delim)] = curropt.substr(delim+1);
                }

                if (pos != std::string::npos)
                    pos++;
            }
        }

        // options valid for all services
        it = opts.find("delay");
        if (it != opts.end()) {
            int delay = atoi( (*it).second.c_str() );
            if (delay >= 100) {
                SetServiceUpdateDelay(Name, delay);
            }
        }

        // indiv. options
        if (Name == "span") {
            int bands = 0; // default: no bands, only volume
            it = opts.find("bands");
            if (it != opts.end()) {
                bands = atoi( (*it).second.c_str() );
            }
            if (bands < 0 || bands > 100)
                bands = 0;

            if (bands != 0) {
                checkSpanData.bands                      = bands;
                checkSpanData.barHeights                 = new unsigned int[bands];
                checkSpanData.barHeightsLeftChannel      = new unsigned int[bands];
                checkSpanData.barHeightsRightChannel     = new unsigned int[bands];
                checkSpanData.barPeaksBothChannels       = new unsigned int[bands];
                checkSpanData.barPeaksLeftChannel        = new unsigned int[bands];
                checkSpanData.barPeaksRightChannel       = new unsigned int[bands];                

                currSpanData.barHeights                  = new unsigned int[bands];
                currSpanData.barHeightsLeftChannel       = new unsigned int[bands];
                currSpanData.barHeightsRightChannel      = new unsigned int[bands];
                currSpanData.barPeaksBothChannels        = new unsigned int[bands];
                currSpanData.barPeaksLeftChannel         = new unsigned int[bands];
                currSpanData.barPeaksRightChannel        = new unsigned int[bands];
            }

            it = opts.find("falloff");
            if (it != opts.end()) {  // if no option 'falloff': use preset value
                int falloff = atoi( (*it).second.c_str() );
                if (falloff > 0 && falloff < 100)
                    checkSpanData.falloff = falloff;
            }
        }
    }

    return rvAvail;
}


void cGraphLCDService::SetServiceUpdateDelay(const std::string & Name, int delay) {
    if (delay < 100)
        return;

    if (Name == "RadioTextService-v1.0" || Name == "radio") {
        radioUpdateDelay = delay;
    } else if (Name == "LcrService-v1.0" || Name == "lcr") {
        lcrUpdateDelay = delay;
    } else if (Name == "FemonService-v1.0" || Name == "femon") {
        femonUpdateDelay = delay;
    } else if (Name == "MailBox-1.0" || Name == "mailbox") {
        mailboxUpdateDelay = delay;
    } else if (Name == "span") {
        spanUpdateDelay = delay;
    }
}


GLCD::cType cGraphLCDService::GetItem(const std::string & ServiceName, const std::string & Item = "")
{
    size_t found = 0;
    std::string ItemName = Item;
    std::string ItemFormat = "";
    std::string ItemExtra = "";

    if (ItemName != "") {
        found = Item.find(",");
        if (found != std::string::npos) {
            ItemName = Item.substr(0, found);
            ItemFormat = Item.substr(found+1);
            if ( (found = ItemFormat.find(",")) != std::string::npos) {
                ItemExtra = ItemFormat.substr(found+1);
                ItemFormat = ItemFormat.substr(0, found);
            }
        } else {
            ItemName = Item;
        }
    }

    //NeedsUpdate();
    if (ServiceName == "RadioTextService-v1.0" || ServiceName == "radio") {
        radioUse = true;
        if (radioActive) {
            if (ItemName == "" || ItemName == "default") {
                if (currRTSData.rds_info == 2 && strstr(currRTSData.rds_title, "---") == NULL) {
                    char rtpinfo[2][65], rtstr[140];
                    strcpy(rtpinfo[0], currRTSData.rds_title);
                    strcpy(rtpinfo[1], currRTSData.rds_artist);
                    sprintf(rtstr, "%02d:%02d  %s | %s", currRTSData.title_start->tm_hour, currRTSData.title_start->tm_min, trim(((std::string)(rtpinfo[0]))).c_str(), trim(((std::string)(rtpinfo[1]))).c_str());
                    return rtstr;
                } else if (currRTSData.rds_info > 0) {
                    return trim(currRTSData.rds_text);
                }
            } else if (ItemName == "title") {
                    return trim(currRTSData.rds_title);
            } else if (ItemName == "artist") {
                    return trim(currRTSData.rds_artist);
            } else if (ItemName == "text") {
                    return trim(currRTSData.rds_text);
            } else if (ItemName == "rds_info" || ItemName == "info") {
                    return (int)(currRTSData.rds_info);
            } else if (ItemName == "rds_pty" || ItemName == "pty") {
                    return (int)(currRTSData.rds_pty);
            }
        }

    } else if (ServiceName == "LcrService-v1.0" || ServiceName == "lcr") {
        lcrUse = true;
        if (lcrActive) {
            if ( strstr( currLcrData.destination, "---" ) == NULL ) {
                char lcrStringParts[3][25], lcrString[100];
                strcpy( lcrStringParts[0], (const char *)currLcrData.destination );
                strcpy( lcrStringParts[1], (const char *)currLcrData.price );
                strcpy( lcrStringParts[2], (const char *)currLcrData.pulse );
                sprintf(lcrString, "%s | %s", trim((std::string)(lcrStringParts[1])).c_str(), trim((std::string)(lcrStringParts[2])).c_str());
                if (ItemName == "" || ItemName == "default") {
                    return trim(lcrString);
                } else if (ItemName == "destination") {
                    return trim(lcrStringParts[0]);
                } else if (ItemName == "price") {
                    return trim(lcrStringParts[1]);
                } else if (ItemName == "pulse") {
                    return trim(lcrStringParts[2]);
                }
            }
        }
    } else if (ServiceName == "FemonService-v1.0" || ServiceName == "femon") {
        femonUse = true;
        if (femonActive) {
            if (ItemName == "" || ItemName == "default" || ItemName == "status") {
                return (const char*)currFemonData.fe_status;
            } else if (ItemName == "name") {
                return (const char*)currFemonData.fe_name;
            } else if (ItemName == "snr") {
                return (int)currFemonData.fe_snr;
            } else if (ItemName == "signal") {
                return (int)currFemonData.fe_signal;
            } else if (ItemName == "percent_snr") {
                return (int)((currFemonData.fe_snr * 100) / 65535);
            } else if (ItemName == "percent_signal") {
                return (int)((currFemonData.fe_signal * 100) / 65535);
            } else if (ItemName == "ber") {
                return (int)currFemonData.fe_ber;
            } else if (ItemName == "unc") {
                return (int)currFemonData.fe_unc;
            } else if (ItemName == "video_bitrate" || ItemName == "vbr") {
                if (ItemFormat == "" && (ItemExtra == "" || ItemExtra == "0")) {
                    return (int)currFemonData.video_bitrate;
                } else {
                    double divisor = (ItemExtra == "") ? 1.0 : atof(ItemExtra.c_str());
                    double value = currFemonData.video_bitrate;
                    if (ItemFormat == "") {
                       return (int)(value / divisor);
                    } else {
                       char buf[20];
                       snprintf(buf, 19, ItemFormat.c_str(), (value / divisor));
                       return buf;
                    }
                }
            } else if (ItemName == "audio_bitrate" || ItemName == "abr") {
                if (ItemFormat == "" && (ItemExtra == "" || ItemExtra == "0")) {
                    return (int)currFemonData.audio_bitrate;
                } else {
                    double divisor = (ItemExtra == "") ? 1 : atof(ItemExtra.c_str());
                    double value = currFemonData.audio_bitrate;
                    if (ItemFormat == "") {
                       return (int)(value / divisor);
                    } else {
                       char buf[20];
                       snprintf(buf, 19, ItemFormat.c_str(), (value / divisor));
                       return buf;
                    }
                }
            } else if (ItemName == "dolby_bitrate" || ItemName == "dbr") {
                if (ItemFormat == "" && (ItemExtra == "" || ItemExtra == "0")) {
                    return (int)currFemonData.dolby_bitrate;
                } else {
                    double divisor = (ItemExtra == "") ? 1 : atof(ItemExtra.c_str());
                    double value = currFemonData.dolby_bitrate;
                    if (ItemFormat == "") {
                       return (int)(value / divisor);
                    } else {
                       char buf[20];
                       snprintf(buf, 19, ItemFormat.c_str(), (value / divisor));
                       return buf;
                    }
                }
            }
        }
    } else if (ServiceName == "MailBox-1.0" || ServiceName == "mailbox") {
        mailboxUse = true;
        if (mailboxActive) {
            if (ItemName == "" || ItemName == "default"|| ItemName == "hasnew") {
                return (bool)currMailboxNewData;
            } else if (ItemName == "newcount") {
                if (currMailboxUnseenData > (unsigned long)std::numeric_limits<int>::max()) {
                    return (int)-1;
                } else {
                    return (int)currMailboxUnseenData;
                }
            }
        }
    } else if (ServiceName == "span") {
        spanUse = true;
        if (spanActive) {
            int index = 0;
            if (ItemFormat != "") {
               index = atoi(ItemFormat.c_str());
            }
            if (ItemName == "" || ItemName == "default" || ItemName == "VolumeBoth" || ItemName == "volume") {
                return (int)checkSpanData.volumeBothChannels[0];
            } else if (ItemName == "VolumeLeft" || ItemName == "volumel") {
                return (int)checkSpanData.volumeLeftChannel[0];
            } else if (ItemName == "VolumeRight" || ItemName == "volumer") {
                return (int)checkSpanData.volumeRightChannel[0];
            } else if (ItemName == "Bands" || ItemName == "bands") {
                return (int)checkSpanData.bands;
            } else if (ItemName == "Falloff" || ItemName == "falloff") {
                return (int)checkSpanData.falloff;
            } else if (ItemName == "BarHeight" || ItemName == "height") {
                return (int)checkSpanData.barHeights[index];
            } else if (ItemName == "BarHeightLeft" || ItemName == "heightl") {
                return (int)checkSpanData.barHeightsLeftChannel[index];
            } else if (ItemName == "BarHeightRight" || ItemName == "heightr") {
                return (int)checkSpanData.barHeightsRightChannel[index];
            } else if (ItemName == "BarPeak" || ItemName == "peak") {
                return (int)checkSpanData.barPeaksBothChannels[index];
            } else if (ItemName == "BarPeakLeft" || ItemName == "peakl") {
                return (int)checkSpanData.barPeaksLeftChannel[index];
            } else if (ItemName == "BarPeakRight" || ItemName == "peakr") {
                return (int)checkSpanData.barPeaksRightChannel[index];
            }
        }
    }

    return "";
}



/* async. check event updates for services from other plugins */
/* only sets flags but does NOT update display output */
bool cGraphLCDService::NeedsUpdate(uint64_t CurrentTime)
{
    //mutex.Lock();

    /*radioActive  = false;*/    radioChanged   = false;
    /*lcrActive    = false;*/    lcrChanged     = false;
    /*femonActive  = false;*/    femonChanged   = false;
                                 mailboxChanged = false;
                                 spanChanged    = false;

    cPlugin *p = NULL;

    // Radiotext
    // only ask if radio-services are defined in the skin and min. request delay exceeded
    if (radioUse && ((CurrentTime-radioLastChange) >= (uint64_t)radioUpdateDelay)) {
        radioLastChange = CurrentTime;
        p = cPluginManager::CallFirstService("RadioTextService-v1.0", NULL);
        if (p) {
            checkRTSData.rds_text    = NULL;
            checkRTSData.rds_title   = NULL;
            checkRTSData.rds_artist  = NULL;

            if (cPluginManager::CallFirstService("RadioTextService-v1.0", &checkRTSData)) {
                radioActive = true;
                if (
                        (currRTSData.rds_info != checkRTSData.rds_info) ||
                        (currRTSData.rds_pty != checkRTSData.rds_pty) ||
                        (ParanoiaStrcmp(currRTSData.rds_text, checkRTSData.rds_text) != 0) ||
                        (ParanoiaStrcmp(currRTSData.rds_title, checkRTSData.rds_title) != 0) ||
                        (ParanoiaStrcmp(currRTSData.rds_artist, checkRTSData.rds_artist) != 0)
                  )
                {
                    currRTSData.rds_info   = checkRTSData.rds_info;
                    currRTSData.rds_pty    = checkRTSData.rds_pty;
                    currRTSData.rds_text   = checkRTSData.rds_text;
                    currRTSData.rds_title  = checkRTSData.rds_title;
                    currRTSData.rds_artist = checkRTSData.rds_artist;
                    currRTSData.title_start= checkRTSData.title_start;

                    radioChanged = true;
                }
            }
        }
    }

    // Lcr
    // only ask if lcr-services are defined in the skin and min. request delay exceeded
    if (lcrUse && ((CurrentTime-lcrLastChange) >= (uint64_t)lcrUpdateDelay)) {
        lcrLastChange = CurrentTime;
        p = cPluginManager::CallFirstService("LcrService-v1.0", NULL);
        if (p) {
            checkLcrData.destination = NULL;
            checkLcrData.price       = NULL;
            checkLcrData.pulse       = NULL;

            if (cPluginManager::CallFirstService("LcrService-v1.0", &checkLcrData)) {
                lcrActive = true;
                if (
                        (ParanoiaStrcmp(currLcrData.destination, checkLcrData.destination) != 0) ||
                        (ParanoiaStrcmp(currLcrData.price, checkLcrData.price) != 0) ||
                        (ParanoiaStrcmp(currLcrData.pulse, checkLcrData.pulse) != 0)
                  )
                {
                    currLcrData.destination = checkLcrData.destination;
                    currLcrData.price       = checkLcrData.price;
                    currLcrData.pulse       = checkLcrData.pulse;

                    lcrChanged = true;
                }
            }
        }
    }


    // femon
    // only ask if femon-services are defined in the skin and min. request delay exceeded
    if (femonUse && ((CurrentTime-femonLastChange) >= (uint64_t)femonUpdateDelay)) {
        femonLastChange = CurrentTime;
        p = cPluginManager::CallFirstService("FemonService-v1.0", NULL);
        if (p) {
#ifdef GRAPHLCD_SERVICE_FEMON_VALID
            femonVersionChecked = femonVersionValid = true;
#else
            // nota bene: femon version <= 1.2.x will not make it until here because of a 2nd bug in femon in these versions
            if (!femonVersionChecked) { // only execute this check once
                std::string version = p->Version();
                size_t mstart = 0;
                size_t mend = std::string::npos;
                int numval = 0;

                if ((mend = version.find(".", mstart)) != std::string::npos) {
                    numval = atoi(version.substr(mstart, mend-mstart).c_str());
                    if (numval > 1) {
                        femonVersionValid = true;
                    } else if (numval == 1) { // version 1.x.x
                        mstart = mend + 1;
                        if ((mend = version.find(".", mstart)) != std::string::npos) {
                            numval = atoi(version.substr(mstart, mend-mstart).c_str());
                            if (numval <= 6) { // version <= 1.6.x
                                isyslog("graphlcd plugin: INFO: femon <= 1.7.7 requires a patch prior to be usable with graphlcd\n");
                                isyslog("graphlcd plugin: INFO: see README for further instructions\n");
                            } else if (numval == 7) {
                                mstart = mend + 1;
                                numval = atoi(version.substr(mstart).c_str());  // ignore trailing characters
                                if (numval <= 7) {
                                    isyslog("graphlcd plugin: INFO: femon <= 1.7.7 requires a patch prior to be usable with graphlcd\n");
                                    isyslog("graphlcd plugin: INFO: see README for further instructions\n");
                                } else { // version >= 1.7.8: ok
                                    femonVersionValid = true;
                                }
                            } else { // version >= 1.8.x: ok
                                femonVersionValid = true;
                            }
                        } else {
                            isyslog("graphlcd plugin: INFO: Unable to decode version information of femon\n");
                        }
                    } else {
                        isyslog("graphlcd plugin: INFO: Unable to decode version information of femon\n");
                    }
                }
                femonVersionChecked = true;
            }
#endif  /* GRAPHLCD_SERVICE_FEMON_VALID */

            if (femonVersionValid) {
                if (cPluginManager::CallFirstService("FemonService-v1.0", &checkFemonData)) {
                    femonActive = true;
                    if (
#if 0
                            (strcmp(currFemonData.fe_name, checkFemonData.fe_name) != 0)  ||
                            (strcmp(currFemonData.fe_status, checkFemonData.fe_status) != 0)  ||
#endif
                            (currFemonData.fe_signal     != checkFemonData.fe_signal)     ||
                            (currFemonData.fe_ber        != checkFemonData.fe_ber)        ||
                            (currFemonData.fe_unc        != checkFemonData.fe_unc)        ||
                            (currFemonData.video_bitrate != checkFemonData.video_bitrate) ||
                            (currFemonData.audio_bitrate != checkFemonData.audio_bitrate) ||
                            (currFemonData.dolby_bitrate != checkFemonData.dolby_bitrate)
                      )
                    {
                        currFemonData.fe_name       = checkFemonData.fe_name;
                        currFemonData.fe_status     = checkFemonData.fe_status;
                        currFemonData.fe_snr        = checkFemonData.fe_snr;
                        currFemonData.fe_signal     = checkFemonData.fe_signal;
                        currFemonData.fe_ber        = checkFemonData.fe_ber;
                        currFemonData.fe_unc        = checkFemonData.fe_unc;
                        currFemonData.video_bitrate = checkFemonData.video_bitrate;
                        currFemonData.audio_bitrate = checkFemonData.audio_bitrate;
                        currFemonData.dolby_bitrate = checkFemonData.dolby_bitrate;

                        femonChanged = true;
                    }
                }
            }
        }
    }

    // Mailbox
    // only ask if mailbox-services are defined in the skin and min. request delay exceeded
    if (mailboxUse && ((CurrentTime-mailboxLastChange) >= (uint64_t)mailboxUpdateDelay)) {
        mailboxLastChange = CurrentTime;
        if (cPluginManager::CallFirstService("MailBox-HasNewMail-1.0", NULL) &&
            cPluginManager::CallFirstService("MailBox-GetTotalUnseen-1.0", NULL)) {
            if (cPluginManager::CallFirstService("MailBox-HasNewMail-1.0", &checkMailboxNewData) &&
                cPluginManager::CallFirstService("MailBox-GetTotalUnseen-1.0", &checkMailboxUnseenData)) {
                mailboxActive = true;
                if (
                        (currMailboxNewData != checkMailboxNewData) ||
                        (currMailboxUnseenData != checkMailboxUnseenData)
                  )
                {
                    currMailboxNewData = checkMailboxNewData;
                    currMailboxUnseenData = checkMailboxUnseenData;

                    mailboxChanged = true;
                }
            }
        }
    }

    // span
    // only ask if span-services are defined in the skin and min. request delay exceeded
    if (checkSpanData.bands > 0 && spanUse && ((CurrentTime-spanLastChange) >= (uint64_t)spanUpdateDelay)) {
        spanLastChange = CurrentTime;
        spanActive = false;
        p = cPluginManager::CallFirstService("Span-ClientCheck-v1.0", NULL);
        if (p) {
            cControl* c = cControl::Control(); // workaround a bug somewhere in music or span-plugin
            if (c != NULL && cPluginManager::CallFirstService("Span-GetBarHeights-v1.0", &checkSpanData)) {
                spanActive = true;
                bool chg = false;

                if ( (currSpanData.volumeLeftChannel[0]  != checkSpanData.volumeLeftChannel[0])  ||
                     (currSpanData.volumeRightChannel[0] != checkSpanData.volumeRightChannel[0])  ||
                     (currSpanData.volumeBothChannels[0] != checkSpanData.volumeBothChannels[0])
                   )
                {
                    chg = true;
                    currSpanData.volumeLeftChannel[0]  = checkSpanData.volumeLeftChannel[0];
                    currSpanData.volumeRightChannel[0] = checkSpanData.volumeRightChannel[0];
                    currSpanData.volumeBothChannels[0] = checkSpanData.volumeBothChannels[0];
                }
                for (unsigned int i = 0; i < checkSpanData.bands; i++) {
                    if ( (currSpanData.barHeights[i]             != checkSpanData.barHeights[i]) ||
                         (currSpanData.barHeightsLeftChannel[i]  != checkSpanData.barHeightsLeftChannel[i]) ||
                         (currSpanData.barHeightsRightChannel[i] != checkSpanData.barHeightsRightChannel[i]) ||
                         (currSpanData.barPeaksBothChannels[i]   != checkSpanData.barPeaksBothChannels[i]) ||
                         (currSpanData.barPeaksLeftChannel[i]    != checkSpanData.barPeaksLeftChannel[i]) ||
                         (currSpanData.barPeaksRightChannel[i]   != checkSpanData.barPeaksRightChannel[i])
                       )
                    {
                        chg = true;
                        currSpanData.barHeights[i]             = checkSpanData.barHeights[i];
                        currSpanData.barHeightsLeftChannel[i]  = checkSpanData.barHeightsLeftChannel[i];
                        currSpanData.barHeightsRightChannel[i] = checkSpanData.barHeightsRightChannel[i];
                        currSpanData.barPeaksBothChannels[i]   = checkSpanData.barPeaksBothChannels[i];
                        currSpanData.barPeaksLeftChannel[i]    = checkSpanData.barPeaksLeftChannel[i];
                        currSpanData.barPeaksRightChannel[i]   = checkSpanData.barPeaksRightChannel[i];
                    }
                }
                spanChanged = chg;
            }
        }
    }

    //mutex.Unlock();
    return (radioChanged || lcrChanged || femonChanged || mailboxChanged || spanChanged);
}
