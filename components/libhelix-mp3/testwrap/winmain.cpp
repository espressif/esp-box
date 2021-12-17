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
 * winmain.cpp - command-line test app that uses C++ interface to MP3 decoder
 **************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mpadecobjfixpt.h"
#include "debug.h"
#include "timing.h"

#define READBUF_SIZE	(1024*16)	/* feel free to change this, but keep big enough for >= one frame at high bitrates */
#define MAX_ARM_FRAMES	25

static int FillReadBuffer(unsigned char *readBuf, unsigned char *readPtr, int bufSize, int bytesLeft, FILE *infile)
{
	int nRead;

	/* move last, small chunk from end of buffer to start, then fill with new data */
	memmove(readBuf, readPtr, bytesLeft);				
	nRead = fread(readBuf + bytesLeft, 1, bufSize - bytesLeft, infile);
	/* zero-pad to avoid finding false sync word after last frame (from old data in readBuf) */
	if (nRead < bufSize - bytesLeft)
		memset(readBuf + bytesLeft + nRead, 0, bufSize - bytesLeft - nRead);	

	return nRead;
}

#if defined (_WIN32) && defined (_WIN32_WCE)
#include <windows.h>
#define main	TestMain
static int main(int argc, char **argv);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd)
{
	/* not designed to do anything right now - just lets us build application to force linker
	 *   to run and check for link errors 
	 */
	char *testArgs[3] = {
		"testwrap.exe", 
		"\\My Documents\\test128.mp3",
		"nul"
	};

	TestMain(3, testArgs);

	return 0;
}

#endif

int main(int argc, char **argv)
{
	int bytesLeft, bytesIn, err, nRead, offset, outOfData, eofReached;
	unsigned char readBuf[READBUF_SIZE], *readPtr;
	short outBuf[MAX_NCHAN * MAX_NGRAN * MAX_NSAMP];
	FILE *infile, *outfile;
	int initFlag, chans, bits;
	unsigned long sampRate, outBytes;
	CMpaDecObj *decobj;

	if (argc != 3) {
		printf("usage: mp3dec infile.mp3 outfile.pcm\n");
		return -1;
	}
	infile = fopen(argv[1], "rb");
	if (!infile) {
		printf("file open error\n");
		return -1;
	}
		
	if (strcmp(argv[2], "nul")) {
		outfile = fopen(argv[2], "wb");
		if (!outfile) {
			printf("file open error\n");
			return -1;
		}
	} else {
		outfile = 0;	/* nul output */
	}

	DebugMemCheckInit();
	decobj = new CMpaDecObj;
	if (!decobj)
		return -1;

	bytesLeft = 0;
	outOfData = 0;
	eofReached = 0;
	readPtr = readBuf;
	nRead = 0;
	err = 0;
	initFlag = 0;
	do {
		/* somewhat arbitrary trigger to refill buffer - should always be enough for a full frame */
		if (bytesLeft < 2*MAINBUF_SIZE && !eofReached) {
			nRead = FillReadBuffer(readBuf, readPtr, READBUF_SIZE, bytesLeft, infile);
			bytesLeft += nRead;
			readPtr = readBuf;
			if (nRead == 0)
				eofReached = 1;
		}

		/* find start of next MP3 frame - assume EOF if no sync found */
		offset = MP3FindSyncWord(readPtr, bytesLeft);
		if (offset < 0) {
			outOfData = 1;
			break;
		}
		readPtr += offset;
		bytesLeft -= offset;

		/* lazy initialization for backwards compatibility with mpadecobj API */
		if (!initFlag) {
			DebugMemCheckStartPoint();
			if (!decobj->Init_n(readPtr, bytesLeft))
				return -1;	/* init error */
			DebugMemCheckEndPoint();
	
			decobj->GetPCMInfo_v(sampRate, chans, bits);
			decobj->GetSamplesPerFrame_n();
			initFlag = 1;
		}

		/* decode one MP3 frame - if offset < 0 then bytesLeft was less than a full frame */
		outBytes = sizeof(outBuf);
		bytesIn = bytesLeft;
		decobj->DecodeFrame_v(readPtr, (unsigned long *)(&bytesIn), (unsigned char *)outBuf, &outBytes, &err);
		readPtr += bytesIn;
		bytesLeft -= bytesIn;

		if (err) {
			/* error occurred */
			switch (err) {
			case ERR_MP3_INDATA_UNDERFLOW:
				outOfData = 1;
				break;
			case ERR_MP3_MAINDATA_UNDERFLOW:
				/* do nothing - next call to decode will provide more mainData */
				break;
			case ERR_MP3_FREE_BITRATE_SYNC:
			default:
				outOfData = 1;
				break;
			}
		} else {
			/* no error */
			if (outfile)
				fwrite(outBuf, 1, (unsigned int)outBytes, outfile);
		}
	} while (!outOfData);

	fclose(infile);
	if (outfile)
		fclose(outfile);

	delete decobj;

	DebugMemCheckFree();

	return 0;
}

