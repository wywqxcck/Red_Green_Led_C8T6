#ifndef __APP_H_
#define __APP_H_

#include <rtthread.h>
#include <rtdevice.h>
#include "string.h"
#include "Nixie.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_def.h"

#define THREAD_STACK_SIZE   1024
#define THREAD_PRIORITY     20
#define THREAD_TIMESLICE    10

#define N_S_G 'G'
#define N_S_R 'R'
#define USART_MAX 10//串口最大接收
#define RED_GREEN_LED_MAX 1
#define SER 4//PA4
#define SCL 6//PA5
#define RCL 5//PA6
#define S_N_YELLOW_LED 28
#define S_N_RED_LED 29
#define S_N_GREEN_LED 30
#define E_W_YELLOW_LED 19
#define E_W_RED_LED 20
#define E_W_GREEN_LED 21
#define REDAR 18//雷达中断引脚


typedef struct Time{
    uint8_t N_S_Red_time;//东西红灯
    uint8_t N_S_Green_time;//南北绿灯
}Time;

typedef struct rx_msg{
    rt_device_t dev;
    rt_size_t size;
}rx_msg;

void RTT_OS_Iint(void);

#endif
