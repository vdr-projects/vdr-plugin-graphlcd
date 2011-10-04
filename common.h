
#include <glcdskin/string.h>

GLCD::cType TimeType(time_t Time, const std::string &Format);
GLCD::cType DurationType(int Index, const std::string &Format, double framesPerSecFactor = 1.0);
int ParanoiaStrcmp(const char *s1, const char *s2);
const std::string SplitText(const std::string &Text, const std::string &Delim, bool firstPart = true);
const std::string SplitToken(const std::string &Token, const struct GLCD::tSkinAttrib Attrib, bool extSplit = false);
