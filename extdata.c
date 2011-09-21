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


bool cExtData::Set(const std::string & key, const std::string & value, const std::string & scope, uint32_t expire ) {
    if (mExtDataInstance == NULL) return false; // paranoia check

    std::string compkey = scope; compkey += ":"; compkey += key; // <scope>:<key>,  <scope> can be "" (== global)
    data[compkey] = value;
    
    if (expire > 0) {
        expData[compkey] = cTimeMs::Now() + (expire * 1000);
    } else {
        expData.erase(compkey); // just in case of an old expiration entry for compkey         
    }
    return true;
}


bool cExtData::Unset(const std::string & key, const std::string & scope ) {
    if (mExtDataInstance == NULL) return false; // paranoia check

    std::string compkey = scope; compkey += ":"; compkey += key; // <scope>:<key>,  <scope> can be "" (== global)
    expData.erase(compkey); // ignore result;
    return ( (data.erase(compkey) > 0) ? true : false );
}


bool cExtData::IsSet(const std::string & key, const std::string & scope ) {
    std::string ret = Get(key, scope);
    return ( (ret != "") ? true : false );
}


std::string cExtData::Get(const std::string & key, const std::string & scope ) {
    if (mExtDataInstance == NULL) return ""; // paranoia check

    std::string compkey;
    int l = 0;
    int loops = (scope == "") ? 1 : 2;
    while ( l < loops) {
        compkey = (l == 0) ? scope : ""; compkey += ":"; compkey += key; // 1st loop: display-scope, 2nd loop: global
        it = data.find(compkey);
        if ( it != data.end() ) {
            expDataIt = expData.find(compkey);
            if ( expDataIt != expData.end() ) {
                uint64_t expts = (*expDataIt).second;
                if ( cTimeMs::Now() > expts ) {
                    expData.erase(compkey);
                    data.erase(compkey);
                    // expired, thus no result here, but maybe there'll be a hit in global scope, so don't return here
                } else {
                    return (*it).second;
                }
            } else {
                return (*it).second;
            }
        }
        l++;
    }
    return "";
}
