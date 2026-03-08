#ifndef __LIGHT_SENSOR_H
#define __LIGHT_SENSOR_H

#define Light_sensor_DO_Pin      GPIO_Pin_8
#define Light_sensor_DO_GPIO	 GPIOA
#define Light_sensor_DO_RCC		 RCC_APB2Periph_GPIOA

void LightSensor_Init(void);
uint8_t LightSensor_Get(void);

#endif
