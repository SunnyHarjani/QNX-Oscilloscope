#include <stdio.h>
#include <unistd.h>       /* for sleep() */
#include <stdint.h>       /* for uintptr_t */
#include <hw/inout.h>     /* for in*() and out*() functions */
#include <sys/neutrino.h> /* for ThreadCtl() */
#include <sys/mman.h>     /* for mmap_device_io() */
#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <ctime>
#include <queue>
#include <string>
#include <sys/mman.h>
#include <sys/time.h>

#define BASE 0x280
#define CONTROL_REGISTER BASE+11
#define INIT_BIT 0x00
#define PORT_LENGTH 1
#define PORT_A BASE+8

#define TO_NANO_FACTOR (1000000000)
#define TO_MICRO_FACTOR (1000000)
#define LOW 0x00
#define HIGH 0xFF

bool checkstatus() { // returns 0 if ok, 1 if error
	for (int i = 0; i < 20000; i++) {
		if (!(in8(BASE+3) & 0x80)) {
			return(false); // conversion completed
		}
	}
	return(true); // conversion did not complete
}

void QNXInit(){
	int privity_err;
	uintptr_t ctrl_handle;
	uintptr_t data_handle;

	struct _clockperiod NewClkPeriod, OldClkPeriod;
	NewClkPeriod.nsec = 100000;
	NewClkPeriod.fract = 0;
	ClockPeriod(CLOCK_REALTIME, &NewClkPeriod, &OldClkPeriod, 0);

	/* Give this thread root permissions to access the hardware */
	privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
		return;
	}

	/* Get a handle to the parallel port's Control register */
	ctrl_handle = mmap_device_io( PORT_LENGTH, CONTROL_REGISTER );

	/* Initialise the parallel port */
	out8( ctrl_handle, INIT_BIT );

	out8(BASE+2, 0x22);// example, set channel range to fixed channel 2

	out8(BASE+3, 0x01); // example: set gain = 2

	out8(BASE+1, 2); // set page
	out8(BASE+13, 0x00); // example: sets bipolar, single-ended
}

struct timespec doubleToTimespec(double d) {
	struct timespec ret;
	ret.tv_sec = (int) d;
	ret.tv_nsec = ((d - ret.tv_sec) * TO_NANO_FACTOR);
	return ret;
}

struct timeval doubleToTimeval(double d) {
	struct timeval ret;
	ret.tv_sec = (int)d;
	ret.tv_usec = ((d - ret.tv_sec) * TO_MICRO_FACTOR);
	return ret;
}

double timespecToDouble(const struct timespec &t) {
	return (double)t.tv_sec + ((double)t.tv_nsec / TO_NANO_FACTOR);
}

double timevalToDouble(const struct timeval &t) {
	double ret;
	ret = t.tv_sec;
	ret += (t.tv_usec / TO_MICRO_FACTOR);
	return ret;
}

struct timespec subtract(const struct timespec& t1, const struct timespec& t2) {
	double diff = timespecToDouble(t1) - timespecToDouble(t2);
	return doubleToTimespec(diff);
}

struct timeval subtract(const struct timeval& t1, const struct timeval& t2) {
	double diff = timevalToDouble(t1) - timevalToDouble(t2);
	return doubleToTimeval(diff);
}

struct tuple {
	uintptr_t pinOutput;
	struct timespec highTime;
	struct timespec lowTime;
};

void* AnalogToDigitalRead(void* arg){
	struct tuple* t = (struct tuple*) arg;
	struct timespec highTime, lowTime;
	double ADdata;
	while (1) {
		while (in8(BASE+3) & 0x20); // wait for ADWAIT to go low, base+3 bit 5

		out8(BASE,0x80); // start the A/D conversion
		while(checkstatus()){};

		//ADdata = ((in8(BASE) << 8) + in8(BASE+1)) / 6553.6; // combine the 2 bytes into a 16-bit value
		//highTime = doubleToTimespec(0.00065 + (0.0002 * ADdata));
		//lowTime = subtract(doubleToTimespec(.02), highTime);

		int8_t LSB = in8(BASE);
		int8_t MSB = in8(BASE+1);
		//ADdata = MSB * 256 + LSB; // combine the 2 bytes into a 16-bit value
		//float InputVoltage = ADdata / 32768.0 * 5.0;

		//ADdata = ((MSB << 8) + LSB) / 6553.6;
		ADdata = 0;
		//highTime = doubleToTimespec(0.00065 + (0.0002 * ADdata));
		//lowTime = subtract(doubleToTimespec(.02), highTime);
		highTime = doubleToTimespec(1.0);
		lowTime = doubleToTimespec(2.0);

		t->highTime = highTime;
		t->lowTime = lowTime;
	}
	//std::cout << InputVoltage << std::endl;
}

void* servoTime(void* arg) {
	struct tuple* t = (struct tuple*) arg;
	struct timeval start, stop;
	while(1){
		/* Output a byte of lows to the data lines */

		out8( t->pinOutput, LOW );
		gettimeofday(&start, NULL);
		nanosleep(&(t->lowTime), NULL);
		gettimeofday(&stop, NULL);
		std::cout << timevalToDouble(subtract(stop, start)) << std::endl;

		/* Output a byte of highs to the data lines */
		out8( t->pinOutput, HIGH );
		gettimeofday(&start, NULL);
		nanosleep(&(t->highTime), NULL);
		gettimeofday(&stop, NULL);
		std::cout << timevalToDouble(subtract(stop, start)) << std::endl;
	}
}

/* ______________________________________________________________________ */
int main( )
{
	QNXInit();

	// wait for ADBUSY to go low, base+3 bit 7
	struct tuple t;
	t.pinOutput = PORT_A;
	t.highTime = doubleToTimespec(0.00025);
	t.lowTime = subtract(doubleToTimespec(.02), t.highTime);
	pthread_t servoPWM, a2d;
	//pthread_setschedprio(servoPWM, 255);
	pthread_create(&servoPWM, NULL, servoTime, &t);
	pthread_create(&a2d, NULL, AnalogToDigitalRead, &t);
	while(1){}
	return EXIT_SUCCESS;
}
