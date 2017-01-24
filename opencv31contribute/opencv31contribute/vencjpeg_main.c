/*
 *******************************************************************************
 * $Header:  $
 *
 *  Copyright (c) 2000-2009 Vivotek Inc. All rights reserved.
 *
 *  +-----------------------------------------------------------------+
 *  | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *  | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *  | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
 *  | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *  | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *  | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *  |                                                                 |
 *  | THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT   |
 *  | ANY PRIOR NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY |
 *  | VIVOTEK INC.                                                    |
 *  +-----------------------------------------------------------------+
 *
 * $History:  $
 * 
 *
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2000-2009 Vivotek, Inc. All rights reserved.
 *
 * \file
 * vencjpeg_main.c
 *
 * \brief
 * Sample code of vencjpeg library
 *
 * \date
 * 2009/01/22
 *
 * \author
 * David Liu
 *
 *
 *******************************************************************************
 */
#include <stdio.h>
//#include <unistd.h>
//#include <sched.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "vencjpeg.h"

#define ISNUM(a) ((a >= '0' && a <= '9') ? 1 : 0)

unsigned int g_bTerminate = 0;

/* =========================================================================================== */
void SignalHandler(int iSignal)
{
	if ((iSignal == SIGINT) || (iSignal == SIGTERM))
	{
		g_bTerminate = 1;
	}
}

/* =========================================================================================== */
int Init_Signal(void)
{
	if (signal(SIGINT, SignalHandler) == SIG_ERR)
	{
		return -1;
	}

	if (signal(SIGTERM, SignalHandler) == SIG_ERR)
	{
		return -1;
	}

	return 0;
}

