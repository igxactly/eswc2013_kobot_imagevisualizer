/* 2013 FIRA HuroCup 
 *
 *	Title: Penalty Kick
 *
 *	Author:	Kobot (Kookmin Univ. CS)
 */

#include "main.h"

// 백보드, 3축 센서
int fdBackBoard;
int fdThreeAxis;

// 카메라, 오버레이
int fdCamera;
int fdOverlay2;

struct pxa_video_buf* vidbuf;
struct pxa_video_buf vidbuf_overlay;
int len_vidbuf;

VideoCopy bufCopy;

// Fuction Prototypes
void initDevices(void); // Initialize Camera and Sensors
int openSerial(void);
void endProgram(void);
void refreshImage(void);
int writeCommand(int cmd); // Write command to backboard uart port
void printMatrix(int interval);

// start of function main()
int main(int argc, char* argv[]) {
	// Initialization
	initDevices();
	usleep(3000000); // delay 3sec
	
	printMatrix(1);

	endProgram();
	return 0;
}

// Print HSV Color Matrix
void printMatrix(int interval) {
	HSV hsv;
	int i, j;

	refreshImage();
	for (i = MAX_Y - (interval / 2); i >= 0; i -= interval) {
		for (j = MAX_X - (interval / 2); j >= 0; j -= interval) {
			hsv = getHSV(&bufCopy, j, i);
			printf("%d,%d {%.2f,%.2f,%.2f}\n", j, i, hsv.h, hsv.s, hsv.v);
		}
		printf("\n\n");
	}
}

// Caculate direction and produce command value
void refreshImage(void) {
	//get frame from cam
	vidbuf = camera_get_frame(fdCamera);
	usleep(3000000); // delay 3sec
	camera_release_frame(fdCamera, vidbuf);
	vidbuf = camera_get_frame(fdCamera);

	//copy frame to var
	memcpy(vidbuf_overlay.ycbcr.y, vidbuf->ycbcr.y,len_vidbuf);
	memcpy(vidbuf_overlay.ycbcr.cb, vidbuf->ycbcr.cb,len_vidbuf/2);
	memcpy(vidbuf_overlay.ycbcr.cr, vidbuf->ycbcr.cr,len_vidbuf/2);
	
	memcpy(bufCopy.ycbcr.y,vidbuf->ycbcr.y,len_vidbuf);
    memcpy(bufCopy.ycbcr.cb,vidbuf->ycbcr.cb,len_vidbuf/2);
    memcpy(bufCopy.ycbcr.cr,vidbuf->ycbcr.cr,len_vidbuf/2);
   
	// !!essential!!
	// put image to screen & clean buffer 
	camera_release_frame(fdCamera, vidbuf);
}

// Write command to backboard uart port
int writeCommand(int cmd) {
	printf("command: %d\n", cmd);
	write(fdBackBoard, &cmd, 1);
}

// Initialize Camera and Sensors
void initDevices(void) {
	struct pxacam_setting camset;
	
	// Backboard uart init
	fdBackBoard = openSerial();

	// 3-axis sensor init 
	// fdThreeAxis = open("/dev/MMA_ADC", O_RDONLY);
	// ASSERT(fdThreeAxis);

	// ioctl(fdThreeAxis,MMA_VIN_ON, 0);
	// ioctl(fdThreeAxis,MMA_SLEEP_MODE_ON, 0);
	// ioctl(fdThreeAxis,MMA_SENS_60G, 0);

	// Camera init
	fdCamera = camera_open(NULL,0);
	ASSERT(fdCamera);

	memset(&camset,0,sizeof(camset));
	camset.mode = CAM_MODE_VIDEO;
	camset.format = pxavid_ycbcr422;

	camset.width = 320;
	camset.height = 240;

	camera_config(fdCamera,&camset);
	camera_start(fdCamera);

	fdOverlay2 = overlay2_open(NULL,pxavid_ycbcr422,NULL, 320, 240, 0 , 0);

	overlay2_getbuf(fdOverlay2, &vidbuf_overlay);
	len_vidbuf = vidbuf_overlay.width * vidbuf_overlay.height;

	// init finish
	printf("Initializing Device Finished\n");	
}

void endProgram(void) {
	int i, cntdown = 3;
	int cmd = 26;

	for (i = 0; i < cntdown; i++) {
		usleep(COMMAND_DELAY); // delay
		write(fdBackBoard, &cmd, 1);
		printf("command: %d\n", cmd);
	}

	camera_stop(fdCamera);
	camera_close(fdCamera);
}

int openSerial(void){
	char fd_serial[20];
	int fd;
	struct termios oldtio, newtio;

	strcpy(fd_serial, MODEDEVICE); //FFUART
	
	fd = open(fd_serial, O_RDWR | O_NOCTTY );
	if (fd <0) {
		printf("Serial %s  Device Err\n", fd_serial );
		exit(1);
    }
	printf("robot uart ctrl %s\n", MODEDEVICE);
    
    tcgetattr(fd,&oldtio); /* save current port settings */
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 8 chars received */
    
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);
    
    return fd;
}
