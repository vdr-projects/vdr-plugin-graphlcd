/**
 *  GraphLCD plugin for the Video Disk Recorder
 *
 *  service.h  -  class for events from external services
 *
 *  (c) 2010-2011 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 *
 *  mailbox-contribution: user 'Keine_Ahnung'
 **/

#ifndef _GRAPHLCD_SERVICE_H_
#define _GRAPHLCD_SERVICE_H_

#include <glcdskin/type.h>

#include <string>
#include <vdr/plugin.h>

#include <vector> // req. for state.h
#include "state.h"

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

// Femon
struct FemonService_v1_0 {
  cString  fe_name;
  cString  fe_status;
  uint16_t fe_snr;
  uint16_t fe_signal;
  uint32_t fe_ber;
  uint32_t fe_unc;
  double video_bitrate;
  double audio_bitrate;
  double dolby_bitrate;
};



class cGraphLCDService
{
private:
    //cMutex mutex;
    cGraphLCDState * mState;

    RadioTextService_v1_0               checkRTSData,            currRTSData;
    LcrService_v1_0                     checkLcrData,            currLcrData;
    FemonService_v1_0                   checkFemonData,          currFemonData;
    bool                                checkMailboxNewData,     currMailboxNewData;
    unsigned long                       checkMailboxUnseenData,  currMailboxUnseenData;
    /*  __Changed = data has been changed */
    /*  __Active  = plugin/service is available and active */
    /*  __Use     = service is requested in skin (don't call services that wouldn't be used anyway) */
    bool                   radioChanged,   radioActive,    radioUse;
    bool                   lcrChanged,     lcrActive,      lcrUse;
    bool                   femonChanged,   femonActive,    femonUse;
    bool                   mailboxChanged, mailboxActive,  mailboxUse;
    // timestamp of last service update request
    uint64_t               radioLastChange, lcrLastChange, femonLastChange, mailboxLastChange;
    // min. delay between two service update requests
    int                    radioUpdateDelay, lcrUpdateDelay, femonUpdateDelay, mailboxUpdateDelay;

    // check if femon version <= 1.7.7
    bool                   femonVersionChecked, femonVersionValid;

//protected:

public:
    cGraphLCDService(cGraphLCDState * state);
    virtual ~cGraphLCDService();

    bool ServiceIsAvailable(const std::string & Name);
    void SetServiceUpdateDelay(const std::string & Name, int delay);
    bool NeedsUpdate(uint64_t CurrentTime);
    GLCD::cType GetItem(const std::string & ServiceName, const std::string & Item);
};

#endif
