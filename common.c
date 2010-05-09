#include <time.h>

#include <glcdskin/type.h>

#include <vdr/plugin.h>

#if APIVERSNUM < 10503
#include <locale.h>

static char* locID[] = {
 "en_US", //" English",
 "de_DE", //   "Deutsch",
 "sl_SI", //   "Slovenski",
 "it_IT", //   "Italiano",
 "nl_NL", //   "Nederlands",
 "pt_PT", //   "Português",
 "fr_FR", //   "Français",
 "no_NO", //   "Norsk",
 "fi_FI", //   "suomi",  Finnish (this is not a typo - it's really lowercase!)
 "pl_PL", //   "Polski",
 "es_ES", //   "Español",
 "el_GR", //   "ÅëëçíéêÜ",  Greek
 "sv_SE", //   "Svenska",
 "ro_RO", //   "Românã",
 "hu_HU", //   "Magyar",
 "ca_ES", //   "Català",
 "ru_RU", //   "ÀãááÚØÙ",  Russian
 "hr_HR", //   "Hrvatski",
 "et_EE", //   "Eesti",
 "da_DK", //   "Dansk",
 "cs_CZ" //   "Èesky", Czech
};
#endif


GLCD::cType TimeType(time_t Time, const std::string &Format)
{
    static char result[1000];
    struct tm tm_r, *tm;
    tm = localtime_r(&Time, &tm_r);

    if (Time > 0)
    {
        if (Format.length() > 0)
        {
// for vdr < 1.5.x: force locale for correct language output. for >= 1.5.x: system locale should be fine
#if APIVERSNUM < 10503
            setlocale(LC_TIME, locID[Setup.OSDLanguage]);
#endif
            strftime(result, sizeof(result), Format.c_str(), tm);

            GLCD::cType r = result;
            return r;
        } else
            return Time;
    }
    return false;
}

GLCD::cType DurationType(int Index, const std::string &Format)
{
    static char result[1000];
    if (Index > 0)
    {
        if (Format.length() > 0)
        {
            uint update = 0;
            const char *ptr = Format.c_str();
            char *res = result;
            enum { normal, format } state = normal;
            int n = 0;
#if VDRVERSNUM >= 10701
            int f = (Index % DEFAULTFRAMESPERSECOND) + 1;
            int s = (Index / DEFAULTFRAMESPERSECOND);
#else
            int f = (Index % FRAMESPERSEC) + 1;
            int s = (Index / FRAMESPERSEC);
#endif
            int m = s / 60 % 60;
            int h = s / 3600;
            s %= 60;
            while (*ptr && res < result + sizeof(result))
            {
                switch (state)
                {
                    case normal:
                        if (*ptr == '%')
                            state = format;
                        else
                            *(res++) = *ptr;
                        break;

                    case format:
                        switch (*ptr)
                        {
                            case 'H':
                                n = snprintf(res, sizeof(result) - (res - result), "%02d", h);
                                break;

                            case 'k':
                                n = snprintf(res, sizeof(result) - (res - result), "% 2d", h);
                                break;

                            case 'M':
                                n = snprintf(res, sizeof(result) - (res - result), "%02d", m);
                                update = 1000*60;
                                break;

                            case 'm':
                                n = snprintf(res, sizeof(result) - (res - result), "%d", m + (h * 60));
                                update = 1000*60;
                                break;

                            case 'S':
                                n = snprintf(res, sizeof(result) - (res - result), "%02d", s);
                                update = 1000;
                                break;

                            case 'f':
                                n = snprintf(res, sizeof(result) - (res - result), "%d", f);
                                update = 1000;
                                break;

                            case '%':
                                n = 1;
                                *res = '%';
                                break;
                        }
                        res += n;
                        state = normal;
                        break;
                }
                ++ptr;
            }

            GLCD::cType r = result;
            r.SetUpdate(update);
            return r;
        } else
            return (int)Index;
    }
    return false;
}
