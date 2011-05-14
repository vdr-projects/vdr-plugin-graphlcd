/**
 *  GraphLCD plugin for the Video Disk Recorder
 *
 *  service.h  -  class for events from external services
 *
 *  (c)      2010 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 **/

#include "strfct.h"

#include "service.h"

#include <vdr/plugin.h>


cGraphLCDService::cGraphLCDService(cGraphLCDState * state)
{
    mState = state;

    /* initialise flags for services */
    radioActive  = false;    radioChanged = false;    radioUse     = false;
    lcrActive    = false;    lcrChanged   = false;    lcrUse       = false;
    femonActive  = false;    femonChanged = false;    femonUse     = false;

    radioLastChange = lcrLastChange = femonLastChange = 0;

    femonVersionChecked = femonVersionValid = false;

    // default min. change delays
    radioUpdateDelay = 100;   // 100 ms
    lcrUpdateDelay   = 60000; // 60 sec
    femonUpdateDelay = 2000;  // 2 sec
}

cGraphLCDService::~cGraphLCDService()
{
}


bool cGraphLCDService::ServiceIsAvailable(const std::string & Name) {
    if (Name == "RadioTextService-v1.0" || Name == "radio") {
        radioUse = true;
        return (mState->GetChannelInfo().isRadio) ? radioActive : false;
    } else if (Name == "LcrService-v1.0" || Name == "lcr") {
        lcrUse = true;
        return lcrActive;
    } else if (Name == "FemonService-v1.0" || Name == "femon") {
        femonUse = true;
        return femonActive;
    }
    return false;
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
    }

    return "";
}



/* async. check event updates for services from other plugins */
/* only sets flags but does NOT update display output */
bool cGraphLCDService::NeedsUpdate(uint64_t CurrentTime)
{
    //mutex.Lock();

    /*radioActive  = false;*/    radioChanged = false;
    /*lcrActive    = false;*/    lcrChanged   = false;
    /*femonActive  = false;*/    femonChanged = false;

    cPlugin *p = NULL;

    // Radiotext
    // only ask if radio-services are defined in the skin and min. request delay exceeded
    if (radioUse && ((CurrentTime-radioLastChange) >= (uint64_t)radioUpdateDelay)) {
        radioLastChange = CurrentTime;
        p = cPluginManager::CallFirstService("RadioTextService-v1.0", NULL);
        if (p) {
            if (cPluginManager::CallFirstService("RadioTextService-v1.0", &checkRTSData)) {
                radioActive = true;
                if (
                        (currRTSData.rds_info != checkRTSData.rds_info) ||
                        (currRTSData.rds_pty != checkRTSData.rds_pty) ||
                        (strcmp(currRTSData.rds_text, checkRTSData.rds_text) != 0) ||
                        (strcmp(currRTSData.rds_title, checkRTSData.rds_title) != 0) ||
                        (strcmp(currRTSData.rds_artist, checkRTSData.rds_artist) != 0)
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
            if (cPluginManager::CallFirstService("LcrService-v1.0", &checkLcrData)) {
                lcrActive = true;
                if (
                        (strcmp(currLcrData.destination, checkLcrData.destination) != 0) ||
                        (strcmp(currLcrData.price, checkLcrData.price) != 0) ||
                        (strcmp(currLcrData.pulse, checkLcrData.pulse) != 0)
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
                                isyslog("graphlcd plugin: femon <= 1.7.7 requires a patch prior to be usable with graphlcd.\n");
                                isyslog("graphlcd plugin: see README for further instructions.\n");
                            } else if (numval == 7) {
                                mstart = mend + 1;
                                numval = atoi(version.substr(mstart).c_str());  // ignore trailing characters
                                if (numval <= 7) {
                                    isyslog("graphlcd plugin: femon <= 1.7.7 requires a patch prior to be usable with graphlcd.\n");
                                    isyslog("graphlcd plugin: see README for further instructions.\n");
                                } else { // version >= 1.7.8: ok
                                    femonVersionValid = true;
                                }
                            } else { // version >= 1.8.x: ok
                                femonVersionValid = true;
                            }
                        } else {
                            isyslog("graphlcd plugin: unable to decode version information of femon.\n");
                        }
                    } else {
                        isyslog("graphlcd plugin: unable to decode version information of femon.\n");
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

    //mutex.Unlock();
    return (radioChanged || lcrChanged || femonChanged);
}
