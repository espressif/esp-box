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
 * main.c - command-line test app that uses C interface to MP3 decoder
 **************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mp3dec.h"
#include "debug.h"
#include "timing.h"

#define READBUF_SIZE		(1024*16)	/* feel free to change this, but keep big enough for >= one frame at high bitrates */
#define MAX_ARM_FRAMES		100
#define ARMULATE_MUL_FACT	1

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

int main(int argc, char **argv)
{
	int bytesLeft, nRead, err, offset, outOfData, eofReached;
	unsigned char readBuf[READBUF_SIZE], *readPtr;
	short outBuf[MAX_NCHAN * MAX_NGRAN * MAX_NSAMP];
	FILE *infile, *outfile;
	MP3FrameInfo mp3FrameInfo;
	HMP3Decoder hMP3Decoder;
	int startTime, endTime, diffTime, totalDecTime, nFrames;
#ifdef ARM_ADS	
	float audioSecs;
#endif

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
	InitTimer();
	
	DebugMemCheckStartPoint();

	if ( (hMP3Decoder = MP3InitDecoder()) == 0 )
		return -2;

	DebugMemCheckEndPoint();

	bytesLeft = 0;
	outOfData = 0;
	eofReached = 0;
	readPtr = readBuf;
	nRead = 0;
	totalDecTime = 0;
	nFrames = 0;
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


		/* decode one MP3 frame - if offset < 0 then bytesLeft was less than a full frame */
		startTime = ReadTimer();
 		err = MP3Decode(hMP3Decoder, &readPtr, &bytesLeft, outBuf, 0);
 		nFrames++;
 		
 		endTime = ReadTimer();
 		diffTime = CalcTimeDifference(startTime, endTime);
		totalDecTime += diffTime;

#if defined ARM_ADS && defined MAX_ARM_FRAMES	
		printf("frame %5d  start = %10d, end = %10d elapsed = %10d ticks\r", 
			nFrames, startTime, endTime, diffTime);
		fflush(stdout);
#endif

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
			MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
			if (outfile)
				fwrite(outBuf, mp3FrameInfo.bitsPerSample / 8, mp3FrameInfo.outputSamps, outfile);
		}

#if defined ARM_ADS && defined MAX_ARM_FRAMES
		if (nFrames >= MAX_ARM_FRAMES)
			break;
#endif
	} while (!outOfData);


#ifdef ARM_ADS	
	MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
	audioSecs = ((float)nFrames * mp3FrameInfo.outputSamps) / ( (float)mp3FrameInfo.samprate * mp3FrameInfo.nChans);
	printf("\nTotal clock ticks = %d, MHz usage = %.2f\n", totalDecTime, ARMULATE_MUL_FACT * (1.0f / audioSecs) * totalDecTime * GetClockDivFactor() / 1e6f);
	printf("nFrames = %d, output samps = %d, sampRate = %d, nChans = %d\n", nFrames, mp3FrameInfo.outputSamps, mp3FrameInfo.samprate, mp3FrameInfo.nChans);
#endif

	MP3FreeDecoder(hMP3Decoder);

	fclose(infile);
	if (outfile)
		fclose(outfile);

	FreeTimer();
	DebugMemCheckFree();

	return 0;
}
