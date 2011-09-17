/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * extdata.h - external data sent via SVDRP
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#ifndef _GRAPHLCD_EXTDATA_H_
#define _GRAPHLCD_EXTDATA_H_

#include <stdint.h>

#include <string>
#include <map>


// external data set via SVDRP
class cExtData {
    friend class cGraphLCDSkinConfig;
    friend class cPluginGraphLCD;
private:
    std::map<std::string,std::string>           data;
    std::map<std::string,std::string>::iterator it;
    std::map<std::string,uint64_t>              expData;
    std::map<std::string,uint64_t>::iterator    expDataIt;

    static cExtData * mExtDataInstance;

    cExtData(void) {}
    ~cExtData(void);
protected:
    static cExtData * GetExtData(void);
    static void ReleaseExtData(void);

public:    
    bool Set(std::string key, std::string value, uint32_t expire = 0);
    bool Unset(std::string key);
    bool IsSet(std::string key);
    std::string Get(std::string key);
};

#endif
