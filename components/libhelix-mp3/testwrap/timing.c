/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/**************************************************************************************
 * Fixed-point MP3 decoder
 * Jon Recker (jrecker@real.com), Ken Cooke (kenc@real.com)
 * June 2003
 *
 * timing.c - implementations of CPU timing functions
 **************************************************************************************/

#include "timing.h"

/* NOTES: 
 * - for armulator (ARM_ADS) use -clock 100 (100 Hz system clock) for accurate
 *     timing since CLOCKS_PER_SEC = 100 (this only works for zero wait-state
 *     memory unless you adjust memory timings accordingly)
 * - other option for armulator is to simulate accurate hardware timers (see below)
 */
#if (defined (_WIN32) && !defined (_WIN32_WCE)) || defined (ARM_ADS) || (defined (__GNUC__) && defined (ARM))

#include <time.h>

int InitTimer(void)
{
    return 0;
}

UINT ReadTimer(void)
{
    return clock();
}

int FreeTimer(void)
{
    return 0;
}

UINT GetClockFrequency(void)
{
    return CLOCKS_PER_SEC;
}

UINT GetClockDivFactor(void)
{
    return 1;
}

UINT CalcTimeDifference(UINT startTime, UINT endTime)
{
    /* timer counts up on x86, 32-bit counter */
    if (endTime < startTime)
		return (0x7fffffff - (startTime - endTime) );
    else
		return (endTime - startTime);
}
#elif defined (_WIN32) && defined (_WIN32_WCE)

#include <windows.h>

int InitTimer(void)
{
    return 0;
}

UINT ReadTimer(void)
{
    return GetTickCount();
}

int FreeTimer(void)
{
    return 0;
}

UINT GetClockFrequency(void)
{
    return 1000;
}

UINT GetClockDivFactor(void)
{
    return 1;
}

UINT CalcTimeDifference(UINT startTime, UINT endTime)
{
    /* timer counts up on x86, 32-bit counter */
    if (endTime < startTime)
		return (0x7fffffff - (startTime - endTime) );
    else
		return (endTime - startTime);
}

#elif 0	/* if defined ARM_ADS - this uses simulated high-res hardware timers */

/* see definitions in ADSv1_2/bin/peripherals.ami */
#define TIMER_BASE		0x0a800000
#define TIMER_VALUE_1	(TIMER_BASE + 0x04)
#define TIMER_CONTROL_1	(TIMER_BASE + 0x08)

int InitTimer(void)
{
	volatile unsigned int *timerControl1 = (volatile unsigned int *)TIMER_CONTROL_1;
	unsigned int control1;
	
	/* see ARMulator Reference, pg 4-78 
	 * bits [3:2] = clock divisor factor (00 = 1, 01 = 16, 10 = 256, 11 = undef)
	 * bit  [6]   = free-running mode (0) or periodic mode (1)
	 * bit  [7]   = timer disabled (0) or enabled (1)
	 */
	control1 = 0x00000088;
	*timerControl1 = control1;
	
	return 0;
}

UINT ReadTimer(void)
{
	volatile unsigned int *timerValue1 = (volatile unsigned int *)TIMER_VALUE_1;
	unsigned int value;
	
	value = *timerValue1 & 0x0000ffff;
	
	return (UINT)value;
}

int FreeTimer(void)
{
    return 0;
}

UINT GetClockFrequency(void)
{
    return 0;
}

UINT GetClockDivFactor(void)
{
    return 256;
}

UINT CalcTimeDifference(UINT startTime, UINT endTime)
{
	
    /* timer counts down int ARMulator, 16-bit counter */
    if (endTime > startTime)
		return (startTime + 0x00010000 - endTime);
    else
		return (startTime - endTime);
}

#endif
