/*
 * Copyright (c) 2021 Ricardo JL Rufino.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  Created on: 9 de abr. de 2021
 *  Author: Ricardo JL Rufino
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#include "libmaix_cam.h"
#include "libmaix_image.h"
#include "quirc.h"

// App includes
#include "convert.h"
#include "log.h"
#include "app_settings.h"
#include "sound.h"


// TODO xxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// [] Play sound on capture
// [] Show led status
// [] Write qrcode to FIFO


int qrcode_fd;
int periph_fd; // gpio

// int res_w = 240, res_h = 240;
int res_w = 640, res_h = 480;
int enable_sound;

// Debug errors
char *out_dir = SNAPSHOT_DEFAULT_DIR;
int qr_error_count = 0;
int qr_save_onerror = 1; // used to debug.

//
libmaix_err_t err = LIBMAIX_ERR_NONE;
struct libmaix_cam* cam     = NULL;
libmaix_image_t* img        = NULL;
libmaix_image_t* resize_img = NULL;

struct quirc *qr;
uint8_t * qr_buf;

void stop_capture();

void my_handler(int s){
   printf("Caught signal %d\n",s);
   stop_capture();
   exit(1);
}

void stop_capture(){

	if(qrcode_fd){
		logi("close qrcode fd");
		unlink(qrcode_fd);
	}

	if(qr){
		logi("qr destory");
		quirc_destroy(qr);
	}

	if(resize_img){
		logi("image destory");
		libmaix_image_destroy(&resize_img);
	}

	if(cam){
		logi("cam destory");
		libmaix_cam_destroy(&cam);
	}
//    if(rgb_img)
//    {
//        logv("--image destory");
//        libmaix_image_destroy(&rgb_img);
//    }
	if(img){
		logi("image destory");
		libmaix_image_destroy(&img);
	}

	logi("image module deinit");
	libmaix_image_module_deinit();
}

void capture_init(){
	// Creating the named file(FIFO)
	logi("Create fifo " QRCODE_WRITE_FIFO);
	mkfifo(QRCODE_WRITE_FIFO, 0666);

	logi("image module init");
	libmaix_image_module_init();

	logi("image_create...");
	img = libmaix_image_create(res_w, res_h, LIBMAIX_IMAGE_MODE_YUV420SP_NV21, LIBMAIX_IMAGE_LAYOUT_HWC, NULL, true);
	if(!img){
		loge("create yuv image fail");
		stop_capture();
	}
//    libmaix_image_t* rgb_img = libmaix_image_create(res_w, res_h, LIBMAIX_IMAGE_MODE_RGB888, LIBMAIX_IMAGE_LAYOUT_HWC, NULL, true);
//    if(!rgb_img)
//    {
//        loge("create rgb image fail");
//        goto end;
//    }

	logi("cam_create...");
	cam = libmaix_cam_creat(res_w, res_h);
	if(!cam){
		loge("create cam fail");
		stop_capture();
	}

	logi("quirc_new...");
	qr = quirc_new();
	if (!qr) {
		loge("couldn't allocate QR decoder");
		stop_capture();
	}

	if (quirc_resize(qr, res_w, res_h) < 0) {
		loge("couldn't allocate QR buffer");
		quirc_destroy(qr);
		stop_capture();
	}
}

void capture_loop(){

    logi("start capture");
    err = cam->strat_capture(cam);
    if(err != LIBMAIX_ERR_NONE){
        loge("start capture fail: %s", libmaix_get_err_msg(err));
        stop_capture();
    }

    while(1)
    {
        __LOG_TIME_START();

        img->mode = LIBMAIX_IMAGE_MODE_YUV420SP_NV21;
        err = cam->capture(cam, (unsigned char*)img->data);

        // not ready， sleep to release CPU
        if(err == LIBMAIX_ERR_NOT_READY){
            usleep(20 * 1000); // 20ms
            continue;
        }

        if(err != LIBMAIX_ERR_NONE){
            loge("capture fail: %s", libmaix_get_err_msg(err));
            break;
        }

        __LOG_TIME_END("capture");

        __LOG_TIME_START();

//        char bmp_data_path[50];
//        sprintf(bmp_data_path, "%s/img-%d.bmp", out_dir, index);
//        logw(" >>>>> Saving file: %s", bmp_data_path);
//        YUVToBMP(bmp_data_path, img->data, NV12ToRGB24, img->width, img->height);

        // Quirc begin
        qr_buf = quirc_begin(qr, NULL, NULL);

        /* copy Y */
