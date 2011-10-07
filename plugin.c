/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * plugin.c - main plugin class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 * (c) 2004-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2011 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 * 
 * Contributions:
 * CONNECT / DISCONNect, multi-display support:
 * (c) 2011      Lutz Neumann <superelchi AT wolke7.net>
 *
 */

#include <getopt.h>

#include <glcddrivers/config.h>
#include <glcddrivers/drivers.h>

#include "display.h"
#include "global.h"
#include "menu.h"
#include "extdata.h"
#include "strfct.h"

#include <vdr/plugin.h>

#include <ctype.h>
#include <vector>

#if APIVERSNUM < 10503
  #include "i18n.h"
  #define trNOOP(_s) (_s)
#endif


static const char * kPluginName = "graphlcd";
static const char *VERSION        = "0.3.0";
static const char *DESCRIPTION    = trNOOP("Output to graphic LCD");
static const char *MAINMENUENTRY  = NULL;

#ifndef PLUGIN_GRAPHLCDCONF
  #define PLUGIN_GRAPHLCDCONF  "/etc/graphlcd.conf"
#endif


enum eDisplayStatus {
    EMPTY,
    DEAD,
    CONNECTED,
    DISCONNECTED,
    CONNECT_PENDING,
    CONNECTING
};

struct tDisplayData {
    eDisplayStatus     Status;
    std::string        Name;
    std::string        Skin;
    GLCD::cDriver    * Driver;
    cGraphLCDDisplay * Disp;
    uint64_t           to_timestamp;
};

#define GRAPHLCD_MAX_DISPLAYS 4

class cPluginGraphLCD : public cPlugin
{
private:
    // Add any member variables or functions you may need here.
    std::string mDisplayNames;
    std::string mSkinNames;
    std::string mConfigName;
    std::string mSkinsPath;
    std::string mConfigDir;
    tDisplayData mDisplay[GRAPHLCD_MAX_DISPLAYS];
    cExtData * mExtData;

    int DisplayIndex(std::string);
    bool ConnectDisplay(unsigned int, unsigned int);
    void DisconnectDisplay(unsigned int);

public:
    cPluginGraphLCD();
    virtual ~cPluginGraphLCD();
    virtual const char * Version() { return VERSION; }
    virtual const char * Description() { return tr(DESCRIPTION); }
    virtual const char * CommandLineHelp();
    virtual bool ProcessArgs(int argc, char * argv[]);
    virtual bool Initialize();
    virtual bool Start();
    virtual void Housekeeping();
    virtual const char **SVDRPHelpPages(void);
    virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
    virtual void MainThreadHook(void);
    virtual const char * MainMenuEntry() { return MAINMENUENTRY; }
    virtual cOsdObject * MainMenuAction();
    virtual cMenuSetupPage * SetupMenu();
    virtual bool SetupParse(const char * Name, const char * Value);
};

cPluginGraphLCD::cPluginGraphLCD()
:   mConfigName(""),
    mSkinsPath("")
{
    for (unsigned int i = 0; i < GRAPHLCD_MAX_DISPLAYS; i++)
    {
        mDisplay[i].Status = EMPTY;
        mDisplay[i].Name = "";
        mDisplay[i].Skin = "";
        mDisplay[i].Driver = NULL;
        mDisplay[i].Disp = NULL;
    }
    mExtData = cExtData::GetExtData();
}

cPluginGraphLCD::~cPluginGraphLCD()
{
    for (unsigned int index = 0; index < GRAPHLCD_MAX_DISPLAYS; index++)
        DisconnectDisplay(index);
    mExtData->ReleaseExtData();
    mExtData = NULL;
}

const char * cPluginGraphLCD::CommandLineHelp()
{
    return "  -c, --config=CFG             use CFG as driver config file (default is \""PLUGIN_GRAPHLCDCONF"\")\n"
           "  -d, --display=DISP[,DISP]... use display DISP for output or if DISP=none: start w/o any display\n"
           "  -s, --skin=SKIN[,SKIN]...    use skin SKIN (default is \"default\")\n"
           "  -p, --skinspath=PATH         use path PATH for skins (default is \"<plugin_config>/skins/\")\n";
}

