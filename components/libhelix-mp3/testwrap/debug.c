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
 * debug.c - implementations of memory testing functions
 **************************************************************************************/

#include <stdio.h>
#include "debug.h"

#if !defined (_DEBUG)

void DebugMemCheckInit(void)
{
}

void DebugMemCheckStartPoint(void)
{
}

void DebugMemCheckEndPoint(void)
{
}

void DebugMemCheckFree(void)
{
}

#elif defined (_WIN32) && !defined (_WIN32_WCE)

#include <crtdbg.h>

#ifdef FORTIFY
#include "fortify.h"
#else
static 	_CrtMemState oldState, newState, stateDiff;
#endif

void DebugMemCheckInit(void)
{
	int tmpDbgFlag;

	/* Send all reports to STDOUT */
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );	

	tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
	tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;
	_CrtSetDbgFlag(tmpDbgFlag);
}

void DebugMemCheckStartPoint(void)
{
#ifdef FORTIFY
	Fortify_EnterScope();
#else
	_CrtMemCheckpoint(&oldState);
#endif
}

void DebugMemCheckEndPoint(void)
{
#ifdef FORTIFY
		Fortify_LeaveScope();
#else
		_CrtMemCheckpoint(&newState);
		_CrtMemDifference(&stateDiff, &oldState, &newState);
		_CrtMemDumpStatistics(&stateDiff);
#endif
}

void DebugMemCheckFree(void)
{
	printf("\n");
	if (!_CrtDumpMemoryLeaks())
		printf("Memory leak test:      no leaks\n");

	if (!_CrtCheckMemory())
		printf("Memory integrity test: error!\n");
	else 
		printf("Memory integrity test: okay\n");
}

#elif defined (ARM_ADS)

void DebugMemCheckInit(void)
{
}

void DebugMemCheckStartPoint(void)
{
}

void DebugMemCheckEndPoint(void)
{
}

void DebugMemCheckFree(void)
{
}

#endif
