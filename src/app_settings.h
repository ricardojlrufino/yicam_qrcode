#ifndef SRC_APP_SETTINGS_H_
#define SRC_APP_SETTINGS_H_

#define LED_CHANGE 1 // Change led on detect QRCODE

#define SNAPSHOT_ON_ERROR 0
#define SNAPSHOT_DEFAULT_DIR "/tmp/sd/record"

#define SOUND_PATH "/tmp/sd/my/job-done-501-16khz.wav"

// TODO: need improvements in wav to C converter
#define SOUND_AS_HEADER 1 // ignore sound .wav and use beep.h ( pre converted wav )

#define QRCODE_WRITE_FIFO "/tmp/qrcode_read"

// Used in replug_sensor();
#define SENSOR_MODULE "/home/qigan/ko/sensor_power.ko"
#define SENSOR_PARAMS "dvdd0_vol=\"1800000\" mclk0=\"27000000\""

#endif /* SRC_APP_SETTINGS_H_ */
