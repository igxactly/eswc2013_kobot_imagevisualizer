/*
 *(C) Copyright 2006 Marvell International Ltd.  
 * All Rights Reserved 
 *
 * Author:     Neo Li
 * Created:    March 07, 2007
 * Copyright:  Marvell Semiconductor Inc. 
 *
 * Sample code interface header for PXA Linux release. All interfaces
 * introduced in this file is not multiple thread safe unless it's 
 * specified explicitly. 
 *
 */

#ifndef	_PXA3XX_MEDIA_LIB_HEADER_
#define	_PXA3XX_MEDIA_LIB_HEADER_

#include <stdio.h>
#include <pxa_dbg.h>
#include <alsa/asoundlib.h>

////////////////////////////////////////////////////////////////////////
// Common Data Definition
////////////////////////////////////////////////////////////////////////
enum pxavid_format{
	pxavid_invalid = 0,
	pxavid_rggb8,
	pxavid_rggb10,
	pxavid_ycbcr420,
	pxavid_ycbcr422,
};

// Common video frame buffer structure
struct pxa_video_buf{
	int width,height;
	enum pxavid_format format;
	union{
		struct{
			unsigned short *buf;
			unsigned int len;
		}rggb10;
		struct{
			unsigned char *y,*cb,*cr;
			unsigned int ystep,cbstep,crstep;
		}ycbcr;
	};
};

// Window rect definition
struct pxa_rect{
	int x,y;
	int width,height;
};

////////////////////////////////////////////////////////////////////////
// Overlay2 Sample Code Interface
// 1. overlay2_open
// 2. overlay2_close
// 3. overlay2_getbuf
// 4. overlay2_write
//
// Application shall open an overlay2 handle and configure it with
// proper settings before write video buffer to the device. If the 
// source buffer is larger than overlay2 window, the pixels those are
// out of border will be cut. 
//
// Overlay2 window rect is pixel coordinate based on base pane. The
// window could not be larger than base pane size. The overlay2 sample
// code is not multi-thread safe.
// 
////////////////////////////////////////////////////////////////////////
/* Open the overlay2 device and return a handle. 
 * Arguments:
 *	dev	overlay2 device file name; e.g. "/dev/fb2"
 *	format	desired frame format; must be ycbcr422 or ycbcr420
 *	window	desired overlay2 window rect in pixel based on base pane
 *		coordinate; if this parameter is NULL, the overlay2 window
 *		will be full screen
 * Return:
 *	NULL	fail
 *	!=NULL	handle of overlay2
 */
//int overlay2_open(char* dev, enum pxavid_format format,
 //               struct pxa_rect* window);

int overlay2_open(char* dev, enum pxavid_format format,struct pxa_rect* rect, int w, int h, int w_off, int h_off);

/* Close the open overlay2 handle.
 * Arguments:
 *	handle	open overlay2 device handle
 */
void overlay2_close(int handle);

/* Write a frame to overlay2 and the frame will be displayed immediately.
 * The frame must have identical format with overlay2 setting. If the
 * frame size is larger than overlay2 window, the pixels those are out of
 * boundary will be cut.
 * Arguments:
 *	handle	overlay2 handle
 *	srcbuf	video frame that will be displayed
 * Return:
 *	0	succeed
 *	<0	fail
 */
int overlay2_write(int handle, struct pxa_video_buf* srcbuf);

/* Get the video buffer so application could write to the buffer directly. 
 * If application maps and writes to the buffer, it must handle the buffer
 * boundary by itself. This function is optional. 
 *
 * If overlay2 device has more than one buffers, application can repeatedly
 * call this function to get all display buffers until the function returns
 * fail. If overlay2 device has more than one frame buffer, the first returned
 * buffer is the default display buffer. Application can use overlay2_setbuf
 * to specify which buffer will be displayed. 
 * Arguments:
 *	handle	overlay2 handle
 *	vidbuf	optional; if it's not NULL, this function will return the
 *		video buffer mapped for overlay2; write any content to
 *		the buffer will be displayed immediately. If application
 *		reconfigure overlay2 settings, it must retrieve the buffer
 *		again and shall never access previous one.
 * Return:
 *	0	succeed
 *	<0	fail
 */
