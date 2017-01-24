/*
 *******************************************************************************
 * $Header:  $
 *
 *  Copyright (c) 2000-2012 Vivotek Inc. All rights reserved.
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
 * vencjpeg.h
 *
 * \brief
 * Header file of vencjpeg library
 *
 * \date
 * 2012/09/10
 *
 * \author
 * David Liu
 *
 *
 *******************************************************************************
 */

#ifndef __VENCJPEG_H__
#define __VENCJPEG_H__

#define VENCJPEG_VERSION MAKEFOURCC(0,0,0,1)
#define VENCJPEG_MODIFY_DATETIME "Oct 14 2014 20:08"

/*!
 ******************************************************************************
 * Available option:
 * 
 * VADP_VIDEO_SET_CAPTURE_FRAME_RATE:
 * Specify the capture frame rate. 0 means the maximum available frame rate.
 * Please also note that the maximum frame of NTSC system is 30 and PAL is 25.
 * 
 * VADP_VIDEO_SET_JPEG_ENC_QUALITY:
 * Specify the quality level of JPEG encode. 0 means the default quality level 
 * and levels 1 to 5 are supported.
 *
 ******************************************************************************
 */

typedef enum
{
	VADP_VIDEO_INVALID_OPTION             = 0x00000000,
	VADP_VIDEO_SET_CAPTURE_FRAME_RATE     = 0x00000001,
	VADP_VIDEO_SET_JPEG_ENC_QUALITY       = 0x00000002
} EOptionFlags;

typedef enum
{
	// YUV420IUV(YYYY....UVUV....) which is the native format of TI DM36X scaler output
	epfYUV420IUV,
	// YUV420Planar (YYYY...UU...VV...): Mozart platform only supports YUV420Planar
	epfYUV420P
} EPixelFormat;

typedef enum
{
	era0Degree = 0,
	era90Degree = 90,
	era270Degree = 270
} ERotationAngle;

/*!
* This structure is for specify the initial information of vencjpeg library
*/
typedef struct 
{
	// Need frame for jpeg encode
	unsigned int		bNeedJpegEncode;
	// Video input device number, value is started from 0
	unsigned int		dwDevNum;
	// Pixel format of captured frame
	EPixelFormat		ePixelFormat;
	// Video capture frame width, must be less than or equal to Max captured Width
	unsigned int		dwCapWidth;
	// Video capture frame height, must be less than or equal to Max captured Height
	unsigned int		dwCapHeight;
	// Video capture frame rate
	unsigned int		dwFrameRate;
	// Video jpeg encode quality level 0 ~ 5
	// 0: default quality
	// 1~5: if the number is bigger, the quality is better 
	unsigned int		dwJpegEncQ;
} TVencJpeg_InitOpt;

/*!
* This structure is for video frame buffer
*/
typedef struct
{
	//! point to start of Y frame buffer
	unsigned char		*pbyBufY;	
	//! point to start of U frame buffer
	unsigned char		*pbyBufU;
	//! point to start of V frame buffer
	unsigned char		*pbyBufV;
	//! video frame stride
	unsigned int		dwStride;   
	//! captured frame width
	unsigned int		dwWidth;
	//! captured frame height
	unsigned int		dwHeight;
	//! sec of current frame
	unsigned int		dwSecond;
	//! millisecond of current frame
	unsigned int		dwMilliSecond;
	//! frame count from start of current frame
	unsigned int		dwFrameCount;
} TFrameBuffer;

/*!
* This structure is for indicating the encode status
*/
typedef struct
{
	//! The address of encode output buffer
	unsigned char		*pbyEncOutputBuf;
	//! The data length of output bitstream
	unsigned int		dwEncSize;
} TJpegEncodeStatus;

/*!
* This structure is for video option setting
*/
typedef struct 
{
	//! VADP setting options (EOptionFlags)
	EOptionFlags	eOptionFlags;
	//! Video capture frame width
	unsigned int	adwUserData[4];
} TVencJpeg_Option;

