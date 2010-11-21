/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * plugin.c - main plugin class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <getopt.h>

#include <glcddrivers/config.h>
#include <glcddrivers/drivers.h>

#include "display.h"
#include "global.h"
#include "menu.h"

#include <vdr/plugin.h>


static const char *VERSION        = "0.1.9-pre (git 2010/11)";
static const char *DESCRIPTION    = "Output to graphic LCD";
static const char *MAINMENUENTRY  = NULL;

static const char * kDefaultConfigFile = "/etc/graphlcd.conf";


class cPluginGraphLCD : public cPlugin
{
private:
    // Add any member variables or functions you may need here.
    std::string mConfigName;
    std::string mDisplayName;
    GLCD::cDriver * mLcd;
    cGraphLCDDisplay * mDisplay;
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
    virtual void MainThreadHook(void);
    virtual const char * MainMenuEntry() { return MAINMENUENTRY; }
    virtual cOsdObject * MainMenuAction();
    virtual cMenuSetupPage * SetupMenu();
    virtual bool SetupParse(const char * Name, const char * Value);
};

cPluginGraphLCD::cPluginGraphLCD()
:   mConfigName(""),
    mDisplayName("")
{
    mLcd = NULL;
    mDisplay = NULL;
}

cPluginGraphLCD::~cPluginGraphLCD()
{
    delete mDisplay;
    if (mLcd)
        mLcd->DeInit();
    delete mLcd;
}

const char * cPluginGraphLCD::CommandLineHelp()
{
    return "  -c CFG,   --config=CFG   use CFG as driver config file\n"
           "  -d DISP,  --display=DISP use display DISP for output\n";
}

bool cPluginGraphLCD::ProcessArgs(int argc, char * argv[])
{
    static struct option long_options[] =
    {
        {"config",  required_argument, NULL, 'c'},
        {"display", required_argument, NULL, 'd'},
        {NULL}
    };

    int c;
    int option_index = 0;
    while ((c = getopt_long(argc, argv, "c:d:", long_options, &option_index)) != -1)
    {
        switch (c)
        {
            case 'c':
                mConfigName = optarg;
                break;

            case 'd':
                mDisplayName = optarg;
                break;

            default:
                return false;
        }
    }

    return true;
}

bool cPluginGraphLCD::Initialize()
{
    unsigned int displayNumber = 0;
    const char * cfgDir;

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

    cfgDir = ConfigDirectory(PLUGIN_NAME);
    if (!cfgDir)
        return false;

    mDisplay = new cGraphLCDDisplay();
    if (!mDisplay)
        return false;
    if (mDisplay->Init(mLcd, cfgDir) != 0)
        return false;

    return true;
}

bool cPluginGraphLCD::Start()
{
    int count;

    dsyslog("graphlcd plugin: waiting for display thread to get ready");
    for (count = 0; count < 1200; count++)
    {
        if (mDisplay->Active())
        {
            dsyslog ("graphlcd plugin: display thread ready");
            return true;
        }
#if VDRVERSNUM < 10314
        usleep(100000);
#else
        cCondWait::SleepMs(100);
#endif
    }
    dsyslog ("graphlcd plugin: timeout while waiting for display thread");
    return false;
}

void cPluginGraphLCD::Housekeeping()
{
}

void cPluginGraphLCD::MainThreadHook()
{
    if (mDisplay)
        mDisplay->Tick();
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
    else if (!strcasecmp(Name, "ShowLogo")) GraphLCDSetup.ShowLogo = atoi(Value);
    else if (!strcasecmp(Name, "ShowSymbols")) GraphLCDSetup.ShowSymbols = atoi(Value);
    else if (!strcasecmp(Name, "ShowETSymbols")) GraphLCDSetup.ShowETSymbols = atoi(Value);
    else if (!strcasecmp(Name, "ShowProgram")) GraphLCDSetup.ShowProgram = atoi(Value);
    else if (!strcasecmp(Name, "ShowTimebar")) GraphLCDSetup.ShowTimebar = atoi(Value);
    else if (!strcasecmp(Name, "ShowMenu")) GraphLCDSetup.ShowMenu = atoi(Value);
    else if (!strcasecmp(Name, "ShowMessages")) GraphLCDSetup.ShowMessages = atoi(Value);
    else if (!strcasecmp(Name, "ShowColorButtons")) GraphLCDSetup.ShowColorButtons = atoi(Value);
    else if (!strcasecmp(Name, "ShowVolume")) GraphLCDSetup.ShowVolume = atoi(Value);
    else if (!strcasecmp(Name, "ShowNotRecording")) GraphLCDSetup.ShowNotRecording = atoi(Value);
    else if (!strcasecmp(Name, "IdentifyReplayType")) GraphLCDSetup.IdentifyReplayType = atoi(Value);
    else if (!strcasecmp(Name, "ModifyReplayString")) GraphLCDSetup.ModifyReplayString = atoi(Value);
    else if (!strcasecmp(Name, "ReplayLogo")) GraphLCDSetup.ReplayLogo = atoi(Value);
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