int overlay2_getbuf(int handle, struct pxa_video_buf* vidbuf);

/* Specify frame buffer that will be displayed on screen. This function is
 * only valid when application maps more than one frame buffer from overlay2
 * device. In this case, only one frame buffer will be the foreground buffer
 * that's displayed on LCD. Application can use this function to specify 
 * foreground frame buffer. 
 * Arguments:
 *	handle	Overlay2 handle
 *	vidbuf	Video frame that will be displayed; this buffer must be got
 *		by overlay2_getbuf.
 * Return:
 *	0	succeed
 *	<0	fail
 */
int overlay2_setbuf(int handle, struct pxa_video_buf* vidbuf);

////////////////////////////////////////////////////////////////////////
// Camera Sample Code Interface
//
// Application shall follow this process in using camera,
//	open -> config -> start -> get frame
//	-> release frame -> stop -> close
// Application can only config camera mode when the
// capture process is stopped. But it's not necessary to close camera
// before application reconfigs camera.
//
// Optimization Guide
// 1. Optimize switch latency between video capture and still image capture
// 2. Application could get new buffer before release old one;
////////////////////////////////////////////////////////////////////////
/* Open camera device with specified sensor. This function returns a 
 * handle which shall be used in other camera functions.
 * Arguments:
 *	camera	Device file name of camera; if this parameter is NULL, the 
 *		function will use default device "/dev/video0"
 *	sensor	Index of desired sensor. If there're two sensors
 *		on the platform, application shall specify the 
 *		index of desired sensor. If there's
 *		only one sensor on the platform, the sensor shall be 0.
 * Return:
 *	NULL	fail
 *	!=NULL	handle of camera device
 */
int camera_open(char* camera,int sensor);
/* Close camera device. Camera must be stopped before it's close. 
 * Arguments:
 *	handle	Handle of camera device
 *	sensor	Index of desired sensor
 * Return
 *	0	succeed
 *	<0	fail
 */
int camera_close(int handle);

/* Configure camera with specified format. Each platform and sensor
 * may have special limitation on the format and picture size. Please
 * refer to associated release notes about the limitation.
 * Arguments
 *	handle	Camera device handle
 *	setting	Desired capture settings; some fields are optional.
 *		Data fields those are not initialized must be cleared
 *		to zero.
 * Return
 *	0	succeed
 *	<0	fail
 */
#define	CAM_MODE_VIDEO	0
#define	CAM_MODE_STILL	1
struct pxacam_setting {
	/* Mandantary settings */
 	/* Still image capture or video capture mode. The value can be 
	   CAM_MODE_VIDEO or CAM_MODE_STILL*/
	int	mode;
 	/* Desired format*/
	enum	pxavid_format format;
 	/* Desired picture width and height*/
	int	width;
	int	height;

	/* Optional settings */
};

int camera_config(int handle, struct pxacam_setting* setting);

/* Start camera capture process. Application can only start camera after
 * it configures camera properly. Application can only get pictures after
 * it starts camera.
 * Arguments
 *	handle	Camera device handle
 * Return
 *	0	succeed
 *	<0	fail
 */
int camera_start(int handle);
/* Stop camera capture process. Application can only stop camera after 
 * it release all frames it have got. 
 * Arguments
 *	handle	Camera device handle
 * Return
 *	0	succeed
 *	<0	fail
 */
int camera_stop(int handle);

/* Get a video frame from camera. Application can only get video frame
 * after it starts camera. Application can all this functions several
 * times to attain more than one frames and release them together later.
 * Application must not release the video frame got by this function by
 * itself. It must call camera_release_frame to return the video frame.
 * Arguments
 *	handle	Camera device handle
 * Return
 *	!=NULL	A valid video buffer
 *	NULL	fail
 */
struct pxa_video_buf* camera_get_frame(int handle);