/* =========================================================================================== */
int main(int argc, char *argv[])
{
	int					iInputOpt;
	char				*pcStrCross = NULL;
	char				*pszResolution = NULL;
	void*				hVencJpeg;
	int					scResult;
	TVencJpeg_InitOpt	tVideoInitOpt;
	TFrameBuffer		*ptCapFrameBuf = NULL;
	unsigned int		dwFrameRate = 15;
	unsigned int		dwReqCapW = 640;
	unsigned int		dwReqCapH = 480;
	char				acFWString[32];
    unsigned int		dwNumofDevices;
    TDeviceStatus       tDeviceStatus;

#ifdef _DUMP_FRAME
	FILE				*pfFrameOutput = NULL;
	TFrameBuffer		**pptDumpFrameBuf = &ptCapFrameBuf;
	unsigned int		dwDumpYUVCount = 0;
#endif

	// Encode JPEG
	TFrameBuffer		*ptCapFrameBuf4Enc = NULL;
	TJpegEncodeStatus	tJpegEncodeStatus;

	// Dump JPEG
	char				szFileName[128];
	unsigned int		dwDumpJpegCount = 0;

#ifdef _SHOW_FRAME_RATE_INFO
	unsigned int		dwFrameCount = 0;
	unsigned int		dwFrameCountMilliSecond = 0;
	unsigned int		dwSecond = 0, dwLastSecond = 0;
	unsigned int		dwMilliSecond = 0, dwLastMilliSecond = 0;
	unsigned int		dwDiffMilliSecond = 0;
#endif

	Init_Signal();

	memset(&tVideoInitOpt, 0, sizeof(tVideoInitOpt));

	if(argc < 2)
	{
		printf("Usage: %s [OPTION]\n", argv[0]);
		printf("Options:\n");
		printf("\t-c [width]x[height]\tCapture size\n");
		printf("\t-e\t\t\tEnable encode JPEG\n\n");
		printf("Use default setting: Capture size 320x240 & Disable encode JPEG\n");
	}

	while ((iInputOpt = getopt(argc, argv, "c:e")) != -1)
	{
		switch (iInputOpt)
		{
			case 'c':
				//pszResolution = optarg;
				pcStrCross = strchr(pszResolution, 'x');
				if (pcStrCross && ISNUM(pszResolution[0]) == 1) // [width]x[height] Expression
				{
					dwReqCapW = atoi(pszResolution);
					dwReqCapH = atoi(pcStrCross + 1);
				}
				break;
			case 'e':
				tVideoInitOpt.bNeedJpegEncode = 1;
				break;
			default:
				break;
		}
	}

	// and set specific device number that you want to access
	tVideoInitOpt.dwDevNum = 0;

	tVideoInitOpt.ePixelFormat = epfYUV420P;
	tVideoInitOpt.dwFrameRate  = dwFrameRate;
	tVideoInitOpt.dwCapWidth   = dwReqCapW;
	tVideoInitOpt.dwCapHeight  = dwReqCapH;
	tVideoInitOpt.dwJpegEncQ   = 5;
	
	if ((scResult = VencJpeg_Initial(&hVencJpeg, tVideoInitOpt)) != 0)
	{
		fprintf(stderr, "Initialize failed.\n");
		goto END;
	}
	else
	{
		printf("Initialize ok!!\n");
	}

#if 0 // Support set option individually
	TVencJpeg_Option	tSetOpt;

	// Set the capture frame rate
	tSetOpt.eOptionFlags   = VADP_VIDEO_SET_CAPTURE_FRAME_RATE;
	tSetOpt.adwUserData[0] = dwFrameRate;
	if (VencJpeg_SetOptions(hVencJpeg, tSetOpt) != 0)	goto END;

	// Encode JPEG
	// Set the quality level of jpeg encoder
	tSetOpt.eOptionFlags   = VADP_VIDEO_SET_JPEG_ENC_QUALITY;
	tSetOpt.adwUserData[0] = 3;
	if (VencJpeg_SetOptions(hVencJpeg, tSetOpt) != 0)	goto END;
#endif //0

#ifdef _DUMP_FRAME
	pfFrameOutput = fopen("/mnt/ramdisk/vadp_output.yuv", "wb");
	if (pfFrameOutput == NULL)
	{
		fprintf(stderr, "Open dump file failed.\n");
		goto END;
	}
#endif // _DUMP_FRAME

	// Request the capture buffer
	if ((ptCapFrameBuf = VencJpeg_VideoCapGetDepth(hVencJpeg)) != NULL)
	{
		// Process captured frame here
#ifdef _DUMP_FRAME
		if (dwDumpYUVCount < 90)
		{
			unsigned int dwYFrameSize = dwReqCapW*dwReqCapH;//(*pptDumpFrameBuf)->dwStride * (*pptDumpFrameBuf)->dwHeight;
			//printf("Dump YUV resolution: %dx%d, %d bytes\n", (*pptDumpFrameBuf)->dwStride, (*pptDumpFrameBuf)->dwHeight, (dwYFrameSize * 3) >> 1);
			printf("Dump YUV resolution: %dx%d, %d bytes\n", dwReqCapW, dwReqCapH, (dwYFrameSize * 3));
			// YUV420
			fwrite((*pptDumpFrameBuf)->pbyBufY, 1, (dwYFrameSize * 3), pfFrameOutput);
			//fwrite((*pptDumpFrameBuf)->pbyBufY, 1, dwYFrameSize, pfFrameOutput);
			//fwrite((*pptDumpFrameBuf)->pbyBufU, 1, dwYFrameSize >> 2, pfFrameOutput);
			//fwrite((*pptDumpFrameBuf)->pbyBufV, 1, dwYFrameSize >> 2, pfFrameOutput);
			dwDumpYUVCount++;
		}
#endif

#ifdef _SHOW_FRAME_RATE_INFO
		printf("Current frame rate: %d\n", dwCurrFrameRate);
		dwSecond = ptCapFrameBuf->dwSecond;
		dwMilliSecond = ptCapFrameBuf->dwMilliSecond;
		dwDiffMilliSecond = ((dwSecond - dwLastSecond) * 1000) + (dwMilliSecond - dwLastMilliSecond);
		dwLastSecond = dwSecond;
		dwLastMilliSecond = dwMilliSecond;

		dwFrameCount++;
		dwFrameCountMilliSecond += dwDiffMilliSecond;
		if (dwFrameCountMilliSecond > 10000)
		{
			unsigned int dwCurrFrameRate = (((dwFrameCount * 1000) + (dwFrameCountMilliSecond >> 1)) / dwFrameCountMilliSecond);
			printf("Current frame rate: %d\n", dwCurrFrameRate);
			dwFrameCount = dwFrameCountMilliSecond = 0;
		}
#endif // _SHOW_FRAME_RATE_INFO

		// Release the capture buffer
		if (VencJpeg_VideoCapReleaseCapFrame(hVencJpeg) != 0)
		{
			fprintf(stderr, "Release one frame failed!\n");
			//				break;
		}
	}

END:

#ifdef _DUMP_FRAME
	if (pfFrameOutput)
	{
		fclose(pfFrameOutput);
		pfFrameOutput = NULL;
	}
#endif

	if ((scResult = VencJpeg_Release(hVencJpeg)) != 0)
	{
		fprintf(stderr, "Release vencjpeg object failed.\n");
		return 1;
	}
	printf("Release vencjpeg object ok!!\n");

	return 0;
}

/* =========================================================================================== */

