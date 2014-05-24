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
#include <getopt.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/types.h>

#include <pxa_lib.h>

#define FORMAT_PLANAR_422       0x3
#define FORMAT_PLANAR_420       0x4
#define MIN(x, y)   ((x < y) ? x : y)

//#define  _ALIGN16(adr)   ((((Ipp32u)(adr))+15)&(~15))

struct overlay2_object{
	int	fd;
	enum	pxavid_format format;
	struct	pxa_rect window;
	unsigned char*	map;
	int	smem_len;
	struct	pxa_video_buf vidbuf;
};

int overlay2_open(char* dev, enum pxavid_format format, struct pxa_rect* rect, int w, int h, int w_off, int h_off)
{
	struct overlay2_object* devobj;
        struct fb_var_screeninfo var;
        struct fb_fix_screeninfo fix;
	int disformat;
	int result,mfb; 
	struct pxa_rect window; 

	ASSERT(format==pxavid_ycbcr422 || format==pxavid_ycbcr420);
	
	devobj = (struct overlay2_object*)malloc(sizeof(struct overlay2_object));
	ASSERT(devobj);
	memset(devobj,0,sizeof(struct overlay2_object));

	if(dev==NULL)
		dev = "/dev/fb2";
	devobj->fd = open(dev, O_RDWR);
	if(devobj->fd<0)
		goto ERROR_RETURN;

	if(rect==NULL){
		// FIXME
		// Open main framebuffer to know screen resolution
		mfb = open("/dev/fb", O_RDWR);
		result = ioctl(mfb, FBIOGET_VSCREENINFO, &var);
		ASSERT(result==0);
		printf("width=%d, height=%d\n",var.xres,var.yres);
		close(mfb);
		window.x=w_off; window.y=h_off;
		window.width=w; window.height=h;
//		window.width=var.xres; window.height=var.yres;
	}
	else{
		memcpy(&window,rect,sizeof(window));
	}
	DBGMSG("LCD: x=%d, y=%d, w=%d, h=%d\n",window.x,window.y,
		window.width,window.height);
	memset(&var,0,sizeof(var));
	var.xoffset = window.x;
	var.yoffset = window.y;
	var.xres = window.width;
	var.yres = window.height;
	switch(format){
	case pxavid_ycbcr422:
		var.bits_per_pixel = 16;
		break;
	case pxavid_ycbcr420:
		var.bits_per_pixel = 16;
		break;
	default:
		ASSERT(0);
	}

	// TODO
	// Convert external format to LCD format. This macro shall be 
	// defined in LCD driver.
	switch(format){
	case pxavid_ycbcr422:
		disformat = FORMAT_PLANAR_422;
		break;
	case pxavid_ycbcr420:
		disformat = FORMAT_PLANAR_420;
		break;
	default:
		// Other format are not supported
		ASSERT(0);
	}
	var.nonstd = (disformat <<20) | (window.y<< 10) | window.x;
        // set "var" screeninfo

	result = ioctl(devobj->fd, FBIOPUT_VSCREENINFO, &var);
	ASSERT(result==0);

	// get updated screen information
	result = ioctl(devobj->fd, FBIOGET_FSCREENINFO, &fix);
	ASSERT(result==0);
	result = ioctl(devobj->fd, FBIOGET_VSCREENINFO, &var);
	ASSERT(result==0);

	devobj->map = (unsigned char*)mmap(0, fix.smem_len,
			PROT_READ | PROT_WRITE, MAP_SHARED, devobj->fd, 0);
	ASSERT(devobj->map != MAP_FAILED);

	// Save video buffer to overlay object
	ASSERT(var.xres%2==0);
	devobj->vidbuf.ycbcr.y = devobj->map+var.red.offset;
	devobj->vidbuf.ycbcr.ystep = var.xres;
	devobj->vidbuf.ycbcr.cb = devobj->map+var.green.offset;
	devobj->vidbuf.ycbcr.cbstep = var.xres/2;
	devobj->vidbuf.ycbcr.cr = devobj->map+var.blue.offset;
	devobj->vidbuf.ycbcr.crstep = var.xres/2;
	devobj->vidbuf.width = window.width;
	devobj->vidbuf.height = window.height;
	devobj->vidbuf.format = format;

	//DBGMSG("y=%x, ystep=%d\n",devobj->vidbuf.ycbcr.y,devobj->vidbuf.ycbcr.ystep);
	//DBGMSG("cb=%x, cbstep=%d\n",devobj->vidbuf.ycbcr.cb,devobj->vidbuf.ycbcr.cbstep);
	//DBGMSG("cr=%x, crstep=%d\n",devobj->vidbuf.ycbcr.cr,devobj->vidbuf.ycbcr.crstep);


	devobj->smem_len = fix.smem_len;
	memcpy(&devobj->window,&window,sizeof(window));


	devobj->format = format;

	return (int)devobj;

ERROR_RETURN:
	if(devobj->fd>=0)
		close(devobj->fd);
	free(devobj);
	return 0;
}

