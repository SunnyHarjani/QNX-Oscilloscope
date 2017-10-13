#include "UART.h"

#include <stdlib.h>

#define REGISTER_POSITION_DELTA (58)

// Move the servo's motor
void moveMotor(volatile uint32_t* dutyCycleRegister, float f_voltage);
