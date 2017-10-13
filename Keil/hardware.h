#include "stm32l476xx.h"
#include "LED.h"
#include "UART.h"
#include "SysClock.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define REGISTER_POSITION_OFFSET (4100)
#define REGISTER_POSITION_DELTA (58)

//Prepare the STM32 board for hardware usage
void hardwareInit(void);

//Read 8 GPIO pins and store them into the given array
void readGPIO(short bin[]);

//Set up timer 2 to control the PWM signal
void timer2Init(void);

// Configure PORT A to be in input mode with pull down resistors
void GPIOA_EInit(void);

// Configure PORT B to be in input mode with pull down resistors
void GPIOBInit(void);

// Move the servo's motor
void moveMotor(volatile uint32_t* dutyCycleRegister, float voltage);