void overlay2_close(int handle)
{
	int result;
	struct overlay2_object* devobj;

	devobj = (struct overlay2_object*)handle;
	ASSERT(devobj && devobj->map && devobj->smem_len);

	result = munmap(devobj->map, devobj->smem_len);
	ASSERT(result==0);
	close(devobj->fd);
	memset(devobj,0,sizeof(struct overlay2_object));
	free(devobj);
	return;
}

// Map display buffer to application work space
// IN		fd, format, rect
//		rect defines the window size. If it's NULL, the display
//		will be full screen
// OUT		vidbuf; return mapped video if the pointer is not NULL
// format	0	RGB
//		2	YCbCr444
//		3	YCbCr422
//		4	YCbCr420
int overlay2_getbuf(int handle, struct pxa_video_buf* vidbuf)
{
	struct overlay2_object* devobj;

	devobj = (struct overlay2_object*)handle;
	ASSERT(devobj && devobj->map && devobj->smem_len);
	ASSERT(vidbuf);
	memcpy(vidbuf,&devobj->vidbuf,sizeof(devobj->vidbuf));
	return 0;
} 

// Write a video frame buffer to overlay. This function only supports
// fill from top left of overlay display buffer. If the source picture
// is larger than overlay size, the picture will be cut down to fit
// the screen.
int overlay2_write(int handle, struct pxa_video_buf* srcbuf)
{
	int i;
	int src_heightY, src_heightCb, src_heightCr;
	int dstHeightY, dstHeightCb, dstHeightCr;
	int srcstepY, srcstepCb, srcstepCr;
	int dststepY, dststepCb, dststepCr;
	unsigned char* srcPtr, *dstPtr;
	int miniStep, miniHeight;
	struct overlay2_object* devobj;

	devobj = (struct overlay2_object*)handle;
	ASSERT(devobj && devobj->map && devobj->smem_len);

	ASSERT(pxavid_ycbcr422==devobj->format || pxavid_ycbcr420==devobj->format);

	if (devobj->format == pxavid_ycbcr422)
	{
		ASSERT(srcbuf->ycbcr.ystep>=(unsigned int)srcbuf->width);
		src_heightY      = srcbuf->height;
		src_heightCb     = srcbuf->height;
	        src_heightCr     = srcbuf->height;
        	dstHeightY   = devobj->vidbuf.height;
	        dstHeightCb  = devobj->vidbuf.height;
        	dstHeightCr  = devobj->vidbuf.height;
	        srcstepY        = srcbuf->width;
        	srcstepCb       = srcbuf->width/2;
	        srcstepCr       = srcbuf->width/2;
        	dststepY     = devobj->vidbuf.ycbcr.ystep;
	        dststepCb    = devobj->vidbuf.ycbcr.cbstep;
        	dststepCr    = devobj->vidbuf.ycbcr.crstep;
	}
	else if (devobj->format == pxavid_ycbcr420)
	{
		ASSERT(srcbuf->ycbcr.ystep>=(unsigned int)srcbuf->width);
		src_heightY      = srcbuf->height;
		src_heightCb     = srcbuf->height/2;
	        src_heightCr     = srcbuf->height/2;
        	dstHeightY   = devobj->vidbuf.height;
	        dstHeightCb  = devobj->vidbuf.height/2;
        	dstHeightCr  = devobj->vidbuf.height/2;
	        srcstepY        = srcbuf->width;
        	srcstepCb       = srcbuf->width/2;
	        srcstepCr       = srcbuf->width/2;
        	dststepY     = devobj->vidbuf.ycbcr.ystep;
	        dststepCb    = devobj->vidbuf.ycbcr.cbstep;
        	dststepCr    = devobj->vidbuf.ycbcr.crstep;
	}
	else{
		ERRMSG("Invalid picture format\n");
		ASSERT(0);
	}

	// copy Y component
	srcPtr = srcbuf->ycbcr.y;
	dstPtr = devobj->vidbuf.ycbcr.y;
	miniStep = MIN(srcstepY, dststepY);
	miniHeight = MIN(src_heightY, dstHeightY);
	for (i = 0; i < miniHeight; i++)
	{
        	memcpy(dstPtr, srcPtr, miniStep);
		srcPtr += srcbuf->ycbcr.ystep;
	        dstPtr += dststepY;
	}

	// copy Cb component
	srcPtr = srcbuf->ycbcr.cb;
	dstPtr = devobj->vidbuf.ycbcr.cb;
	miniStep = MIN(srcstepCb, dststepCb);
	miniHeight = MIN(src_heightCb, dstHeightCb);
	for (i = 0; i < miniHeight; i++)
	{
	        memcpy( dstPtr,srcPtr, miniStep);
		srcPtr += srcbuf->ycbcr.cbstep;
	        dstPtr += dststepCb;
	}

	// copy Cr component
	srcPtr = srcbuf->ycbcr.cr;
	dstPtr = devobj->vidbuf.ycbcr.cr;
	miniStep = MIN(srcstepCr, dststepCr);
	miniHeight = MIN(src_heightCr, dstHeightCr);
	for (i = 0; i < miniHeight; i++)
	{
	        memcpy(dstPtr, srcPtr, miniStep);
		srcPtr += srcbuf->ycbcr.crstep;
	        dstPtr += dststepCr;
	}

	return 0;
}