bool cPluginGraphLCD::ProcessArgs(int argc, char * argv[])
{
    static struct option long_options[] =
    {
        {"config",    required_argument, NULL, 'c'},
        {"display",   required_argument, NULL, 'd'},
        {"skinspath", required_argument, NULL, 'p'},
        {"skin",      required_argument, NULL, 's'},
        {NULL}
    };

    int c;
    int option_index = 0;
    while ((c = getopt_long(argc, argv, "c:d:p:s:", long_options, &option_index)) != -1)
    {
        switch (c)
        {
            case 'c':
                mConfigName = optarg;
                break;

            case 'd':
                mDisplayNames = optarg;
                break;

            case 'p':
                mSkinsPath = optarg;
                break;

            case 's':
                mSkinNames = optarg;
                break;

            default:
                return false;
        }
    }

    return true;
}

bool cPluginGraphLCD::Initialize()
{

#if APIVERSNUM < 10503
    RegisterI18n(Phrases);
#endif

    if (mConfigName.length() == 0)
    {
        mConfigName = PLUGIN_GRAPHLCDCONF;
        isyslog("graphlcd plugin: No config file specified, using default (%s).\n", mConfigName.c_str());
    }
    if (GLCD::Config.Load(mConfigName) == false)
    {
        esyslog("graphlcd plugin: Error loading config file!\n");
        return false;
    }
    if (GLCD::Config.driverConfigs.size() == 0)
    {
        esyslog("graphlcd plugin: ERROR: No displays specified in config file!\n");
        return false;
    }

    if (mDisplayNames == "none")
    {
        isyslog("graphlcd plugin: WARNING: displayname = none, starting with no display connected.\n");
        return true;
    }

    if (mDisplayNames.length() == 0)
    {
        isyslog("graphlcd plugin: WARNING: No display specified, using first one (%s).\n", GLCD::Config.driverConfigs[0].name.c_str());
        mDisplayNames = GLCD::Config.driverConfigs[0].name;
    }
    
    size_t pos1, pos2 = -1;
    unsigned int index = 0;
    do
    {
        pos1 = pos2 + 1;
        pos2 = mDisplayNames.find(',', pos1);
        mDisplay[index].Name = mDisplayNames.substr(pos1, (pos2 == std::string::npos) ? pos2 : pos2 - pos1); 
        mDisplay[index].Status = CONNECT_PENDING;
        index++;
    }
    while (pos2 != std::string::npos && index < GRAPHLCD_MAX_DISPLAYS);
    
    pos2 = -1;
    index = 0;
    do
    {
        pos1 = pos2 + 1;
        pos2 = mSkinNames.find(',', pos1);
        mDisplay[index].Skin = mSkinNames.substr(pos1, (pos2 == std::string::npos) ? pos2 : pos2 - pos1); 
        index++;
    }
    while (pos2 != std::string::npos && index < GRAPHLCD_MAX_DISPLAYS);    

    const char* tempConfigDir = ConfigDirectory(kPluginName);
    if (!tempConfigDir) {
        return false;
    } else {
        mConfigDir = tempConfigDir;
    }

    bool connecting = false;
    for (unsigned int i = 0; i < GRAPHLCD_MAX_DISPLAYS; i++)
    {
        if (mDisplay[i].Name.size() > 0)
        {
            int displayNumber = DisplayIndex(mDisplay[i].Name);
            if (displayNumber == -1)
            {
                esyslog("graphlcd plugin: ERROR: Specified display %s not found in config file!\n", mDisplay[i].Name.c_str());
                return false;
            }
            
            if (!connecting)
            {
                if (ConnectDisplay(i, (unsigned int) displayNumber) == false)
                    return false;
                connecting = true;
            }
        }
    }
    return true;
}   

bool cPluginGraphLCD::Start()
{
    return true;
}

int cPluginGraphLCD::DisplayIndex(std::string displayName)
{
    unsigned int displayNumber;
    for (displayNumber = 0; displayNumber < GLCD::Config.driverConfigs.size(); displayNumber++)
    {
        if (GLCD::Config.driverConfigs[displayNumber].name == displayName)
            break;
    }
    return (displayNumber == GLCD::Config.driverConfigs.size()) ? -1 : (int) displayNumber;
}

