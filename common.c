#include <time.h>

#include <glcdskin/type.h>
#include <glcdskin/string.h>

#include <vdr/plugin.h>
#include <string.h>

#include "strfct.h"

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
            setlocale(LC_TIME, ( Setup.OSDLanguage < (int)(sizeof(locID)/sizeof(char*)) ) ? locID[Setup.OSDLanguage] : "en_US" );
#endif
            strftime(result, sizeof(result), Format.c_str(), tm);

            GLCD::cType r = result;
            return r;
        } else
            return Time;
    }
    return false;
}

GLCD::cType DurationType(int Index, const std::string &Format, double framesPerSecFactor = 1.0)
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

            int f = (Index % (int)framesPerSecFactor) + 1;
            int s = (Index / (int)framesPerSecFactor);

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

int ParanoiaStrcmp(const char *s1, const char *s2)
{
    if (! s1 || ! s2)
        return -1;
    
    // s1 must be under 'our' control (and thus valid), s2 may be a string w/ unexpected content
    size_t n = strlen(s1);
    
    int rv = strncmp(s1, s2, n);
    if (rv == 0)
        return (s2[n] == '\0') ? 0 : -1;
    else
        return rv;
}

const std::string SplitText(const std::string &Text, const std::string &Delim, bool firstPart = true) {
    size_t found = Text.find(Delim);

    if (found != std::string::npos) {
        return (firstPart) ? Text.substr(0, found) : Text.substr(found + Delim.size());
    }
    return (firstPart) ? Text : NULL;
}

const std::string SplitToken(const std::string &Token, const struct GLCD::tSkinAttrib Attrib, bool extSplit) {
    if (Attrib.Type == GLCD::aClean) {
        if (extSplit) {
            std::string Text = SplitText(Token, " - ", true);

            // also cut leading index numbers, eg: "1  Menu" -> "Menu"
            size_t pos = 0;
            bool exitw = false;
            bool valid = true;

            while (!exitw && valid && pos < Text.size()-1) {
                if (Text[pos] == ' ') {
                    pos++;
                } else {
                    valid = (isdigit(Text[pos]) || Text[pos] == ' ' || Text[pos] == '\t') ? true : false;
                    exitw = true;
                }
            }

            exitw = false;
            while (!exitw && valid && pos < Text.size()-1) {
                if (isdigit(Text[pos])) {
                    pos++;
                } else {
                    valid = (Text[pos] == ' ' || Text[pos] == '\t') ? true : false;
                    exitw = true;
                }
            }

            exitw = false;
            while (!exitw && valid && pos < Text.size()-1) {
                if (Text[pos] == ' ') {
                    pos++;
                } else {
                    exitw = true;
                }
            }

            if (valid && Text[pos] == '\t')
                pos++;
            return trim( (valid) ? Text.substr(pos) : Text );
        }
        return SplitText(Token, " - ", true);
    } else if (Attrib.Type == GLCD::aRest) {
        return SplitText(Token, " - ", false);
    }
    return Token;
}
