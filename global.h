/*
 * GraphLCD plugin for the Video Disk Recorder
 *
 * global.h - global definitions
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GRAPHLCD_GLOBAL_H_
#define _GRAPHLCD_GLOBAL_H_

#include <stdlib.h>

template<class T> inline void clip(T & value, T min, T max)
{
    if (value < min) value = min;
    if (value > max) value = max;
}

#endif
