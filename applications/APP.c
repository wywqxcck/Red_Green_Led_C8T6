#include "APP.h"

static rt_timer_t Nixie_Timer_thred = RT_NULL;//数码管扫描定时器句柄
static rt_timer_t Red_Green_Led_Timer_thred = RT_NULL;//红绿灯计数定时器句柄
static rt_thread_t usar_thread;//串口处理线程句柄
static rt_thread_t Red_Green_Led_thread;//红绿灯处理线程句柄
Time Red_Green_Led = {6,6};//红绿灯时间

static rt_device_t serial;//串口句柄
#define SAMPLE_UART_NAME       "uart2"      /* 串口设备名称 */
/* 串口接收消息结构 */
static rt_mq_t usar_mq;//串口邮箱
static rt_mq_t red_green_led_mq;//红路灯邮箱

/*串口接收中断回调函数*/
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    static char rx_buffer[USART_MAX  + 2];
    struct rx_msg msg;
    static rt_uint8_t rx_length = 1;
    msg.dev = dev;
    msg.size = size;
    rt_device_read(msg.dev, 0, &rx_buffer[rx_length++],1);
    if(rx_length > USART_MAX){
        rt_kprintf("usart tx err\n");
        rx_length = 1;
    }
    if((*(rx_buffer + rx_length - 1)) == '#')
    {
        rx_buffer[rx_length] = '\0';
        rt_device_write(serial, 0, rx_buffer, rx_length);
        rt_device_write(serial, 0, "\r\n", 2);
        rx_buffer[0] = rx_length - 2;
        rx_length = 1;
        rt_mq_send(usar_mq, &rx_buffer, USART_MAX);
    }
    return 0;
}


uint8_t Invert(uint8_t Data){
    if(Data >= '0' && Data <= '9')
        return Data - 48;
    return 0xFF;
}


/*串口接收处理线程*/
static void usar_thread_entry(void *parameter)
{
    char rx_buffer[USART_MAX  + 2];
    while(1)
    {
        rt_mq_recv(usar_mq, &rx_buffer,USART_MAX, RT_WAITING_FOREVER);
        rt_kprintf("RX_Data:%s\nRX_Len:%d\n",rx_buffer + 1,rx_buffer[0]);
        for(rt_uint8_t i = 1;i < rx_buffer[0];i++){
            if(rx_buffer[i] == N_S_G){
                Red_Green_Led.N_S_Green_time = Invert(rx_buffer[i + 1]) * 10 + Invert(rx_buffer[i + 2]) + 1;
            }
            if(rx_buffer[i] == N_S_R){
                Red_Green_Led.N_S_Red_time = Invert(rx_buffer[i + 1]) * 10 + Invert(rx_buffer[i + 2]) + 1;
            }
        }
    }
}

/*红绿灯处理线程*/
static void Red_Green_Led_thread_entry(void *parameter)
{
    static rt_uint8_t green_red_time = 0;
    while(1)
    {
        rt_mq_recv(red_green_led_mq, &green_red_time,USART_MAX, 200);
        Nixie_Num[0] = green_red_time / 10;
        Nixie_Num[1] = green_red_time % 10;
        Nixie_Num[2] = Nixie_Num[0];
        Nixie_Num[3] = Nixie_Num[1];
        Nixie_Num[4] = Nixie_Num[0];
        Nixie_Num[5] = Nixie_Num[1];
        Nixie_Num[6] = Nixie_Num[0];
        Nixie_Num[7] = Nixie_Num[1];
    }
}


/*将字符转化为数字*/
void Reder_IAR(void *args)
{
    rt_device_write(serial, 0, "1 -> 0\r\n", 8);
}
/*数码管扫描定时器回调函数*/
static void Nixie_Timer_ISR(void* parameter)
{
    static rt_uint8_t i = 0;
    Nixie_Show(Nixie_Num[i], i);
    i++;
    i %= 8;
}

uint8_t Count_time = 0,i = 0,j = 0;
/*红绿灯控制定时器回调函数*/
static void Red_Green_Led_Timer_ISR(void* parameter)
{
    static rt_uint8_t green_red_time = 0;
    static Time temp = {6,6};
    if(temp.N_S_Green_time != 0){
        temp.N_S_Green_time--;
        green_red_time = temp.N_S_Green_time;
    }
    else{
        temp.N_S_Red_time--;
        green_red_time = temp.N_S_Red_time;
    }

    /*南北方向灯状态*/
    if(temp.N_S_Green_time <= 3 && temp.N_S_Green_time != 0){//黄灯
        rt_pin_write(S_N_YELLOW_LED,PIN_LOW);
        rt_pin_write(S_N_GREEN_LED,PIN_HIGH);
        rt_pin_write(S_N_RED_LED,PIN_HIGH);
    }
    else if(temp.N_S_Green_time >= 3){//绿灯
        rt_pin_write(S_N_YELLOW_LED,PIN_HIGH);
        rt_pin_write(S_N_GREEN_LED,PIN_LOW);
        rt_pin_write(S_N_RED_LED,PIN_HIGH);
    }
    else{//红灯
        rt_pin_write(S_N_YELLOW_LED,PIN_HIGH);
        rt_pin_write(S_N_GREEN_LED,PIN_HIGH);
        rt_pin_write(S_N_RED_LED,PIN_LOW);
    }

    /*东西方向灯状态*/
    if(temp.N_S_Green_time != 0){//红灯
        rt_pin_write(E_W_RED_LED,PIN_LOW);
        rt_pin_write(E_W_YELLOW_LED,PIN_HIGH);
        rt_pin_write(E_W_GREEN_LED,PIN_HIGH);
    }
    else if(temp.N_S_Red_time > 3){//绿灯
        rt_pin_write(E_W_RED_LED,PIN_HIGH);
        rt_pin_write(E_W_YELLOW_LED,PIN_HIGH);
        rt_pin_write(E_W_GREEN_LED,PIN_LOW);
    }
    else{//黄灯
        rt_pin_write(E_W_RED_LED,PIN_HIGH);
        rt_pin_write(E_W_YELLOW_LED,PIN_LOW);
        rt_pin_write(E_W_GREEN_LED,PIN_HIGH);
    }


    /*某一方向结束*/
    if(temp.N_S_Green_time == 0 && temp.N_S_Red_time == Red_Green_Led.N_S_Red_time){
        rt_device_write(serial, 0, "#", 1);
    }
    if(temp.N_S_Red_time == 0){
           rt_device_write(serial, 0, "*", 1);
    }

    /*红绿灯一轮结束*/
    if(temp.N_S_Green_time == 0 && temp.N_S_Red_time == 0){
        temp = Red_Green_Led;
    }
    rt_mq_send(red_green_led_mq, &green_red_time, RED_GREEN_LED_MAX);
}