bool cPluginGraphLCD::ConnectDisplay(unsigned int index, unsigned int displayNumber)
{
    mDisplay[index].Driver = GLCD::CreateDriver(GLCD::Config.driverConfigs[displayNumber].id, &GLCD::Config.driverConfigs[displayNumber]);
    if (!mDisplay[index].Driver)
    {
        esyslog("graphlcd plugin: ERROR: Failed creating display object %s\n", mDisplay[index].Name.c_str());
        mDisplay[index].Status = DEAD;
        return false;
    }

    mDisplay[index].Disp = new cGraphLCDDisplay();
    if (!mDisplay[index].Disp) {
        mDisplay[index].Status = DEAD;
        return false;
    }
    if (mDisplay[index].Skin == "")
        mDisplay[index].Skin = "default";
    if (!mDisplay[index].Disp->Initialise(mDisplay[index].Driver, mConfigDir.c_str(), mSkinsPath, mDisplay[index].Skin)) {
        mDisplay[index].Status = DEAD;
        return false;
    }

    /* if plugin was deactivated -> reactivate */
    GraphLCDSetup.PluginActive = 1;
    dsyslog("graphlcd plugin: init timeout waiting for display %s thread to get ready", mDisplay[index].Name.c_str());
    mDisplay[index].to_timestamp = cTimeMs::Now();
    mDisplay[index].Status = CONNECTING;
    
    return true;    
}

void cPluginGraphLCD::DisconnectDisplay(unsigned int index)
{
    if (mDisplay[index].Disp != NULL)
        delete mDisplay[index].Disp;
    mDisplay[index].Disp = NULL;
    if (mDisplay[index].Driver != NULL) {
        mDisplay[index].Driver->DeInit();
        delete mDisplay[index].Driver;
    }
    mDisplay[index].Driver = NULL;
    mDisplay[index].Status = DISCONNECTED;
}

void cPluginGraphLCD::Housekeeping()
{
}

void cPluginGraphLCD::MainThreadHook()
{
    bool wait = false;
    int nextconnect = -1;
    for (unsigned int index = 0; index < GRAPHLCD_MAX_DISPLAYS; index++)
    {
        if (mDisplay[index].Status == CONNECTING)
        {
            if (mDisplay[index].Disp->Active())
            {
                dsyslog("graphlcd plugin: display thread for %s is ready", mDisplay[index].Name.c_str());
                mDisplay[index].Status = CONNECTED;
            }
            else
            {
                if ( (cTimeMs::Now() - mDisplay[index].to_timestamp) > (uint64_t) 10000)
                {
                    dsyslog ("graphlcd plugin: timeout while waiting for display thread %s", mDisplay[index].Name.c_str());
                    /* no activity after 10 secs: display is unusable */
                    //GraphLCDSetup.PluginActive = 0;
                    DisconnectDisplay(index);
                }
                else
                    wait = true;
            }
        }
        if (nextconnect == -1 && mDisplay[index].Status == CONNECT_PENDING)
            nextconnect = index;
            
        if (mDisplay[index].Status == CONNECTED)
            mDisplay[index].Disp->Tick();
    }
    
    if (nextconnect != -1 && !wait)
        if (ConnectDisplay(nextconnect, (unsigned int) DisplayIndex(mDisplay[nextconnect].Name)) == false)
            esyslog("graphlcd plugin: ERROR: failed connecting display %s\n", mDisplay[nextconnect].Name.c_str());
}

const char **cPluginGraphLCD::SVDRPHelpPages(void)
{
    static const char *HelpPages[] = {
        "CLS     Clear display.",
        "UPD     Update display.",
        "OFF     Switch plugin off.",
        "ON      Switch plugin on.",
        "SET     <key>[,expire=<expire>][,display=<display>] <value>  Set entry <key> to value <value>. optionally expire after <expire> seconds.",
        "UNSET   <key>[,display=<display>]  Unset (clear) entry <key>.",
        "GET     <key>[,display=<display>]  Get value assigned to entry <key>.",
        "CONNECT [<display> [<skin>]]   Connect given display or reconnect all displays if called w/o parameter.",
        "DISCONN [<display>]   Disconnect given display or all displays if called w/o parameter.",
        NULL
    };
    return HelpPages;
}

