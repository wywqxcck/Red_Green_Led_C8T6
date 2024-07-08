#include "Nixie.h"

rt_uint8_t Nixie_Num[8] = {10,10,10,10,10,10,10,10};
const rt_uint8_t seg1[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff};


void Nixie_Show(rt_uint8_t Data,rt_uint8_t Num)
{
	rt_uint8_t num_temp,temp_data,i;
	temp_data = seg1[Data];
	num_temp = 0x01 << Num;
	for(i = 0;i < 8;i++){
		if(num_temp & 0x80)
			rt_pin_write(SER,PIN_HIGH);
		else
			rt_pin_write(SER,PIN_LOW);
		num_temp <<= 1;
		rt_pin_write(SCL,PIN_LOW);
		rt_pin_write(SCL,PIN_HIGH);
	}
	for(i = 0;i < 8;i++){
		if(temp_data & 0x80)
			rt_pin_write(SER,PIN_HIGH);
		else
			rt_pin_write(SER,PIN_LOW);
		temp_data <<= 1;
		rt_pin_write(SCL,PIN_LOW);
		rt_pin_write(SCL,PIN_HIGH);
	}
	rt_pin_write(RCL,PIN_HIGH);
	rt_pin_write(RCL,PIN_LOW);	
}
