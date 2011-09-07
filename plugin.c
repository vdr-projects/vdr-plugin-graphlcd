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
 * CONNECT / DISCONNect:
 * (c) 2011      Lutz Neumann <superelchi AT wolke7.net>
 *
 */

#include <getopt.h>

#include <glcddrivers/config.h>
#include <glcddrivers/drivers.h>

#include "display.h"
#include "global.h"
#include "menu.h"

#include <vdr/plugin.h>

#include <ctype.h>

#if APIVERSNUM < 10503
  #include "i18n.h"
  #define trNOOP(_s) (_s)
#endif


static const char * kPluginName = "graphlcd";
static const char *VERSION        = "0.3.0";
static const char *DESCRIPTION    = trNOOP("Output to graphic LCD");
static const char *MAINMENUENTRY  = NULL;

static const char * kDefaultConfigFile = "/etc/graphlcd.conf";


class cPluginGraphLCD : public cPlugin
{
private:
    // Add any member variables or functions you may need here.
    std::string mConfigName;
    std::string mDisplayName;
    std::string mSkinsPath;
    std::string mSkinName;
    GLCD::cDriver * mLcd;
    cGraphLCDDisplay * mDisplay;

    uint64_t to_timestamp;
    bool ConnectDisplay();
    void DisconnectDisplay();

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
    mDisplayName(""),
    mSkinsPath(""),
    mSkinName(""),
    mLcd(NULL),
    mDisplay(NULL)
{
}

cPluginGraphLCD::~cPluginGraphLCD()
{
    DisconnectDisplay();
}

const char * cPluginGraphLCD::CommandLineHelp()
{
    return "  -c CFG,   --config=CFG   use CFG as driver config file (default is \"/etc/graphlcd.conf\")\n"
           "  -d DISP,  --display=DISP use display DISP for output\n"
           "  -s SKIN,  --skin=SKIN    use skin SKIN (default is \"default\")\n"
           "  -p PATH,  --skinspath=PATH  use path PATH for skins (default is \"<plugin_config>/skins/\")\n";
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
                mDisplayName = optarg;
                break;

            case 'p':
                mSkinsPath = optarg;
                break;

            case 's':
                mSkinName = optarg;
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
        mConfigName = kDefaultConfigFile;
        isyslog("graphlcd plugin: No config file specified, using default (%s).\n", mConfigName.c_str());
    }
    if (GLCD::Config.Load(mConfigName) == false)
    {
        esyslog("graphlcd plugin: Error loading config file!\n");
        return false;
    }
    return ConnectDisplay();
}

bool cPluginGraphLCD::Start()
{
    return true;
}

bool cPluginGraphLCD::ConnectDisplay()
{
    unsigned int displayNumber = 0;
    const char* cfgDir;

    if (GLCD::Config.driverConfigs.size() > 0)
    {
        if (mDisplayName.length() > 0)
        {
            for (displayNumber = 0; displayNumber < GLCD::Config.driverConfigs.size(); displayNumber++)
            {
                if (GLCD::Config.driverConfigs[displayNumber].name == mDisplayName)
                    break;
            }
            if (displayNumber == GLCD::Config.driverConfigs.size())
            {
                esyslog("graphlcd plugin: ERROR: Specified display %s not found in config file!\n", mDisplayName.c_str());
                return false;
            }
        }
        else
        {
            isyslog("graphlcd plugin: WARNING: No display specified, using first one (%s).\n", GLCD::Config.driverConfigs[0].name.c_str());
            displayNumber = 0;
            mDisplayName = GLCD::Config.driverConfigs[0].name;
        }
    }
    else
    {
        esyslog("graphlcd plugin: ERROR: No displays specified in config file!\n");
        return false;
    }

    mLcd = GLCD::CreateDriver(GLCD::Config.driverConfigs[displayNumber].id, &GLCD::Config.driverConfigs[displayNumber]);
    if (!mLcd)
    {
        esyslog("graphlcd plugin: ERROR: Failed creating display object %s\n", mDisplayName.c_str());
        return false;
    }

    cfgDir = ConfigDirectory(kPluginName);
    if (!cfgDir)
        return false;

    mDisplay = new cGraphLCDDisplay();
    if (!mDisplay)
        return false;
    if (mSkinName == "")
        mSkinName = "default";
    if (!mDisplay->Initialise(mLcd, cfgDir, mSkinsPath, mSkinName))
        return false;

    dsyslog("graphlcd plugin: init timeout waiting for display thread to get ready");
    to_timestamp = cTimeMs::Now();

    return true;
}

void cPluginGraphLCD::DisconnectDisplay()
{
    delete mDisplay;
    mDisplay = NULL;
    if (mLcd)
        mLcd->DeInit();
    delete mLcd;
    mLcd = NULL;
}

void cPluginGraphLCD::Housekeeping()
{
}

void cPluginGraphLCD::MainThreadHook()
{
    if (mDisplay)
    {
        if (mDisplay->Active())
        {
            if (to_timestamp != (uint64_t)0)
            {
                dsyslog("graphlcd plugin: display thread is ready");
                to_timestamp = (uint64_t)0;
            }
            mDisplay->Tick();
        }
        else 
        {
            if (to_timestamp != (uint64_t)0)
            {
                if ( (cTimeMs::Now() - to_timestamp) > (uint64_t)10000)
                {
                    dsyslog ("graphlcd plugin: timeout while waiting for display thread");
                    /* no activity after 10 secs: display is unusable */
                    GraphLCDSetup.PluginActive = 0;
                    to_timestamp = (uint64_t)0;
                    DisconnectDisplay();
                }
            }
        }
    }
}

