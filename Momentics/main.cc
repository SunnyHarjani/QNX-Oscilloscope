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
#include <math.h>
#include <sys/mman.h>
#include <sys/time.h>

#define BASE 0x280
#define CONTROL_REGISTER BASE+11
#define INIT_BIT 0x00
#define PORT_LENGTH 1
#define PORT_A BASE+8

#define A2D_TO_FLOAT_FACTOR (6553.6)
#define TO_NANO_FACTOR (1000000000)
#define TO_MICRO_FACTOR (1000000)
#define LOW 0x00
#define HIGH 0xFF

//Global binary array used to represent a floating point number to output
// out[7] = signed bit
// out[6 downto 4] = integer bit
// out[3 downto 0] = decimal bit
bool bin[8] = {0};

// Ensures proper a2d functionality
bool checkstatus() { // returns 0 if ok, 1 if error
	for (int i = 0; i < 20000; i++) {
		if (!(in8(BASE+3) & 0x80)) {
			return(false); // conversion completed
		}
	}
	return(true); // conversion did not complete
}

// Initialize QNIX hardware to enable 8 bit outputs and the A2D
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


//Single struct used to communicate between threads
struct tuple {
	uintptr_t pinOutput;
	uint voltageValue;
};

// Convert a float to an 8 bit binary array
void floatToBinary(float f, bool* out) {
	//static bool out[8] = {0};
	out[7] = (f < 0);
	if (out[7]) { f *= -1; }
	int whole = (int)f;
	int decimal = 625 * ((int)((f - whole)/0.0625));
	out[6] = (whole >> 2) & 1;
	out[5] = (whole >> 1) & 1;
	out[4] = (whole >> 0) & 1;
	out[3] = decimal / 5000;
	if (out[3]) { decimal -= 5000; }
	out[2] = decimal / 2500;
	if (out[2]) { decimal -= 2500; }
	out[1] = decimal / 1250;
	if (out[1]) { decimal -= 1250; }
	out[0] = decimal / 625;
}

// Read a2d input
void* AnalogToDigitalRead(void* arg){
	struct tuple* t = (struct tuple*) arg;
	struct timespec highTime, lowTime;
	double ADdata;
	while (1) {
		while (in8(BASE+3) & 0x20); // wait for ADWAIT to go low, base+3 bit 5

		out8(BASE,0x80); // start the A/D conversion
		while(checkstatus()){};
		volatile int8_t LSB = in8(BASE);
		volatile int8_t MSB = in8(BASE+1);

		ADdata = ((MSB << 8) + LSB) / A2D_TO_FLOAT_FACTOR;
		floatToBinary(ADdata, bin);

	}
	return 0;
}


// Output the contents of bin[] to GPIO
void* GPIOOut(void* arg) {
	struct tuple* t = (struct tuple*) arg;
	short binaryOut;
	while(1){
		/* Output a byte of lows to the data lines */
		int i = 0;
		for (i = 0; i < 8; i++) {
			binaryOut = (binaryOut << 1) + bin[i];
		}
		out8( t->pinOutput, binaryOut );

	}
	return 0;
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
	pthread_setschedprio(servoPWM, 255);
	pthread_create(&servoPWM, NULL, GPIOOut, &t);
	pthread_create(&a2d, NULL, AnalogToDigitalRead, &t);
	while(1){}
	return EXIT_SUCCESS;
}
