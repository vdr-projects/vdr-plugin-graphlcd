/**
 *  GraphLCD plugin for the Video Disk Recorder
 *
 *  strfct.c  -  various string functions
 *
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online de>
 *  (c) 2004 Andreas Regel <andreas.regel AT powarman de>
 **/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program;                                              *
 *   if not, write to the Free Software Foundation, Inc.,                  *
 *   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA               *
 *                                                                         *
 ***************************************************************************/

#include <string.h>
#include <ctype.h>
#include <algorithm>

#include "strfct.h"


char * strncopy(char * dest , const char * src , size_t n)
{
  strncpy(dest, src, n);
  if (strlen(src) >= n)
  {
    *(dest + n - 1) = 0;
  }
  return dest;
}


std::string trim(const std::string & s)
{
    std::string::size_type left, right;
    left = std::find_if_not(s.begin(), s.end(), isspace) - s.begin();
    if (left == s.length()) // String consists of space characters only
        return "";
    right = std::find_if_not(s.rbegin(), s.rend(), isspace) - s.rbegin();
    return s.substr(left, s.length() - left - right);
}

std::string compactspace(const std::string & s)
{
  std::string str = "";
  std::string tmp;
  unsigned int pos = 0;
  unsigned int cnt = 0;
  tmp = trim(s);
  while (pos < tmp.length())
  {
    if (!isspace(tmp[pos]))
    {
      str += tmp[pos];
      cnt = 0;
    }
    else if (cnt == 0)
    {
      str += tmp[pos];
      cnt++;
    }
    else
      cnt++;
    pos++;
  }
  return str;
}
