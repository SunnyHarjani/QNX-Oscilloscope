#include <math.h>
#include <string.h>
#include <stdio.h>
#include "stm32l476xx.h"
#include "UART.h"
#include "servo.h"
#include "hardware.h"

// = 1: Use a real a2d value from the STM32's GPIOs
// = 0: Use an internally simulated voltage value
#DEFINE READ_GPIO (1)

// Print every GPIO value received; Makes operation erratic; Only use when debugging
#DEFINE PRINT_GPIO (0)

// Print every voltage value received; Makes operation erratic; Only use when debugging
#DEFINE PRINT_VOLTAGE (0)

// Register that controls PWM duty cycle
#define SERVO1_REGISTER (TIM2->CCR2)

// Simulated +/- voltage range
#define VOLTAGE_RANGE (5)

// Direction = 1 when incrementing simulated voltage
#define INCREMENT_SIMULATED_VOLTAGE (1)
// Direction = -1 when decrementing simulated voltage
#define DECREMENT_SIMULATED_VOLTAGE (-1)

//Convert a binary to a float
float binaryToFloat(short* bin) {
	float f = 0;
	int i = 0;
	for (i = 0; i < 7; i++) {
		f += bin[7-i] * pow(2.0, i-4);
	}
	if (bin[0]) { f *= -1; }
	return f;
}

//Use a servo as a voltmeter
int main(void){
  hardwareInit();
	//Buffer used for console writing
	int consoleOutputBufferSize;
	char consoleOutputBuffer[BufferSize];
	
	consoleOutputBufferSize = sprintf(consoleOutputBuffer, "\r\n**********************\r\n* Initializing *\r\n**********************\r\n");
	USART_Write(USART2, (uint8_t *)consoleOutputBuffer, consoleOutputBufferSize);
	
	float voltage = 0; // Last read/simulated voltage
	
	// Variables used to for PWM simulation
	int direction = INCREMENT_SIMULATED_VOLTAGE; 
	float delta = 0.0625; // Stepping size for simulated voltage
	
	//Variables used for real PWM operation
  short gpio[8] = {0}; // Input GPIO buffer 
	
	while (1) {
#IF READ_GPIO // Real PWM operation
      readGPIO(gpio);
      voltage = binaryToFloat(gpio);
#ELSE // Simulated PWM operation
      voltage += direction * delta;
      if (direction == INCREMENT_SIMULATED_VOLTAGE && voltage >= VOLTAGE_RANGE) { direction = DECREMENT_SIMULATED_VOLTAGE; }
      else if (direction == DECREMENT_SIMULATED_VOLTAGE && voltage <= -VOLTAGE_RANGE) {direction = INCREMENT_SIMULATED_VOLTAGE;}
      USART_Delay(100); //200ms
#ENDIF
    
#IF PRINT_GPIO 
      consoleOutputBufferSize = sprintf(consoleOutputBuffer, "\r\n %d | %d |%d | %d |%d | %d | %d | %d \r\n", gpio[0],gpio[1] ,gpio[2] ,gpio[3] ,gpio[4] ,gpio[5] ,gpio[6] ,gpio[7]);
      USART_Write(USART2, (uint8_t *)consoleOutputBuffer, consoleOutputBufferSize);
#ENDIF

#IF PRINT_VOLTAGE 
      consoleOutputBufferSize = sprintf(consoleOutputBuffer, "CCR: %d | Voltage:%f\r\n", SERVO1_REGISTER, voltage);
      USART_Write(USART2, (uint8_t *)consoleOutputBuffer, consoleOutputBufferSize);
#ENDIF
    
		moveMotor(SERVO1_REGISTER, voltage);
	}
	consoleOutputBufferSize = sprintf(consoleOutputBuffer, "Program Terminated\r\n");
	USART_Write(USART2, (uint8_t *)consoleOutputBuffer, consoleOutputBufferSize);
	
	return 1;
}