/*!
 * This structure is for querying device status
 */
typedef struct
{
	//! The data of Rotation Angle of Captured image
	ERotationAngle      eRotationAngle;
	//! The data of Max captured frame Width the device can provide
	unsigned int		dwMaxCapW;
	//! The data of Max captured frame Height the device can provide
	unsigned int		dwMaxCapH;
} TDeviceStatus;

#if 0
// --------------------- function brief ----------------------------------------
unsigned int VencJpeg_Initial(void* *phObject, TVencJpeg_InitOpt tVideoInitOpt);
unsigned int VencJpeg_Release(void* hObject);
TFrameBuffer* VencJpeg_VideoCapGetOneFrame(void* hObject);
unsigned int VencJpeg_JpegEncOneFrame(void* hObject, TFrameBuffer *ptFrameBuf, TJpegEncodeStatus *ptEncStatus);
unsigned int VencJpeg_VideoCapReleaseCapFrame(void* hObject);
unsigned int VencJpeg_QueryLuminance(void* hObject, unsigned int *pdwQueryResult);
unsigned int VencJpeg_QueryFirmwareVersion(char* pcVersionStringBuf, unsigned int dwBufSize);
unsigned int VencJpeg_QueryNumofDevices(unsigned int *pdwNumofDevices);
unsigned int VencJpeg_QueryDeviceStatus(unsigned int dwDevNum, TDeviceStatus *ptDeviceStatus);
unsigned int VencJpeg_SetOptions(void* hObject, TVencJpeg_Option tVadpSetOpt);
#endif

/*!
 ******************************************************************************
 * \brief
 * Create an instance to handle vencjpeg.
 *
 * \param *phObject
 * \a (o) a pointer to receive the handle of vencjpeg object.
 *
 * \param tVadpInitOpt
 * \a (i) a structure to set options of vencjpeg.
 *
 * \retval 0
 * Create the instance successfully.
 *
 * \retval non-zero
 * Failed to create the instance.
 *
 * \remark
 * NONE
 *
 * \see also VencJpeg_Release
 *
 ******************************************************************************
 */
unsigned int VencJpeg_Initial(void* *phObject, TVencJpeg_InitOpt tVideoInitOpt);

/*!
 ******************************************************************************
 * \brief
 * Delete the instance of vencjpeg.
 *
 * \param hObject
 * \a (i) the handle of vencjpeg.
 *
 * \retval 0
 * Create the instance successfully.
 *
 * \retval non-zero
 * Failed to delete the instance.
 *
 * \remark
 * NONE
 *
 * \see also VencJpeg_Initial
 *
 ******************************************************************************
 */
unsigned int VencJpeg_Release(void* hObject);

/*!
 ******************************************************************************
 * \brief
 * Get one frame from capture driver.
 *
 * \param hObject
 * \a (i) the handle of vencjpeg.
 *
 * \retval (TFrameBuffer*) ptFrameBuf
 * The address of one frame buffer.
 * NULL pointer means the capture device get the same frame, the application needs to 
 * perform sched_yield().
 *
 * \remark
 * Each frame buffer needs to be released, please call VencJpeg_VideoCapReleaseCapFrame
 * to release the buffer.
 *
 * \see also VencJpeg_VideoCapReleaseCapFrame.
 *
 ******************************************************************************
 */
TFrameBuffer* VencJpeg_VideoCapGetOneFrame(void* hObject);

/*!
 ******************************************************************************
 * \brief
 * Release the frame to capture driver.
 *
 * \param hObject
 * \a (i) the handle of vencjpeg.
 *
 * \retval 0
 * Release the frame buffer successfully.
 *
 * \remark
 * NONE
 *
 * \see also VencJpeg_VideoCapGetOneFrame.
 *
 ******************************************************************************
 */
unsigned int VencJpeg_VideoCapReleaseCapFrame(void* hObject);