/*系统初始化*/
void RTT_OS_Iint(void)
{
    char uart_name[RT_NAME_MAX];
    /*初始化串口处理线程*/
    usar_thread = rt_thread_create( "usar",
                usar_thread_entry,
                RT_NULL,
                THREAD_STACK_SIZE,
                THREAD_PRIORITY,
                THREAD_TIMESLICE);
    if (usar_thread != RT_NULL)
        rt_thread_startup(usar_thread);

    /*初始化红绿灯线程*/
    Red_Green_Led_thread = rt_thread_create( "Red_Green_Led",
                Red_Green_Led_thread_entry,
                RT_NULL,
                THREAD_STACK_SIZE,
                THREAD_PRIORITY,
                THREAD_TIMESLICE);
    if (Red_Green_Led_thread != RT_NULL)
        rt_thread_startup(Red_Green_Led_thread);

    /*初始化串口*/
    rt_strncpy(uart_name, SAMPLE_UART_NAME, RT_NAME_MAX);
    serial = rt_device_find(uart_name);
    if (!serial)
    {
        rt_kprintf("find %s failed!\n", uart_name);
    }
    else {
        rt_kprintf("uart ok\n");
    }
    rt_device_open(serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    rt_device_set_rx_indicate(serial, uart_input);/* 设置接收回调函数 */
    rt_device_write(serial, 0, "123\r\n", 5);
    /*初始化串口邮箱*/
    usar_mq = rt_mq_create("uart_mq",USART_MAX,1,RT_IPC_FLAG_FIFO);
    /*初始化红绿灯邮箱*/
    red_green_led_mq = rt_mq_create("red_green_led_mq",RED_GREEN_LED_MAX,1,RT_IPC_FLAG_FIFO);
    /*数码管定时器*/
    Nixie_Timer_thred = rt_timer_create("Nixie_Timer",
                    Nixie_Timer_ISR,
                    RT_NULL,
                    1,RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    if(Nixie_Timer_thred != RT_NULL)
        rt_timer_start(Nixie_Timer_thred);
    rt_kprintf("Nixie Timer OK\n");

    /*红绿灯定时器*/
    Red_Green_Led_Timer_thred = rt_timer_create("Red_Green_Led_Timer",
                    Red_Green_Led_Timer_ISR,
                    RT_NULL,
                    1000,RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    if(Red_Green_Led_Timer_thred != RT_NULL)
        rt_timer_start(Red_Green_Led_Timer_thred);
    rt_kprintf("Red_Green_Led_Timer_thred Timer OK\n");


    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    /*初始化数码管端口*/
    rt_pin_mode(SER, PIN_MODE_OUTPUT);
    rt_pin_mode(RCL, PIN_MODE_OUTPUT);
    rt_pin_mode(SCL, PIN_MODE_OUTPUT);

    /*灯初始化*/
    rt_pin_mode(S_N_YELLOW_LED, PIN_MODE_OUTPUT);
    rt_pin_mode(S_N_RED_LED, PIN_MODE_OUTPUT);
    rt_pin_mode(S_N_GREEN_LED, PIN_MODE_OUTPUT);
    rt_pin_mode(E_W_YELLOW_LED, PIN_MODE_OUTPUT);
    rt_pin_mode(E_W_RED_LED, PIN_MODE_OUTPUT);
    rt_pin_mode(E_W_GREEN_LED, PIN_MODE_OUTPUT);

    /* 按键1引脚为输入模式 */
    rt_pin_mode(REDAR, PIN_MODE_INPUT_PULLDOWN);
    /* 绑定中断，下降沿模式，回调函数名为Reder_IAR */
    rt_pin_attach_irq(REDAR, PIN_IRQ_MODE_FALLING, Reder_IAR, RT_NULL);
    /* 使能中断 */
    rt_pin_irq_enable(REDAR, PIN_IRQ_ENABLE);

    /*灯默认关闭*/
    rt_pin_write(E_W_YELLOW_LED,PIN_HIGH);
    rt_pin_write(E_W_GREEN_LED,PIN_HIGH);
    rt_pin_write(E_W_RED_LED,PIN_HIGH);
    rt_pin_write(S_N_YELLOW_LED,PIN_HIGH);
    rt_pin_write(S_N_GREEN_LED,PIN_HIGH);
    rt_pin_write(S_N_RED_LED,PIN_HIGH);


}

MSH_CMD_EXPORT(RTT_OS_Iint, uart device dma sample);
