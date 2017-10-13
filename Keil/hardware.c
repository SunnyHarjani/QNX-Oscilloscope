#include "hardware.h"

//Prepare the STM32 board for hardware usage
void hardwareInit(void) {
  System_Clock_Init();
  UART2_Init();
	GPIOA_EInit();
  GPIOBInit();
  timer2Init();
  timer5Init();
}

//Read 8 GPIO pins and store them into the given array
void readGPIO(short bin[]){
	bin[0] = (GPIOA->IDR >> 0) & 1;
	bin[1] = (GPIOA->IDR >> 5) & 1;
	bin[2] = (GPIOA->IDR >> 1) & 1;
	bin[3] = (GPIOA->IDR >> 2) & 1;
	bin[4] = (GPIOE->IDR >> 10) & 1;
	bin[5] = (GPIOE->IDR >> 12) & 1;
	bin[6] = (GPIOE->IDR >> 13) & 1;
	bin[7] = (GPIOE->IDR >> 14) & 1;
}

//Set up timer 2 to control the PWM signal
void timer2Init(void){
  RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

  //Output/comparemode for channel 2 (OC2M)
  TIM2->CCMR1 &= ~(0x07<<12);
  TIM2->CCMR1 |= (0x03<<13);
  
  //Outputcompare 2 preload enable (OC2PE)
  TIM2->CCMR1 |= (1<<11);
  
  //Output/comparemode for channel 2 (OC3M)
  TIM2->CCMR2 &= ~(0x07<<4);
  TIM2->CCMR2 |= (0x03<<5);
  
  //Outputcompare 2 preload enable (OC3PE)
  TIM2->CCMR2 |= (1<<3);
  
  TIM2->CR1 |= (1<<7);
  TIM2->CCER |= (1<<4); //Capture/Compare channel 2
  TIM2->CCER |= (1<<8); //Capture/Compare channel 3
  
  TIM2->PSC = 25;
  TIM2->ARR = 61700;
  
  TIM2->CCR2 = 4100;//7000;//1200;//PB3
  
  TIM2->EGR |= 0x01;
  TIM2->CR1 |= 0x01;
}

// Configure PORT A & E to be in input mode with pull down resistors
void GPIOA_EInit(void) {
	// enable clock for A group of GPIO
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOEEN);	
  
	// set ports A & E with pull down resistors
  GPIOA->PUPDR = 0xAAAAAAAA;
  GPIOE->PUPDR = 0xAAAAAAAA;
	
	// set ports A & E as inputs
	GPIOA->MODER &= ~(GPIO_MODER_MODER0 |  GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER5);
  GPIOE->MODER &= ~(GPIO_MODER_MODER14 | GPIO_MODER_MODER10 |  GPIO_MODER_MODER12 |  GPIO_MODER_MODER13);
  
}

// Configure PORT B to control the servo PWM
void GPIOBInit(void) {
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
  //Set PB3 as AF1 mode
  GPIOB->MODER &= ~(0x03<<(2*3));
  GPIOB->MODER |= (1<<7); 
  
  GPIOB->AFR[0] &= ~(0x0F<<(4*3));
  GPIOB->AFR[0] |= (1<<12);
  
}

// Move the servo's motor
void moveMotor(volatile uint32_t* dutyCycleRegister, float voltage) {
	dutyCycleRegister = ((int)(REGISTER_POSITION_DELTA * (voltage * 10))) + REGISTER_POSITION_OFFSET;
}