//        for(int i = 0; i < img->width * img->height; i++){
//        	qr_buf[i] = ((char *)img->data)[2*i];
//        }
//
        //copy Y
        memcpy(qr_buf,img->data, img->width * img->height);

//         //  --conver YUV to RGB
//         libmaix_err_t err0 = img->convert(img, LIBMAIX_IMAGE_MODE_RGB888, &qr_buf);
//         if(err0 != LIBMAIX_ERR_NONE)
//         {
//             printf("conver to RGB888 fail:%s\r\n", libmaix_get_err_msg(err0));
//             continue;
//         }
//         printf("--convert test end\n");

        // Quirc end
		quirc_end(qr);

		int count = quirc_count(qr);
//		logi("Decoding qrcode , count: %d \n", count);

		if(count <= 0 && LED_CHANGE) ioctl(periph_fd, _IO(0x70, 0x1b), 0); // OFF

		if(count >= 1 && enable_sound) beepSound();

		for (int i = 0; i < count; i++) {
			struct quirc_code code;
			struct quirc_data data;

			quirc_extract(qr, i, &code);
			err = quirc_decode(&code, &data);
			if (err){
				printf("Decode failed: %s\n", quirc_strerror(err));

				// Save snapshot of current failure
				#if(SNAPSHOT_ON_ERROR)
				qr_error_count++;
				if(qr_save_onerror){
					char bmp_data_path[50];
					sprintf(bmp_data_path, "%s/img-%d.bmp", out_dir, qr_error_count);
					logw(" >>>>> Saving file: %s", bmp_data_path);
					YUVToBMP(bmp_data_path, img->data, NV12ToRGB24, img->width, img->height);
//					sleep(1);
				}
				#endif

			}else{

				fprintf(stderr, " - Data: %s\n", data.payload);

//				qrcode_fd = open(QRCODE_WRITE_FIFO, O_WRONLY);
//				write(qrcode_fd, data.payload, strlen(data.payload)+1);
//				close(qrcode_fd);

				if(LED_CHANGE) ioctl(periph_fd, _IO(0x70, 0x1c), 0); // ON
			}

		}

        __LOG_TIME_END("QR Decoding time");
    }
    
    stop_capture();

}

// Before capture we need restart sensor..
void replug_sensor(){
    logi("restart sensor ");
    system("rmmod " SENSOR_MODULE);
    system("insmod " SENSOR_MODULE " " SENSOR_PARAMS);
}


int parse_args(int argc, char* argv[]){

//    if(argc == 1) {
//        loge("Usage: ./camerademo -d /tmp/sd/record -w 640 -h 480");
//        return -1;
//    }

    for (;;) {
        int opt = getopt(argc, argv, ":d:w:h:s");
        if (opt == -1)
            break;
        switch (opt) {
        case '?':
            fprintf(stderr, "%s: Unexpected option: %c\n", argv[0], optopt);
            return -1;
        case ':':
            fprintf(stderr, "%s: Missing value for: %c\n", argv[0], optopt);
            return -1;
        case 'd':
            fprintf(stdout, "Using directory: %s\n", optarg);
            out_dir = optarg;
            break;
        case 's':
            enable_sound = 1;
            break;
        case 'w':
            res_w = atoi(optarg);
            break;
        case 'h':
            res_h = atoi(optarg);
            break;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {

	if (parse_args(argc, argv) < 0) return -1;

	signal(SIGINT, my_handler);

	// this is needed to power sensor
	logi("Init /dev/cpld_periph ");
	periph_fd = open("/dev/cpld_periph", O_RDWR);
	ioctl(periph_fd, _IOC(0, 0x70, 0x15, 0x00), 0);
	ioctl(periph_fd, _IOC(0, 0x70, 0x16, 0x00), 0);
	ioctl(periph_fd, _IOC(0, 0x70, 0x13, 0x00), 0xbefa5c3c);

	// restart sensor before each capture
	replug_sensor();
	sleep(1);

	capture_init();

	capture_loop();

	return 0;
}
