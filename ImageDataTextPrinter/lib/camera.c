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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/time.h>

#include <getopt.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/types.h>

#include <sys/time.h>
#include <videodev2.h>

#include <pxa_camera_zl.h>
//#include <imm.h>

#include <pxa_lib.h>

///////////////////////////////////////////////////////////////////////
// Camera Object
///////////////////////////////////////////////////////////////////////
#define	PAGE_SIZE	(4096)
#define	PAGE_MASK	(~((unsigned int)PAGE_SIZE-1))

struct videobuf_dev{
	unsigned char *buf;
	unsigned int length;
};

struct ext_video_buf{
	struct pxa_video_buf	pxabuf;
	struct v4l2_buffer	v4l2buf;
};

static void cambuf_to_ycbcr422(struct videobuf_dev* simple, 
			struct pxa_video_buf* vidbuf,
			int width,int height)
{
	int ylen,cblen;

	ASSERT((width%4)==0);

	vidbuf->ycbcr.y = simple->buf;
	vidbuf->ycbcr.ystep = width;
	ylen = width*height;

	vidbuf->ycbcr.cb = (unsigned char*)(((unsigned int)vidbuf->ycbcr.y + 
				ylen + PAGE_SIZE-1) & PAGE_MASK);
	vidbuf->ycbcr.cbstep = width/2;
	cblen = (width*height)/2;

	vidbuf->ycbcr.cr = (unsigned char*)(((unsigned int)vidbuf->ycbcr.cb + 
				cblen + PAGE_SIZE-1) & PAGE_MASK);
	vidbuf->ycbcr.crstep = width/2;

	vidbuf->format = pxavid_ycbcr422;
	vidbuf->width = width;
	vidbuf->height = height;

	//DBGMSG("simple = 0x%08X, len = %d\n",simple->buf,simple->length);
	//DBGMSG("y=0x%08X, cb=0x%08X, cr=0x%08X\n",
	//		vidbuf->ycbcr.y,vidbuf->ycbcr.cb,vidbuf->ycbcr.cr);
}

static void cambuf_to_ycbcr420(struct videobuf_dev* simple, 
			struct pxa_video_buf* vidbuf,
			int width,int height)
{
	int ylen,cblen;

	ASSERT((width%4)==0);
	vidbuf->ycbcr.y = simple->buf;
	vidbuf->ycbcr.ystep = width;
	ylen = width*height;
	vidbuf->ycbcr.cb = (unsigned char*)(((unsigned int)vidbuf->ycbcr.y + 
				ylen + PAGE_SIZE-1) & PAGE_MASK);
	vidbuf->ycbcr.cbstep = width/4;
	cblen = (width*height)/4;
	vidbuf->ycbcr.cr = (unsigned char*)(((unsigned int)vidbuf->ycbcr.cb + 
				cblen + PAGE_SIZE-1) & PAGE_MASK);
	vidbuf->ycbcr.crstep = width/4;
	vidbuf->format = pxavid_ycbcr420;
	vidbuf->width = width;
	vidbuf->height = height;
}

// VIDEOBUF_COUNT must be larger than STILLBUF_COUNT
#define	VIDEOBUF_COUNT	3
#define	STILLBUF_COUNT	2

#define	CAM_STATUS_INIT		0
#define	CAM_STATUS_READY	1
#define	CAM_STATUS_BUSY		2

struct pxa_camera{
	int	handle;
	int	status;
	int	mode;
	int	sensor;
	int	ref_count;

	// Video Buffer
	int	width;
	int	height;
	enum	pxavid_format format;
	struct	videobuf_dev	cambuf[VIDEOBUF_COUNT];
};

///////////////////////////////////////////////////////////////////////
// External API
///////////////////////////////////////////////////////////////////////
int camera_open(char* camname,int sensor)
{
	int handle;
	struct pxa_camera* camobj = NULL;

	// Initialize camera object
	memset(&camobj,0,sizeof(camobj));

	if(camname==NULL){
		camname="/dev/video0";
	}

	handle = open(camname, O_RDONLY);
	if (handle<0) {
		ERRMSG("cann't open camera %s (%d)\n",camname,errno);
		ASSERT(handle>=0);
		return 0;
	}

	if(ioctl(handle, VIDIOC_S_INPUT, &sensor)<0) {
        	ERRMSG("can't set input\n");
	        close(handle);
        	return 0;
	}

	// Initialize camera object
	camobj = malloc(sizeof(struct pxa_camera));
	memset(camobj,0,sizeof(struct pxa_camera));
	ASSERT(camobj);
	camobj->handle = handle;
	camobj->status = CAM_STATUS_READY;
	camobj->width = 0;
	camobj->height = 0;
	camobj->sensor = sensor;

	ASSERT(sizeof(int)==sizeof(struct pxa_camera*));
	return (int)camobj;
//	return handle;
}

