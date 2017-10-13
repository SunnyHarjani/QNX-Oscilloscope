#include "servo.h"

// Initialize this servo
void initServo(struct Servo* s, volatile uint32_t* registerAddress, unsigned short int registerPositionInit) {
	s->dutyCycleRegister = registerAddress;
	s->registerPositionOffset = registerPositionInit;
	s->state = running;
}