/*!
 ******************************************************************************
 * \brief
 * Perform JPEG encoding.
 *
 * \param hObject
 * \a (i) the handle of vencjpeg.
 *
 * \param ptInVFB
 * \a (i) The address of input frame buffer.
 * The frame format must be 16x16 macro block mode for some platform.
 *
 * \param hObject
 * \a (o) the encode status.
 *
 * \retval 0
 * Encode one frame successfully.
 *
 * \retval non-zero
 * Fail to encode this frame.
 *
 * \remark
 * If you call VencJpeg_VideoCapGetOneFrame and VencJpeg_F2BHandler and want to 
 * encode JPEG, you can call VencJpeg_VideoCapReleaseCapFrame first to release
 * capture buffer to avoid that the buffer is occupied for a long time.
 *
 * \see also VencJpeg_VideoCapGetOneFrame, VencJpeg_VideoCapReleaseCapFrame.
 *
 ******************************************************************************
 */
unsigned int VencJpeg_JpegEncOneFrame(void* hObject, TFrameBuffer *ptInVFB, TJpegEncodeStatus *ptEncStatus);

/*!
 ******************************************************************************
 * \brief
 * Query current luminance value from sensor.
 *
 * \param hObject
 * \a (i) the handle of vencjpeg.
 *
 * \param pdwQueryResult
 * \dw (o) the pointer of unsigned int type variable for query result
 *
 * \retval 0
 * Query successfully.
 *
 * \retval non-zero.
 * Query failed.
 *
 * \remark
 * Please note that for the time being, FD7130 can support this feature.
 *
 ******************************************************************************
 */
unsigned int VencJpeg_QueryLuminance(void* hObject, unsigned int *pdwQueryResult);

/*!
 ******************************************************************************
 * \brief
 * Query firmware version of this device.
 *
 * \param pcVersionStringBuf
 * \a (o) The f/w version string buffer address which should be specified by caller.
 *
 * \param dwBufSize
 * \dw (i) The input size of string buffer. 32 bytes should be enough.
 *
 * \retval 0
 * Query successfully.
 *
 * \retval non-zero.
 * Query failed.
 *
 * \remark
 * None.
 *
 ******************************************************************************
 */
unsigned int VencJpeg_QueryFirmwareVersion(char* pcVersionStringBuf, unsigned int dwBufSize);

/*!
 ******************************************************************************
 * \brief
 * Query total number of devices.
 *
 * \param pdwNumofDevices
 * \a (o) the pointer of unsigned int type variable for total number of devices
 *
 * \retval 0
 * Query successfully.
 *
 * \retval non-zero.
 * Query failed.
 *
 * \remark
 * None.
 *
 ******************************************************************************
 */
unsigned int VencJpeg_QueryNumofDevices(unsigned int *pdwNumofDevices);

/*!
 ******************************************************************************
 * \brief
 * Query device status
 *
 * \param dwDevNum
 * \dw (i) Video input device number
 *
 * \param ptDeviceStatus
 * \dw (o) the pointer of TDeviceStatus type variable for device status
 *
 * \retval 0
 * Get options successfully.
 *
 * \retval non-zero
 * Failed to get options.
 *
 * \remark
 * You can query device status to get necessary information before initialize VencJpeg Handler.
 *
 ******************************************************************************
 */
unsigned int VencJpeg_QueryDeviceStatus(unsigned int dwDevNum, TDeviceStatus *ptDeviceStatus);

/*!
 ******************************************************************************
 * \brief
 * Set VADP options.
 *
 * \param hObject
 * \a (i) the handle of vencjpeg.
 *
 * \param TVencJpeg_Option
 * \a (i) a structure to set options of vencjpeg.
 *
 * \retval 0
 * Set options successfully.
 *
 * \retval non-zero
 * Failed to set options.
 *
 * \remark
 * NONE.
 *
 ******************************************************************************
 */
unsigned int VencJpeg_SetOptions(void* hObject, TVencJpeg_Option tVadpSetOpt);

#endif // __VENCJPEG_H__

