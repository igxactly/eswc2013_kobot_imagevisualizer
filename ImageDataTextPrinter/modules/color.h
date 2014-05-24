#include <string.h>

#define _TRUE_ 1
#define _FALSE_ 0

#define MAX_X 320
#define MAX_Y 240

#define max(a,b) (((a)>(b))? (a):(b))
#define min(a,b) (((a)<(b))? (a):(b))

#define MAX_3(a, b, c)	( ((a)>(b)) ? ( (((a)>(c)) ? (a) : (c)) ) : ( (((b)>(c)) ? (b) : (c)) ) )
#define MIN_3(a, b, c)	( ((a)>(b)) ? ( (((b)>(c)) ? (c) : (b)) ) : ( (((a)>(c)) ? (c) : (a)) ) )

#define NOISE_COUNT 2

typedef struct{
  struct{
    unsigned char y[MAX_X*MAX_Y]; 
    unsigned char cb[MAX_X*MAX_Y/2];
    unsigned char cr[MAX_X*MAX_Y/2];
  }ycbcr;
} VideoCopy;

typedef struct {
  int hmin;
  int hmax;
  int smin;
  int smax;
  int vmin;
  int vmax;
} ColorBoundary;

typedef struct {
	int y;
	int cb;
	int cr;
} YCbCr422;

typedef struct {
  float h;
  float s;
  float v;
} HSV;

typedef struct {
	float r;
	float g;
	float b;
} RGBf;

int isColor(HSV hsv, ColorBoundary color_B);
HSV getHSV(VideoCopy* buf,int x, int y);
YCbCr422 getYCbCr422(VideoCopy* buf, int x, int y);
HSV yCbCr2HSV(YCbCr422 ycbcr);
RGBf yCbCr2RGB(YCbCr422 ycbcr);
HSV RGB2HSV(RGBf rgb);

extern const ColorBoundary blue_B;
extern const ColorBoundary red_B;
extern const ColorBoundary yellow_B;
extern const ColorBoundary green_B;
extern const ColorBoundary error_B;
