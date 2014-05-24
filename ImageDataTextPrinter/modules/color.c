#include "modules/color.h"
// #include <stdio.h>

const ColorBoundary blue_B = {190, 250, 40, 100, 40, 100};
const ColorBoundary red_B = {345, 15, 45, 100, 56, 100};
const ColorBoundary yellow_B = {50, 76, 50, 100, 81, 100};
const ColorBoundary green_B = {157, 170, 50, 100, 71, 100};
const ColorBoundary error_B = {-1, -1, -1, -1, -1, -1};

int isColor(HSV hsv, ColorBoundary color_B){
    if(color_B.hmax < color_B.hmin){    // 색이 Red 일 경우
	    if(hsv.h >= color_B.hmin && hsv.h <= 360 &&
            hsv.s >= color_B.smin && hsv.s <= color_B.smax &&
            hsv.v >= color_B.vmin && hsv.v <= color_B.vmax)

            return _TRUE_;

        else if(hsv.h >= 0 && hsv.h <= color_B.hmax &&
            hsv.s >= color_B.smin && hsv.s <= color_B.smax &&
            hsv.v >= color_B.vmin && hsv.v <= color_B.vmax)

            return _TRUE_;

        else
            return _FALSE_;
    }

    else{    // 그 외 경우

        if(hsv.h >= color_B.hmin && hsv.h <= color_B.hmax &&
            hsv.s >= color_B.smin && hsv.s <= color_B.smax &&
            hsv.v >= color_B.vmin && hsv.v <= color_B.vmax)

            return _TRUE_;

        else
            return _FALSE_;
    }
}

HSV getHSV(VideoCopy* buf,int x, int y) {
	YCbCr422 ycbcr;
	HSV hsv;

	// printf("in getHSV\n");
	ycbcr = getYCbCr422(buf, x, y);
	// printf("ycbcr value: %d %d %d ", ycbcr.y, ycbcr.cb, ycbcr.cr);
	hsv = yCbCr2HSV(ycbcr);
	return hsv;
}

YCbCr422 getYCbCr422(VideoCopy* buf, int x, int y) {
	int index = y * MAX_X + x;
	YCbCr422 ycbcr;

	// printf("in getYCbCr422\n");
	ycbcr.y = buf->ycbcr.y[index];
	ycbcr.cb = buf->ycbcr.cb[index/2];
	ycbcr.cr = buf->ycbcr.cr[index/2];

	return ycbcr;
}

HSV yCbCr2HSV(YCbCr422 ycbcr) {
	RGBf rgb;
	HSV hsv;

	// printf("in yCbCr2HSV\n");
	rgb = yCbCr2RGB(ycbcr);	
	hsv = RGB2HSV(rgb);

	return hsv;
}

RGBf yCbCr2RGB(YCbCr422 ycbcr) {
	RGBf rgb;
	int y, cb, cr;

	// set ycbcr val
	y = ycbcr.y;
	cb = ycbcr.cb;
	cr = ycbcr.cr;

	// printf("in yCbCr2RGB\n");

	rgb.r = min( max( ((y-16))*255.0/219.0 + 1.402*((cr-128)*255.0)/224.0 + 0.5 ,  0 ) ,  255);
	rgb.g = min( max( ((y-16))*255.0/219.0 - 0.344*((cb-128)*255.0)/224.0 - 0.714*((cr-128)*255.0)/224.0 + 0.5 ,  0 ) ,  255);
	rgb.b = min( max( ((y-16))*255.0/219.0 + 1.772*((cb-128)*255.0)/224.0 + 0.5 , 0 ) ,  255);
	// printf("rgb: %.2f %.2f %.2f", rgb.r, rgb.g, rgb.b);

	return rgb;
}

HSV RGB2HSV(RGBf rgb){
	HSV hsv;
	float min, max, delta;
	
	int r, g, b;

	// printf("in RGB2HSV\n");

	r = rgb.r;
	g = rgb.g;
	b = rgb.b;
		
	min = MIN_3(r, g, b);
	max = MAX_3(r, g, b);
	hsv.v = max; // v
	
	delta = max - min;

	if(delta == 0){

		hsv.h = 0;
		hsv.s = 0;
		hsv.v = hsv.v / 255.0;
		
		return hsv;
	}

	if( max != 0 ) {
		hsv.s = delta / max; // s
	} else {
		// r = g = b = 0 // s = 0, v is undefined
		hsv.s = 0;
		hsv.h = -1;
		return hsv;
	}

	if( r == max )
		hsv.h = ( g - b ) / delta; // between yellow & magenta

	else if( g == max )
		hsv.h = 2 + ( b - r ) / delta; // between cyan & yellow

	else
		hsv.h = 4 + ( r - g ) / delta; // between magenta & cyan
	
	hsv.h *= 60; // degrees
	if( hsv.h < 0 )
		hsv.h += 360;

	hsv.s *= 100;
	hsv.v = hsv.v / 255 * 100;

	return hsv;
}