/* Release a video frame that's got by camera_get_frame. 
 * Arguments
 *	handle	Camera device handle
 *	vidbuf	The video buffer object that's returned by camera_get_frame.
 * Return
 *	0	succeed
 *	<0	fail
 */
int camera_release_frame(int handle,struct pxa_video_buf* vidbuf);

////////////////////////////////////////////////////////////////////////
// Audio Sample Code Interface
// 1. audio_open
// 2. audio_close
// 3. audio_write
// 4. audio_flush
// 5. audio_paush
// 6. audio_stop
// 7. audio_read
// 8. audio_set_volume
// 9. audio_mute
//
// Aplication shall open an audio handle to play or record sound. Each
// handle could only be open for play or record at a time(cannot be both).
// However, application could open two handles (one for play and one for
// record) at the same time.
//
// Application could change volume at any time and it's not necessary
// to open an audio handle before changing volume. Application could 
// record, playback and set volume in different thread or process.
//
////////////////////////////////////////////////////////////////////////

/* Open the default playback device with specified settings.
 * Arguments:
 *	dev	Sound device name; if this parameter is NULL, the program
 *		will use default sound device for input and output.
 *	mode	SND_PCM_STREAM_PLAYBACK or SND_PCM_STREAM_CAPTURE
 *	format	data format; could be SND_PCM_FORMAT_S16_LE or
 *		SND_PCM_FORMAT_S8 or SND_PCM_FORMA_U8
 *	s_rate	sample rate, could be any standard sample rate between
 *		8000 and 48000.
 *	channel	1 (mono) or 2 (stereo) channel
 *	
 * Return:
 *	NULL	fail
 *	!=NULL	audio handle that could be used by other audio function
 */
int audio_open(char* dev, int mode, int format, int sample_rate,int channel);

/* Close audio handle. Stop playback or record process if it's needed.
 * Arguments:
 *	handle	audio handle that's created by audio_open
 */
void audio_close(int handle);

/* Write a buffer to audio playback device and the sound will be played.
 * This function returns after the buffer is copied to audio device buffer.
 * It doesn't wait till the audio sample is output to speaker. The handle
 * must be open for playback.
 * Arguments:
 *	handle	audio handle that's created by audio_open
 *	buf	audio data buffer
 *	sample	sample number (not byte count) that would be played
 * Return:
 *	0	succeed
 *	<0	fail
 */
int audio_write(int handle, char* buf, int sample);

/* Flush playback buffer to make sure all audio samples in the buffer
 * are played on speaker. 
 * Arguments:
 *	handle	audio handle, must be created for playback
 */
int audio_flush(int handle);

/* Pause a playback or recording process or resume the process. 
 * Arguments:
 *	handle	audio handle
 *	enable	1 to pause and 0 for resume
 * Return:
 *	0	succeed
 *	<0	fail
 */
int audio_pause(int handle, int enable);

/* Stop a playback or recording process immediately. All pending buffer will be dropped. 
 * Arguments:
 *	handle	audio handle
 * Return:
 *	0	succeed
 *	<0	fail
 */
int audio_stop(int handle);

/* Read buffer from audio recording device. This function will block until
 * the it gets all requested samples. The handle must be open for record.
 * Arguments:
 *	handle	audio handle that's created by audio_open
 *	buf	data buffer to store audio samples
 *	sample	sample number (not byte count) that will be recorded
 *Return:
 *	>=0	actual buffer length the function reads
 *	<0	fail
 */
int audio_read(int handle, char* buf, int sample);

/* Set volume of specific channel of master volume. This function could be
 * called at any time and the setting will take effect immediately.
 * Arguments:
 *	channel	Target channel or master volume, could be following value
 *		PXA_AUDIO_MASTER_VOL
 *		PXA_AUDIO_HEADSET_VOL
 *		PXA_AUDIO_SPEAKER_VOL
 *		PXA_AUDIO_MIC1_VOL
 *	volume	Desired volume setting; should be between 0 (mute) and 100
 * Return:
 *	0	succeed
 *	<0	fail
 */
