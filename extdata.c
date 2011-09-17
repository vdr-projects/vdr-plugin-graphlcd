/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * extdata.c - external data sent via SVDRP
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#include "extdata.h"

#include <vdr/tools.h>

#include <stdio.h>
cExtData * cExtData::mExtDataInstance = NULL;

cExtData * cExtData::GetExtData(void) {
    if (mExtDataInstance == NULL) {
        mExtDataInstance = new cExtData();
    }
    return mExtDataInstance;
}


void cExtData::ReleaseExtData(void) {
    delete mExtDataInstance;
    mExtDataInstance = NULL;
}


cExtData::~cExtData(void) {
    data.clear();
    expData.clear();
}


bool cExtData::Set(std::string key, std::string value, uint32_t expire) {
    if (mExtDataInstance == NULL) return false; // paranoia check

    data[key] = value;
    
    if (expire > 0) {
        expData[key] = cTimeMs::Now() + (expire * 1000);
    } else {
        expData.erase(key); // just in case of an old expiration entry for key         
    }
    return true;
}


bool cExtData::Unset(std::string key) {
    if (mExtDataInstance == NULL) return false; // paranoia check

    expData.erase(key); // ignore result;
    return ( (data.erase(key) > 0) ? true : false );
}


bool cExtData::IsSet(std::string key) {
    std::string ret = Get(key);
    return ( (ret != "") ? true : false );
}


std::string cExtData::Get(std::string key) {
    if (mExtDataInstance == NULL) return ""; // paranoia check

    it = data.find(key);
    if ( it != data.end() ) {
        expDataIt = expData.find(key);
        if ( expDataIt != expData.end() ) {
            uint64_t expts = (*expDataIt).second;
            if ( cTimeMs::Now() > expts ) {
                expData.erase(key);
                data.erase(key);
                return "";
            } else {
                return (*it).second;
            }
        } else {
            return (*it).second;
        }
    } else {
        return "";
    }
}
