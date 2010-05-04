#include <time.h>

#include <glcdskin/type.h>

#include <vdr/plugin.h>

GLCD::cType TimeType(time_t Time, const std::string &Format)
{
    static char result[1000];
    struct tm tm_r, *tm;
    tm = localtime_r(&Time, &tm_r);

    if (Time > 0)
    {
        if (Format.length() > 0)
        {
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
            int f = (Index % FRAMESPERSEC) + 1;
            int s = (Index / FRAMESPERSEC);
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