const char **cPluginGraphLCD::SVDRPHelpPages(void)
{
    static const char *HelpPages[] = {
        "CLS   Clear Display.",
        "UPD   Update Display.",
        "OFF   Switch Plugin off.",
        "ON   Switch Plugin on.",
        "SET <key> <value>   Set a key=value entry.",
        "SETEXP <exp> <key> <value>   Set a key=value entry which expires after <exp> secs.",
        "UNSET <key>   Unset (clear) entry <key>.",
        "GET <key>   Get value assigned to key.",
        "CONNECT <displayname> [<skin>]   Connect given display.",
        "DISCONN    Disconnect currently connected display.",
        NULL
    };
    return HelpPages;
}

cString cPluginGraphLCD::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    std::string option = Option;
    size_t firstpos = std::string::npos;
    size_t secondpos = std::string::npos;
    firstpos = option.find_first_of(' ');
    if (firstpos != std::string::npos) {
        // remove extra spaces
        while ( ((firstpos+1) < option.length()) && (option[firstpos+1] == ' ') ) {
            option.erase(firstpos+1, 1);
        }
        secondpos = option.find_first_of(' ', firstpos+1);
        if (firstpos != std::string::npos) {
            // remove extra spaces
            while ( ((secondpos+1) < option.length()) && (option[secondpos+1] == ' ') ) {
                option.erase(secondpos+1, 1);
           }
        }
    }
    

    if (strcasecmp(Command, "OFF") == 0) {
        GraphLCDSetup.PluginActive = 0;
        return "GraphLCD Plugin switched off.";
    } else
    if (strcasecmp(Command, "ON") == 0) {
        GraphLCDSetup.PluginActive = 1;
        if (mDisplay != NULL)
            mDisplay->Update();
        return "GraphLCD Plugin switched on.";
    } else
    if (strcasecmp(Command, "CONNECT") == 0) {        
        if (option.size() == 0) {
            return "CONNECT requires at least one parameter: CONNECT <displayname> [<skin>].";
        } else if (firstpos == std::string::npos && option.size() > 0) {
            mDisplayName = option;
        } else {
            mDisplayName = option.substr(0, firstpos);
            mSkinName = option.substr(firstpos+1);
        }
          
        if (mDisplay != NULL)
            DisconnectDisplay();
        std::string retval = "Display "; retval.append(mDisplayName);
        retval.append(ConnectDisplay() ? ": CONNECT ok." : ": CONNECT error");
        return retval.c_str();
    } else
    if (strcasecmp(Command, "DISCONN") == 0 || strcasecmp(Command, "DISCONNECT") == 0 ) {
        if (mDisplay != NULL)
            DisconnectDisplay();
        return "DISCONNect ok";
    } else // following commands are valid only if mDisplay is valid
    if (mDisplay != NULL) {
        if (strcasecmp(Command, "CLS") == 0) {
            if (GraphLCDSetup.PluginActive == 1) {
                return "Error: Plugin is active.";
            } else {
                mDisplay->Clear();
                return "GraphLCD cleared.";
            };
        } else
        if (strcasecmp(Command, "UPD") == 0) {
            if (GraphLCDSetup.PluginActive == 0) {
                return "Error: Plugin is not active.";
            } else {
                mDisplay->Update();
                return "GraphLCD updated.";
            };
        } else
        if (strcasecmp(Command, "SET") == 0) {
            if (firstpos != std::string::npos) {
                std::string key = option.substr(0, firstpos);
                if ( isalpha(key[0]) ) { 
                    mDisplay->GetExtData()->Set(key, option.substr(firstpos+1));
                    return "SET ok";
                }
            }
            return "SET requires two parameters: SET <key> <value>.";
        } else
        if (strcasecmp(Command, "SETEXP") == 0) {
            if (secondpos != std::string::npos) {
                std::string key = option.substr(firstpos+1, secondpos-firstpos-1);
                std::string value = option.substr(secondpos+1);
                if ( isalpha(key[0]) && isdigit(option[0]) ) { 
                    uint32_t expsec = (uint32_t)strtoul( option.substr(0, firstpos).c_str(), NULL, 10);
                    mDisplay->GetExtData()->Set( key, value, expsec );
                    return "SETEXP ok";
                }
            }
            return "SETEXP requires three parameters: SETEXP <exp> <key> <value>.";
        } else
        if (strcasecmp(Command, "UNSET") == 0) {
            if (firstpos == std::string::npos) {
                mDisplay->GetExtData()->Unset( option );
                return "UNSET ok";
            } else {
                return "UNSET requires exactly one parameter: UNSET <key>.";
            }
        } else
        if (strcasecmp(Command, "GET") == 0) {
            if (firstpos == std::string::npos) {
                std::string res = mDisplay->GetExtData()->Get( option );
                std::string retval = "GET "; retval.append(option); retval.append(": "); 
                if (res != "" ) {
                    retval.append(res);
                } else {
                    retval.append("(null)");
                }
                return retval.c_str();
            } else {
                return "GET requires exactly one parameter: GET <key>.";
            }
        } else {  // command not supported
          return NULL;
        }
    } else {
        return "Error: no display connected";
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