// int audio_set_volume(int channel, int volume);

/* Mute or unmute specific audio channel. When a channel is unmuted, the
 * orginal volume setting before mute is applied.
 * Arguments:
 *	channel	Target channel or master volume; use same definition as 
 *		function audio_set_volume
 *	enable	1 - mute the channel
 *		0 - unmute the channel
 * Return:
 *	0	succeed
 *	<0	fail
 */
// int audio_mute(int channel, int enable);

////////////////////////////////////////////////////////////////////////
// Graphics Library Sample Code Interface
// 1. img_open
// 2. img_close
// 3. img_resize
// 4. img_sync
// 5. img_alloc_buf
// 6. img_free_buf
// Application must open a graphics handle before using any image 
// processing functions like resize. Graphics library has special 
// requirement for video buffer. It could only accept a buffer that's
// allocated by graphics library buffer management function (e.g.
// img_alloc_buf). Passing a buffer that's not allocated by these
// functions may introduce unpredictable result. 
//
// The buffer management functions (img_alloc_buf) assures that the
// allocated buffer is compatible with other drivers of PXA Linux. 
// The buffer could be used by camera and LCD driver as well.
////////////////////////////////////////////////////////////////////////
/* Open image processing handle.
 * Return:
 *	NULL	fail
 *	!=NULL	Image processing handle
 */
int img_open(void);
/* Close the image processing handle.
 * Arguments:
 *	handle	Handle that will be closed
 */
void img_close(int handle);
/* Resize an image. Both width and height of the source picture must be
 * extended or shrunk at the same time. Application cannot extend width
 * while height is shrunk, vice versa. Source image format must be 
 * identical to destination image format. This function may return before
 * the real resize option is completed. Application must call img_sync
 * to make sure the operation is completed.
 * 
 * Auguments:
 *	handle	Image processing handle
 *	srcbuf	Source image buffer; must be a valid frame
 *	srcrect	Optional; it defines a subpicture in the source frame
 *		which will be resized. If this parameter is NULL, whole
 *		source frame will be processed.
 *	dstbuf	Destination image buffer; application must initialize
 *		the structure before calls this function.
 *	dstrect	Optional; it defines a subpicture in the destination
 *		frame which the picture will be resized to. If this
 *		parameter is NULL, the resized picture will fill whole
 *		destination frame.
 * Return:
 *	0	succeed
 *	<0	fail
 */
int img_resize(int handle, 
		struct pxa_video_buf* srcbuf, struct pxa_rect* srcrect,
		struct pxa_video_buf* dstbuf, struct pxa_rect* dstrect);
/* Graphics interfaces like img_resize are asynchronous interface. In other
 * words, these functions will return before the operation is completed. 
 * This function is used to synchronize such interface. It will block until
 * all outstanding graphics requests are completed.
 * Auguments:
 *	handle	Image processing handle
 * Return:
 *	0	succeed
 *	<0	fail
 */
int img_sync(int handle);
/* This function is used to allocate a video buffer and return the buffer
 * pointer to caller. The allocated buffer is assured to be compatible with
 * PXA image processor, camera, video accelerator and display device. 
 * Application must use this function to alloate buffer for image processing
 * functions.
 * Arguments:
 *	buffer	Pointer to a data structure that indicates buffer format. 
 *		It's also used to return the buffer pointer. Caller must 
 *		initialize width, height and format field of this structure.
 *		The "ycbcr" or "rggb10" field will be initialized if the
 *		buffer is allocated successfully.
 * Return:
 *	0	succeed
 *	<0	fail
 */
int img_alloc_buf(struct pxa_video_buf* buffer);
/* This function free a buffer that's allocated by img_alloc_buf. Buffer
 * that's not allocated by that function cannot be freed with this function.
 * Arguments:
 *	buffer	Pointer to a buffer that will be freed
 * Return:
 *	0	succeed
 *	<0	fail
 */
int img_free_buf(struct pxa_video_buf* buffer);

#endif