cString cPluginGraphLCD::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    std::vector <std::string> options;

    // split option-string
    if (trim(Option).size() > 0) {
        std::string options_raw = trim(Option);
        size_t firstpos = std::string::npos;
        while ( (firstpos = options_raw.find_first_of(' ')) != std::string::npos  ) {
            // special treatment for SET <key> <value>:  option <value> is the rest of the cmd line, including spaces
            if ( (strcasecmp(Command, "SET") == 0) && (options.size() == 1) ) {
                options.push_back( trim(options_raw) );
                options_raw = "";
            } else {
                options.push_back( trim(options_raw.substr(0, firstpos)) );
                options_raw = trim(options_raw.substr(firstpos+1));
            }
        }
        if (trim(options_raw).size() > 0)
            options.push_back( trim(options_raw) );
    }

    if (strcasecmp(Command, "OFF") == 0) {
        GraphLCDSetup.PluginActive = 0;
        return "GraphLCD Plugin switched off.";
    } else
    if (strcasecmp(Command, "ON") == 0) {
        GraphLCDSetup.PluginActive = 1;
        for (unsigned int index = 0; index < GRAPHLCD_MAX_DISPLAYS; index++)
        {
            if (mDisplay[index].Status == CONNECTED)
                mDisplay[index].Disp->Update();
        }
        return "GraphLCD Plugin switched on.";
    } else
    if (strcasecmp(Command, "CONNECT") == 0) {
        if (options.size() >= 1 && options.size() <= 2) {
            std::string name;
            
            name = options[0];
            
            int index = -1;
            for (unsigned int i = 0; i < GRAPHLCD_MAX_DISPLAYS; i++) {
                if (mDisplay[i].Name == name) {
                    index = i;
                    break;
                }
                if (index == -1 && mDisplay[i].Disp == NULL)
                    index = i;
            }

            if (index == -1)
                return "CONNECT error: max number of displays already connected.";

            int displayNumber = DisplayIndex(name);
            if (displayNumber < 0)
                return "CONNECT errror: display not in config file.";

            mDisplay[index].Name = name;
            mDisplay[index].Skin = (options.size() == 2)
                                   ? options[1]
                                   : ( 
                                       (mDisplay[index].Skin != "")
                                       ? mDisplay[index].Skin
                                       : "default"
                                     );
            
            std::string retval = "Display "; retval.append(name); retval.append(": ");
            if (mDisplay[index].Status == CONNECTED) {
                DisconnectDisplay(index);
                retval.append("RE");
            }
            
            if (ConnectDisplay(index, (unsigned int) displayNumber) == true) {
                for (unsigned int count = 0; count < 100; count++) {
                    cCondWait::SleepMs(100);
                    if (mDisplay[index].Disp->Active()) {
                        dsyslog ("graphlcd plugin: display thread ready");
                        mDisplay[index].Status = CONNECTED;
                        break;
                    }
                }
                if (mDisplay[index].Status != CONNECTED)
                    dsyslog ("graphlcd plugin: timeout while waiting for display thread");
            }

            retval.append((mDisplay[index].Status == CONNECTED) ? "CONNECT ok." : "CONNECT error.");
            return retval.c_str();
        } else if (options.size() == 0) {
            int count_ok = 0;
            int count_fail = 0;

            for (unsigned int i = 0; i < GRAPHLCD_MAX_DISPLAYS; i++) {
                if (mDisplay[i].Name != "") {

                    if (mDisplay[i].Skin == "") // should never occur, only for paranoia ...
                        mDisplay[i].Skin = "default";

                    int displayNumber = DisplayIndex(mDisplay[i].Name);
                    if (mDisplay[i].Status == CONNECTED) {
                        DisconnectDisplay(i);
                    }
                    
                    if (ConnectDisplay(i, (unsigned int) displayNumber) == true) {
                        for (unsigned int count = 0; count < 100; count++) {
                            cCondWait::SleepMs(100);
                            if (mDisplay[i].Disp->Active()) {
                                dsyslog ("graphlcd plugin: display thread ready for '%s'", mDisplay[i].Name.c_str());
                                mDisplay[i].Status = CONNECTED;
                                break;
                            }
                        }
                        if (mDisplay[i].Status != CONNECTED)
                            dsyslog ("graphlcd plugin: timeout while waiting for display thread '%s'", mDisplay[i].Name.c_str());
                    }

                    if (mDisplay[i].Status != CONNECTED) {
                        dsyslog ("graphlcd plugin: timeout while waiting for display thread");
                        count_fail ++;
                    } else {
                        count_ok ++;
                    }     
                }
            }
            char buf[25];
            std::string retval = "RECONNECT status: ";
            snprintf(buf, 24, "OK = %1d, FAILED = %1d", count_ok, count_fail);
            retval += buf;
            return retval.c_str();
        }
        return "CONNECT requires up to two parameters: CONNECT [<display> [<skin>]].";
    } else
    if (strcasecmp(Command, "DISCONN") == 0 || strcasecmp(Command, "DISCONNECT") == 0 ) {
        if ( options.size() >= 0 && options.size() < 2 ) {
            std::string name = "";
            bool disconn_all = true;
            if (options.size() != 0) {
                disconn_all = false;
                name = options[0];
            }

            for (unsigned int index = 0; index < GRAPHLCD_MAX_DISPLAYS; index++) {
                if (mDisplay[index].Status == CONNECTED && (disconn_all || name == mDisplay[index].Name))
                    DisconnectDisplay(index);
            }
            return "DISCONNect ok.";
        }
        return "DISCONNect requires zero or one parameters: DISCONNect [<display>].";
    } else { // following commands are valid only if at least one display is connected
        unsigned int index;
        for (index = 0; index < GRAPHLCD_MAX_DISPLAYS; index++)
            if (mDisplay[index].Status == CONNECTED)
                break;
        if (index == GRAPHLCD_MAX_DISPLAYS) {
            return "Error: no display connected";
        } else {
            if (strcasecmp(Command, "CLS") == 0) {
                if (GraphLCDSetup.PluginActive == 1) {
                    return "Error: Plugin is active.";
                } else {
                    for (unsigned int index = 0; index < GRAPHLCD_MAX_DISPLAYS; index++)
                        if (mDisplay[index].Status == CONNECTED)
                            mDisplay[index].Disp->Clear();
                    return "GraphLCD cleared.";
                };
            } else
            if (strcasecmp(Command, "UPD") == 0) {
                if (GraphLCDSetup.PluginActive == 0) {
                    return "Error: Plugin is not active.";
                } else {
                    for (unsigned int index = 0; index < GRAPHLCD_MAX_DISPLAYS; index++)
                        if (mDisplay[index].Status == CONNECTED)
                            mDisplay[index].Disp->Update();
                    return "GraphLCD updated.";
                };
            } else
            if ( (strcasecmp(Command, "SET") == 0) || (strcasecmp(Command, "UNSET") == 0) || (strcasecmp(Command, "GET") == 0) ) {
                std::string key = "";
                std::string display = "";
                std::string expire = "";

                if (options.size() > 0) {
                    // split key parameter
                    std::string key_raw = options[0];
                    size_t commapos = key_raw.find_first_of(',');
                    if (commapos == std::string::npos) {
                        key = key_raw;
                    } else {
                        key = key_raw.substr(0,commapos);
                        do {
                            key_raw = key_raw.substr(commapos+1);
                            size_t delimpos = key_raw.find_first_of('=');
                            if (delimpos != std::string::npos) {
                                std::string paramkey = key_raw.substr(0, delimpos);
                                size_t nextpos = key_raw.find_first_of(',');
                                std::string paramval = (nextpos == std::string::npos)
                                                       ? key_raw.substr(delimpos+1)
                                                       : key_raw.substr(delimpos+1, nextpos-delimpos-1);

                                if ( paramkey == "d" || paramkey == "display") {
                                    display = paramval;
                                } else if ( (strcasecmp(Command, "SET") == 0) && 
                                            ( paramkey == "e" || paramkey == "expire" || paramkey == "expires" )
                                          )
                                {
                                    expire = paramval;
                                } else {
                                    return "SET: invalid refining parameter(s) for option '<key>'.";
                                }
                            } else {
                                return "SET: invalid refining parameter(s) for option '<key>'.";
                            }
                        } while ( (commapos = key_raw.find_first_of(',')) != std::string::npos );
                    }
                }

                if (strcasecmp(Command, "SET") == 0) {
                    if (options.size() == 2) {
                        uint32_t expsecs = 0;
                        if (expire != "") {
                            expsecs = (uint32_t)strtoul(expire.c_str(), NULL, 10);
                        }

                        if ( isalpha(key[0]) ) {
                            for (unsigned int index = 0; index < GRAPHLCD_MAX_DISPLAYS; index++)
                                if (mDisplay[index].Status == CONNECTED)
                                    mExtData->Set(key, options[1], display, expsecs );
                            return "SET ok";
                        }
                        return "SET not ok";
                    }
                    return "SET requires exactly two parameters: SET <key>[,expire=<expire>][,display=<display>] <value>.";
                } else
                if (strcasecmp(Command, "UNSET") == 0) {
                    if (options.size() == 1) {
                        for (unsigned int index = 0; index < GRAPHLCD_MAX_DISPLAYS; index++)
                            if (mDisplay[index].Status == CONNECTED)
                                mExtData->Unset( key, display );
                        return "UNSET ok";
                    }
                    return "UNSET requires exactly one parameter: UNSET <key>[,display=<display>].";
                } else
                if (strcasecmp(Command, "GET") == 0) {
                    if (options.size() == 1) {
                        std::string res = "";
                        for (unsigned int index = 0; res == "" && index < GRAPHLCD_MAX_DISPLAYS; index++)
                            if (mDisplay[index].Status == CONNECTED)
                                res = mExtData->Get( key, display );
                        std::string retval = "GET "; retval.append(key); retval.append(": "); 
                        if (res != "" ) {
                            retval.append(res);
                        } else {
                            retval.append("(null)");
                        }
                        return retval.c_str();
                    }
                    return "GET requires exactly one parameter: GET <key>[,display=<display>].";
                }
            } else {  // command not supported
                return NULL;
            }
        }
    }
}

