/* 2013 FIRA HuroCup 
 *
 *	Title: Penalty Kick
 *
 *	Author:	Kobot (Kookmin Univ. CS)
 */

#define COMMAND_DELAY 300000

#define _TRUE_ 1
#define _FALSE_ 0

#define MAX_X 320
#define MAX_Y 240

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <termios.h>
#include <pthread.h> 
#include <fcntl.h>

#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

// Front Board, Camera Library
#include <pxa_lib.h>
#include <pxa_camera_zl.h>
#include "modules/color.h"

// Serial 
#define BAUDRATE B4800
#define MODEDEVICE "/dev/ttyS2"

// 3axis
// #define MMA_SLEEP_MODE_ON 0x1001
// #define MMA_SLEEP_MODE_OFF 0x1002
// #define MMA_VIN_ON 0x1003
// #define MMA_VIN_OFF 0x1004
// #define MMA_SENS_15G 0x1005
// #define MMA_SENS_20G 0x1006
// #define MMA_SENS_40G 0x1007
// #define MMA_SENS_60G 0x1008