int camera_config(int hcam,struct pxacam_setting* setting)
{
	struct  v4l2_format         vformat;
	struct  v4l2_streamparm     streamparm;
	struct  v4l2_requestbuffers requestbuffers;
	struct  v4l2_buffer         buffer;
	struct	pxa_camera* camera;
	void    *pmapped = NULL;
	int	count,i;
	int	mode;
	enum	pxavid_format format;
	int	width, height;

	mode = setting->mode;
	format = setting->format;
	width = setting->width;
	height = setting->height;

	// set video stream parameter
	camera = (struct pxa_camera*)hcam;
	ASSERT(camera->handle>0 && camera->status!=CAM_STATUS_BUSY);
	ASSERT(camera->ref_count==0);

	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	streamparm.parm.capture.timeperframe.numerator      = 0;
	streamparm.parm.capture.timeperframe.denominator    = 0;
	streamparm.parm.capture.capturemode = mode;
	streamparm.parm.capture.extendedmode = CI_SSU_SCALE_DISABLE;

	printf("camera_config :  streamparm.type = %d\n",  streamparm.type);
	if(ioctl(camera->handle, VIDIOC_S_PARM, &streamparm)<0) { 
		ERRMSG("can't set camera parameter\n");
		return -1;
	}

	// Prepare video buffer for the first configuration

	// Set video format
	vformat.type                 = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// Only support three pixel formats
	ASSERT(format==pxavid_rggb10 || format==pxavid_ycbcr422 || format==pxavid_ycbcr420);
	switch(format){
	case pxavid_rggb10:
		vformat.fmt.pix.pixelformat  = V4L2_PIX_FMT_SRGGB10;
		break;
	case pxavid_ycbcr422:
		vformat.fmt.pix.pixelformat  = V4L2_PIX_FMT_YUV422P;
		break;
	case pxavid_ycbcr420:	
		vformat.fmt.pix.pixelformat  = V4L2_PIX_FMT_YUV420;
		break;
	default:
		ASSERT(0);
		break;
	}

	vformat.fmt.pix.width = width;
	vformat.fmt.pix.height = height;

	if(ioctl(camera->handle, VIDIOC_S_FMT, &vformat)<0) {
        	ERRMSG("can't set camera format\n");
		goto FAIL_EXIT;
	}

	// Release previous allocated buffer if it's applicable
	if(mode==CAM_MODE_VIDEO){
		count = VIDEOBUF_COUNT;
	}else{
		count = STILLBUF_COUNT;
	}
	for (i = 0; i < count; ++i) {
		if(camera->cambuf[i].buf){
			munmap(camera->cambuf[i].buf,
				camera->cambuf[i].length);
		}
	}
	memset(camera->cambuf,0,sizeof(camera->cambuf));

	// Request buffers
	requestbuffers.type     = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	requestbuffers.memory   = V4L2_MEMORY_MMAP;
	requestbuffers.count    = count;
	printf("count = %d\n", count);
	if(ioctl(camera->handle, VIDIOC_REQBUFS, &requestbuffers)<0) {
        	DBGMSG("can't require camera buffer");
		goto FAIL_EXIT;
	}

	if (requestbuffers.count < (unsigned int)count ) {
	        ERRMSG("Insufficient camera buffer allocated\n");
		goto FAIL_EXIT;
    	}

	// do memory map 
	for (i = 0; i < (int)requestbuffers.count; i++) {
        	memset (&buffer, 0, sizeof(buffer));
	        buffer.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        	buffer.memory      = V4L2_MEMORY_MMAP;
	        buffer.index       = i;

        	if (ioctl (camera->handle, VIDIOC_QUERYBUF, &buffer)<0) {
			goto FAIL_EXIT;
	        }

        	pmapped = mmap (NULL, buffer.length, PROT_READ ,
				MAP_SHARED, camera->handle, buffer.m.offset);
	        if (MAP_FAILED == pmapped) {
        		ERRMSG("can't do mmap of camera buffer\n");
			goto FAIL_EXIT;
	        }
		ASSERT(camera->cambuf[i].buf==0);
		camera->cambuf[i].buf = pmapped;
		camera->cambuf[i].length = buffer.length;
	}

	// queue buffer for capture
	for (i = 0; i < (int)requestbuffers.count; i++) {
        	memset (&buffer, 0, sizeof(buffer));
		buffer.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        	buffer.memory      = V4L2_MEMORY_MMAP;
	        buffer.index       = i;

        	if (ioctl (camera->handle, VIDIOC_QBUF, &buffer)<0) {
			ERRMSG("fatal error - fail to queue buffer\n");
			ASSERT(0);
			goto FAIL_EXIT;
        	}
	}

	// Save configuration to device object
	camera->mode = mode;
	camera->width = width;
	camera->height = height;
	camera->format = format;
	camera->status = CAM_STATUS_READY;
	return 0;

FAIL_EXIT:
	for (i = 0; i < VIDEOBUF_COUNT; ++i) {
		if(camera->cambuf[i].buf){
			munmap(camera->cambuf[i].buf,
				camera->cambuf[i].length);
		}
	}
	memset(camera->cambuf,0,sizeof(camera->cambuf));
	camera->width = 0;
	camera->height = 0;
	camera->format = pxavid_invalid;
	camera->status = CAM_STATUS_INIT;

	return -1;
}