// IN		srcbuf, rotate
// srcbuf	source video buffer
int display_write(int handle, struct pxa_video_buf* srcbuf)
{
	int	ret;
	struct overlay2_object* devobj;
	unsigned char dst_ori[320*240*3];
		
	devobj = (struct overlay2_object*)handle;
	ASSERT(devobj && devobj->map && devobj->smem_len);

	ASSERT(pxavid_ycbcr422==devobj->format ||
		pxavid_ycbcr420==devobj->format);
	ASSERT(srcbuf->format==devobj->format);

	if(pxavid_ycbcr422==devobj->format)
	{
		static FILE* fp=NULL;
		static int pvcount=0;
		pvcount++;
		if(fp==NULL&&pvcount==10)
		{
			fp=fopen("test.yuv","wb");
			fwrite(srcbuf->ycbcr.y,1,srcbuf->width*srcbuf->height,fp);
			fwrite(srcbuf->ycbcr.cb,1,srcbuf->width*srcbuf->height/2,fp);
			fwrite(srcbuf->ycbcr.cr,1,srcbuf->width*srcbuf->height/2,fp);
			fclose(fp);
			printf("Write the YUV file successfully %d %d\n",
					srcbuf->width, srcbuf->height);

			exit(0);
		}
	}

	return 0;
}