cOsdObject * cPluginGraphLCD::MainMenuAction()
{
    return NULL;
}

cMenuSetupPage * cPluginGraphLCD::SetupMenu()
{
    return new cGraphLCDMenuSetup();
}

bool cPluginGraphLCD::SetupParse(const char * Name, const char * Value)
{
    if      (!strcasecmp(Name, "PluginActive")) GraphLCDSetup.PluginActive = atoi(Value);
    else if (!strcasecmp(Name, "ShowDateTime")) GraphLCDSetup.ShowDateTime = atoi(Value);
    else if (!strcasecmp(Name, "ShowChannel")) GraphLCDSetup.ShowChannel = atoi(Value);
    else if (!strcasecmp(Name, "ShowChannelLogo")) GraphLCDSetup.ShowChannelLogo = atoi(Value);
    else if (!strcasecmp(Name, "ShowSymbols")) GraphLCDSetup.ShowSymbols = atoi(Value);
    else if (!strcasecmp(Name, "ShowProgram")) GraphLCDSetup.ShowProgram = atoi(Value);
    else if (!strcasecmp(Name, "ShowTimebar")) GraphLCDSetup.ShowTimebar = atoi(Value);
    else if (!strcasecmp(Name, "ShowMenu")) GraphLCDSetup.ShowMenu = atoi(Value);
    else if (!strcasecmp(Name, "ShowMessages")) GraphLCDSetup.ShowMessages = atoi(Value);
    else if (!strcasecmp(Name, "ShowColorButtons")) GraphLCDSetup.ShowColorButtons = atoi(Value);
    else if (!strcasecmp(Name, "ShowVolume")) GraphLCDSetup.ShowVolume = atoi(Value);
    else if (!strcasecmp(Name, "ShowNotRecording")) GraphLCDSetup.ShowNotRecording = atoi(Value);
    else if (!strcasecmp(Name, "IdentifyReplayType")) GraphLCDSetup.IdentifyReplayType = atoi(Value);
    else if (!strcasecmp(Name, "ModifyReplayString")) GraphLCDSetup.ModifyReplayString = atoi(Value);
    else if (!strcasecmp(Name, "ShowReplayLogo")) GraphLCDSetup.ShowReplayLogo = atoi(Value);
    else if (!strcasecmp(Name, "ScrollMode")) GraphLCDSetup.ScrollMode = atoi(Value);
    else if (!strcasecmp(Name, "ScrollSpeed")) GraphLCDSetup.ScrollSpeed = atoi(Value);
    else if (!strcasecmp(Name, "ScrollTime")) GraphLCDSetup.ScrollTime = atoi(Value);
    else if (!strcasecmp(Name, "BrightnessActive")) GraphLCDSetup.BrightnessActive = atoi(Value);
    else if (!strcasecmp(Name, "BrightnessIdle")) GraphLCDSetup.BrightnessIdle = atoi(Value);
    else if (!strcasecmp(Name, "BrightnessDelay")) GraphLCDSetup.BrightnessDelay = atoi(Value);
    else return false;
    return true;
}

VDRPLUGINCREATOR(cPluginGraphLCD); // Don't touch this!
