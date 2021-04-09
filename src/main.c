/**
 * 
 **/

#include "stdio.h"
#include <stdint.h>
#include <stdbool.h>


#include "libmaix_cam.h"
#include "libmaix_image.h"
#include "quirc.h"
#include "convert.h"
#include "log.h"
#include <sys/time.h>
#include <unistd.h>

// Used in replug_sensor();
#define SENSOR_MODULE "/home/qigan/ko/sensor_power.ko"
#define SENSOR_PARAMS "dvdd0_vol=\"1800000\" mclk0=\"27000000\""



#define TEST_RESIZE_IMAGE 0
#define LED_CHANGE 1 // Change led on detect QRCODE

// int res_w = 240, res_h = 240;
int res_w = 640, res_h = 480;
char *out_dir = "/tmp/sd/record";
int capture_count = 1;

int qr_error_count = 0;
int qr_save_onerror = 1; // used to debug.

void capture_loop(){
    libmaix_err_t err = LIBMAIX_ERR_NONE;
    struct libmaix_cam* cam     = NULL;
    libmaix_image_t* img        = NULL;
    libmaix_image_t* resize_img = NULL;

    struct quirc *qr;
    uint8_t * qr_buf;

    logi("image module init");
    libmaix_image_module_init();

    logi("create image");
    img = libmaix_image_create(res_w, res_h, LIBMAIX_IMAGE_MODE_YUV420SP_NV21, LIBMAIX_IMAGE_LAYOUT_HWC, NULL, true);
    if(!img)
    {
        loge("create yuv image fail");
        goto end;
    }
//    libmaix_image_t* rgb_img = libmaix_image_create(res_w, res_h, LIBMAIX_IMAGE_MODE_RGB888, LIBMAIX_IMAGE_LAYOUT_HWC, NULL, true);
//    if(!rgb_img)
//    {
//        loge("create rgb image fail");
//        goto end;
//    }
    logi("--create cam");
    cam = libmaix_cam_creat(res_w, res_h);
    if(!cam)
    {
        loge("create cam fail");
        goto end;
    }

    qr = quirc_new();
	if (!qr) {
		loge("couldn't allocate QR decoder");
		goto end;
	}

	if (quirc_resize(qr, res_w, res_h) < 0) {
		loge("couldn't allocate QR buffer");
		quirc_destroy(qr);
		goto end;
	}

    logi("--cam start capture");
    err = cam->strat_capture(cam);
    if(err != LIBMAIX_ERR_NONE)
    {
        loge("start capture fail: %s", libmaix_get_err_msg(err));
        goto end;
    }
#if TEST_RESIZE_IMAGE
    resize_img = libmaix_image_create(224, 224, LIBMAIX_IMAGE_MODE_RGB888, LIBMAIX_IMAGE_LAYOUT_HWC, NULL, true);
    if(!resize_img)
    {
        printf("create image error!\n");
        goto end;
    }
#endif

    int index = 0;
    while(1)
    {
        // printf("--cam capture\n");
        LOG_TIME_START();
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

        LOG_TIME_END("capture");

        index++;

        LOG_TIME_START();

//        char bmp_data_path[50];
//        sprintf(bmp_data_path, "%s/img-%d.bmp", out_dir, index);
//        logw(" >>>>> Saving file: %s", bmp_data_path);
//        YUVToBMP(bmp_data_path, img->data, NV12ToRGB24, img->width, img->height);

        // Quirc begin
        qr_buf = quirc_begin(qr, NULL, NULL);

        /* copy Y */
        for(int i = 0; i < img->width * img->height; i++){
        	qr_buf[i] = ((uint8_t *)img->data)[2*i];
        }

        // img->data[2*i]

//         // FILLL // --conver YUV to RGB
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
		logi("Decoding qrcode , count: %d \n", count);

//		if(count <= 0 && LED_CHANGE) system("/home/yi-hack/bin/ipc_cmd -l OFF");

		for (int i = 0; i < count; i++) {
			struct quirc_code code;
			struct quirc_data data;

			quirc_extract(qr, i, &code);
			err = quirc_decode(&code, &data);
			if (err){
				printf("Decode failed: %s\n", quirc_strerror(err));

				// Save snapshot of current failure
				qr_error_count++;
				if(qr_save_onerror){
					char bmp_data_path[50];
					sprintf(bmp_data_path, "%s/img-%d.bmp", out_dir, qr_error_count);
					logw(" >>>>> Saving file: %s", bmp_data_path);
					YUVToBMP(bmp_data_path, img->data, NV12ToRGB24, img->width, img->height);
					sleep(1);
				}

			}else{

				fprintf(stderr, " - Data: %s\n", data.payload);
//				fprintf(stdout, "\a" ); // BEEP !!!
//				sleep(2);
//				if(LED_CHANGE) system("/home/yi-hack/bin/ipc_cmd -l ON");
			}

		}

        LOG_TIME_END("Save BMP");

#if TEST_RESIZE_IMAGE
        LOG_TIME_START();
        err0 = rgb_img->resize(rgb_img, resize_img->width, resize_img->height, &resize_img);
        if(err0 != LIBMAIX_ERR_NONE)
        {
            printf("resize image error: %s\r\n", libmaix_get_err_msg(err0));
            continue;
        }
        LOG_TIME_END("resize image");
#endif

    }
    
end:

	if(qr_buf)
    {
        logv("--qr destory");
        quirc_destroy(qr);
        free(qr_buf);
    }
    if(resize_img)
    {
        logv("--image destory");
        libmaix_image_destroy(&resize_img);
    }
    if(cam)
    {
        logv("--cam destory");
        libmaix_cam_destroy(&cam);
    }
//    if(rgb_img)
//    {
//        logv("--image destory");
//        libmaix_image_destroy(&rgb_img);
//    }
    if(img)
    {
        logv("--image destory");
        libmaix_image_destroy(&img);
    }
    logv("--image module deinit");
    libmaix_image_module_deinit();
}

// Before capture we need restart sensor..
void replug_sensor(){
    logi("restart sensor ");
    system("rmmod " SENSOR_MODULE);
    system("insmod " SENSOR_MODULE " " SENSOR_PARAMS);
    sleep(1);
}


int parse_args(int argc, char* argv[]){

    if(argc == 1) {
        loge("Usage: ./camerademo -d /tmp/sd/record -w 640 -h 480 -n NUM_IMAGES");
        return -1;
    }

    for (;;) {
        int opt = getopt(argc, argv, ":d:w:h:n:");
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
        case 'w':
            res_w = atoi(optarg);
            break;
        case 'h':
            res_h = atoi(optarg);
            break;
        case 'n':
            capture_count = atoi(optarg);
            break;            
        }
    }

    return 0;
}

int main(int argc, char* argv[])
{

   if(parse_args(argc, argv) < 0) return -1;

    replug_sensor();

    capture_loop();

        // //register process function for SIGINT, to exit program.
    // if (signal(SIGINT, handle_exit) == SIG_ERR)
    //     perror("can't catch SIGSEGV");

    return 0;
}