int camera_start(int hcam)
{
	int buftype;
	struct	pxa_camera* camera;

	// set video stream parameter
	camera = (struct pxa_camera*)hcam;
	ASSERT(camera->status==CAM_STATUS_READY);

	buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl (camera->handle, VIDIOC_STREAMON, &buftype)<0) {
        	ERRMSG("can't start camera streaming\n");
		return -1;
        }
	camera->status = CAM_STATUS_BUSY;
	
 	return 0;
}

struct pxa_video_buf*  camera_get_frame(int hcam)
{
	int ret;
	struct  pollfd ufds;
	struct	v4l2_buffer* buffer;
	struct	pxa_camera* camera;
	struct	ext_video_buf*	extbuf;
	struct	pxa_video_buf*	vidbuf;

	// set video stream parameter
	camera = (struct pxa_camera*)hcam;
	ASSERT(camera->status==CAM_STATUS_BUSY);
	ASSERT(camera->ref_count<VIDEOBUF_COUNT);

	// FIXME
	// This function maybe obsolete
	ufds.fd         = camera->handle;
	ufds.events     = POLLIN;
        do {
		ret = poll(&ufds, 1, -1);
        } while (!ret);

	extbuf = (struct ext_video_buf*)malloc(sizeof(struct ext_video_buf));
	ASSERT(extbuf);
	vidbuf = &extbuf->pxabuf;
	buffer = &extbuf->v4l2buf;
        memset(buffer, 0, sizeof(struct v4l2_buffer));
        buffer->type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer->memory  = V4L2_MEMORY_MMAP;

        if (ioctl (camera->handle, VIDIOC_DQBUF, buffer)<0) {
		ERRMSG("can't dequeue camera buffer\n");
		return NULL;
        }

	// Return buffer information
	if(camera->format==pxavid_rggb10){
		vidbuf->width = camera->width;
		vidbuf->height = camera->height;
		vidbuf->format = pxavid_rggb10;
		vidbuf->rggb10.buf = (unsigned short*)camera->cambuf[buffer->index].buf;
		vidbuf->rggb10.len = camera->cambuf[buffer->index].length;

	}else if(camera->format==pxavid_ycbcr422){
		cambuf_to_ycbcr422(&camera->cambuf[buffer->index],vidbuf,
					camera->width,camera->height);
	}else if(camera->format==pxavid_ycbcr420){
		cambuf_to_ycbcr420(&camera->cambuf[buffer->index],vidbuf,
					camera->width,camera->height);
	}else {ASSERT(0);}

	camera->ref_count++;

	return (struct pxa_video_buf*)extbuf;
}

int camera_release_frame(int hcam,struct pxa_video_buf* vidbuf)
{
	struct	pxa_camera* camera;
	struct	ext_video_buf* extbuf;
	// set video stream parameter
	camera = (struct pxa_camera*)hcam;
	ASSERT(camera->status==CAM_STATUS_BUSY);
	ASSERT(camera->ref_count>0);

	extbuf = (struct ext_video_buf*)vidbuf;

        if (ioctl (camera->handle, VIDIOC_QBUF, (void*)&extbuf->v4l2buf)<0) {
		DBGMSG("can't enqueue buffer\n");
		return -1;
        }

	memset(extbuf,0,sizeof(struct ext_video_buf));
	free(extbuf);
	camera->ref_count--;

	return 0;	
}

int camera_stop(int hcam)
{
	struct	pxa_camera* camera;
	enum v4l2_buf_type buftype;
	// set video stream parameter
	camera = (struct pxa_camera*)hcam;
	ASSERT(camera->status==CAM_STATUS_BUSY);
	ASSERT(camera->ref_count==0);

	buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl (camera->handle, VIDIOC_STREAMOFF, &buftype)<0) {
        	DBGMSG("can't stop streaming\n");
                return -1;
        }
	
	camera->status = CAM_STATUS_READY;
	return 0;
}

int camera_close(int hcam)
{
	int i;
	struct	pxa_camera* camera;
	// set video stream parameter
	camera = (struct pxa_camera*)hcam;
	ASSERT(camera->status!=CAM_STATUS_BUSY);
	ASSERT(camera->ref_count==0);

	for(i=0;i<VIDEOBUF_COUNT;i++){
		if (camera->cambuf[i].buf){
			munmap(camera->cambuf[i].buf,
				camera->cambuf[i].length);
		}
	}
	close(camera->handle);
	memset(camera,0,sizeof(struct pxa_camera));
	camera->handle = -1;
	free(camera);
	return 0;
}


