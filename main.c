#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../Common/Include/serial.h"
#include "../Common/Include/stm32l051xx.h"
#include "lcd.h"
#include "adc.h"
#define F_CPU 32000000L
#define DEF_F 100000L // 10us tick

volatile int PWM_Counter = 0;
volatile unsigned char pwm3=0, pwm4=100, pwm5=0;
void delay(int dly)
{
	while( dly--);
}

void wait_1ms(void)
{
	// For SysTick info check the STM32L0xxx Cortex-M0 programming manual page 85.
	SysTick->LOAD = (F_CPU/1000L) - 1;  // set reload register, counter rolls over from zero, hence -1
	SysTick->VAL = 0; // load the SysTick counter
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	while((SysTick->CTRL & BIT16)==0); // Bit 16 is the COUNTFLAG.  True when counter rolls over from zero.
	SysTick->CTRL = 0x00; // Disable Systick counter
}

// void waitms(int len)
// {
// 	while(len--) wait_1ms();
// }

#define PIN_PERIOD (GPIOA->IDR&BIT8)

// GetPeriod() seems to work fine for frequencies between 300Hz and 600kHz.
// 'n' is used to measure the time of 'n' periods; this increases accuracy.
long int GetPeriod (int n)
{
	int i;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
	SysTick->LOAD = 0xffffff;  // 24-bit counter set to check for signal present
	SysTick->VAL = 0xffffff; // load the SysTick counter
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(SysTick->CTRL & BIT16) return 0;
	}
	SysTick->CTRL = 0x00; // Disable Systick counter

	SysTick->LOAD = 0xffffff;  // 24-bit counter set to check for signal present
	SysTick->VAL = 0xffffff; // load the SysTick counter
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(SysTick->CTRL & BIT16) return 0;
	}
	SysTick->CTRL = 0x00; // Disable Systick counter
	
	SysTick->LOAD = 0xffffff;  // 24-bit counter reset
	SysTick->VAL = 0xffffff; // load the SysTick counter to initial value
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(SysTick->CTRL & BIT16) return 0;
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(SysTick->CTRL & BIT16) return 0;
		}
	}
	SysTick->CTRL = 0x00; // Disable Systick counter

	return 0xffffff-SysTick->VAL;
}

void TIM2_Handler(void) 
{
	TIM2->SR &= ~BIT0; // clear update interrupt flag
	PWM_Counter++;
	
	if(pwm3>PWM_Counter)
	{
		GPIOB->ODR |= BIT3;
	}
	else
	{
		GPIOB->ODR &= ~BIT3;
	}
	if(pwm4>PWM_Counter)
	{
		GPIOB->ODR |= BIT4;
	}
	else
	{
		GPIOB->ODR &= ~BIT4;
	}
	if(pwm5>PWM_Counter)
	{
		GPIOB->ODR |= BIT5;
	}
	else
	{
		GPIOB->ODR &= ~BIT5;
	}
	
	if (PWM_Counter > 2000) // THe period is 20ms
	{
		PWM_Counter=0;
		GPIOA->ODR |= (BIT11|BIT12);
	}   
}

void Configure_Pins (void)
{
	RCC->IOPENR |= BIT0; // peripheral clock enable for port A
	RCC->IOPENR |= BIT1; // peripheral clock enable for port B
	// Make pins PA0 to PA5 outputs (page 200 of RM0451, two bits used to configure: bit0=1, bit1=0)
    GPIOA->MODER = (GPIOA->MODER & ~(BIT0|BIT1)) | BIT0; // PA0
	GPIOA->OTYPER &= ~BIT0; // Push-pull
    
    GPIOA->MODER = (GPIOA->MODER & ~(BIT2|BIT3)) | BIT2; // PA1
	GPIOA->OTYPER &= ~BIT1; // Push-pull
    
    GPIOA->MODER = (GPIOA->MODER & ~(BIT4|BIT5)) | BIT4; // PA2
	GPIOA->OTYPER &= ~BIT2; // Push-pull
    
    GPIOA->MODER = (GPIOA->MODER & ~(BIT6|BIT7)) | BIT6; // PA3
	GPIOA->OTYPER &= ~BIT3; // Push-pull
    
    GPIOA->MODER = (GPIOA->MODER & ~(BIT8|BIT9)) | BIT8; // PA4
	GPIOA->OTYPER &= ~BIT4; // Push-pull
    
    GPIOA->MODER = (GPIOA->MODER & ~(BIT10|BIT11)) | BIT10; // PA5
	GPIOA->OTYPER &= ~BIT5; // Push-pull
	
	GPIOB->MODER = (GPIOB->MODER & ~(BIT6|BIT7)) | BIT6; // Make pin PB3 output (page 225 of UM, two bits used to configure: bit0=1, bit1=0)
	GPIOB->OTYPER &= ~BIT3; // Push-pull
	
	GPIOB->MODER = (GPIOB->MODER & ~(BIT8|BIT9)) | BIT8; // Make pin PB4 output (page 225 of UM, two bits used to configure: bit0=1, bit1=0)
	GPIOB->OTYPER &= ~BIT4; // Push-pull
	
	GPIOB->MODER = (GPIOB->MODER & ~(BIT10|BIT11)) | BIT10; // Make pin PB5 output (page 225 of UM1, two bits used to configure: bit0=1, bit1=0)
	GPIOB->OTYPER &= ~BIT3; // Push-pull
	
	GPIOB->MODER |= (BIT0|BIT1); // Configure PB0 for input
	/* IMPORTANT! CONFIGURE PA6 AND PA7 FOR ADC INPUT */
	RCC->IOPENR  |= BIT7; // peripheral clock enable for port B
	GPIOB->MODER |= (BIT14|BIT15);
	
	RCC->IOPENR  |= BIT6;
	GPIOB->MODER |= (BIT12|BIT13);
	
	GPIOB->MODER |= (BIT0|BIT1); // Configure PB0 for input
	
}

void Hardware_Init(void){

	RCC->APB1ENR |= BIT0;  // turn on clock for timer2 (UM: page 177)
	TIM2->ARR = F_CPU/DEF_F-1;
	NVIC->ISER[0] |= BIT15; // enable timer 2 interrupts in the NVIC
	TIM2->CR1 |= BIT4;      // Downcounting    
	TIM2->CR1 |= BIT7;      // ARPE enable    
	TIM2->DIER |= BIT0;     // enable update event (reload event) interrupt 
	TIM2->CR1 |= BIT0;      // enable counting   
	 
	__enable_irq();
}

// LQFP32 pinout
//             ----------
//       VDD -|1       32|- VSS
//      PC14 -|2       31|- BOOT0
//      PC15 -|3       30|- PB7
//      NRST -|4       29|- PB6
//      VDDA -|5       28|- PB5
//      *PA0 -|6       27|- PB4
//      *PA1 -|7       26|- PB3
//      *PA2 -|8       25|- PA15
//      *PA3 -|9       24|- PA14
//      *PA4 -|10      23|- PA13
//      *PA5 -|11      22|- PA12
//       PA6 -|12      21|- PA11
//      *PA7 -|13      20|- PA10 (Reserved for RXD)
//      *PB0 -|14      19|- PA9  (Reserved for TXD)
//       PB1 -|15      18|- PA8  (Measure the period at this pin)
//       VSS -|16      17|- VDD
//             ----------

// * = reserved

void main(void)
{
	long int count;
	float T, f, cap;
	char lcd_buff[17];
	int i;
	float a;
	int j;
	int LED_state = 2;
	Configure_Pins();
	LCD_4BIT();
	Hardware_Init();
	initADC();

	RCC->IOPENR |= 0x00000001; // peripheral clock enable for port A
	
	GPIOA->MODER &= ~(BIT16 | BIT17); // Make pin PA8 input
	// Activate pull up for pin PA8:
	GPIOA->PUPDR |= BIT16; 
	GPIOA->PUPDR &= ~(BIT17); 

	// Configure PA15, PA14, PA13, and PA12 as inputs with pull-up resistors
	GPIOA->MODER &= ~(BIT30 | BIT28 | BIT26 | BIT24); // Clear mode bits for PA15, PA14, PA13, and PA12
	GPIOA->PUPDR |= (BIT30 | BIT28 | BIT26 | BIT24);  // Set pull-up mode for PA15, PA14, PA13, and PA12
	GPIOA->PUPDR &= ~(BIT31 | BIT29 | BIT27 | BIT25); // Clear pull-down mode for PA15, PA14, PA13, and PA12

	waitms(500); // Wait for putty to start.
	//printf("Period measurement using the Systick free running counter.\r\n"
	      //"Connect signal to PA8 (pin 18).\r\n");
	LCDprint("LCD 4-bit test:", 1, 1);
	LCDprint("Hello, World!", 2, 1);
	while(1) {

		int s1 = (GPIOA->IDR & BIT15) ? 1 : 0; // Read PA15
		int s2 = (GPIOA->IDR & BIT14) ? 1 : 0; // z PA14
		int s3 = (GPIOA->IDR & BIT13) ? 1 : 0; // Read PA13
		int s4 = (GPIOA->IDR & BIT12) ? 1 : 0; // Read PA12

		// LED_state = 0 means change nothing
		// LED_state = 1 means change red
		// LED_state = 2 means change blue
		// LED_state = 3 means change green
		if(LED_state == 0){
			j = readADC(ADC_CHSELR_CHSEL6);
			a = (j*3.3)/0x1000;
			
			if(a < 0.1){
				LED_state = 3;
			}
			else if (a > 3.1){
				LED_state = 1;
			}
		}
		else if(LED_state == 1)
		{
			// Changes to pwm3
			
			// Reads VRy Channel
			j = readADC(ADC_CHSELR_CHSEL7);
			a = (j*3.3)/0x1000;

			if(a < 0.1){
				while(a < 0.1){
					pwm3 += 5;
					if(pwm3 >= 255){
						pwm3 = 5;
					}
					waitms(100);
					j = readADC(ADC_CHSELR_CHSEL7);
					a = (j*3.3)/0x1000;
					printf("pwm3: %d\n\r", pwm3);
					
				}
			}
			else if (a > 3.1){
				while(a > 3.1){
					pwm3 = pwm3 - 5;
					if(pwm3 <= 0){
						pwm3 = 250;
					}
					waitms(100);
					j = readADC(ADC_CHSELR_CHSEL7);
					a = (j*3.3)/0x1000;
					printf("pwm3: %d \n\r", pwm3);
				}
			}
			
			// Reads VRx Channel
			j = readADC(ADC_CHSELR_CHSEL6);
			a = (j*3.3)/0x1000;
			
			if(a > 3.0){
				LED_state = 2;
				waitms(500);
			}
			else if (a < 0.1){
				LED_state = 0;
				waitms(500);
			}
		} 
		else if (LED_state == 2) { // Change Blue Channel
			// Reads VRy Channel
			j = readADC(ADC_CHSELR_CHSEL7);
			a = (j*3.3)/0x1000;

			if(a < 0.1){
				while(a < 0.1){
					pwm4 += 5;
					if(pwm4 >= 255){
						pwm4 = 5;
					}
					waitms(100);
					j = readADC(ADC_CHSELR_CHSEL7);
					a = (j*3.3)/0x1000;
					printf("pwm4: %d\n\r", pwm4);
					
				}
			}
			else if (a > 3.1){
				while(a > 3.1){
					pwm4 = pwm4 - 5;
					if(pwm4 <= 0){
						pwm4 = 250;
					}
					waitms(100);
					j = readADC(ADC_CHSELR_CHSEL7);
					a = (j*3.3)/0x1000;
					printf("pwm4: %d \n\r", pwm4);
				}
			}
			// Reads VRx Channel
			j = readADC(ADC_CHSELR_CHSEL6);
			a = (j*3.3)/0x1000;
			
			if(a > 3.0){
				LED_state = 3;
				waitms(500);
			}
			else if (a < 0.1){
				LED_state = 1;
				waitms(500);
			}
		} else if (LED_state == 3) { // Change Green Channel
					// Reads VRy Channel
			j = readADC(ADC_CHSELR_CHSEL7);
			a = (j*3.3)/0x1000;

			if(a < 0.1){
				while(a < 0.1){
					pwm5 += 5;
					if(pwm5 >= 255){
						pwm5 = 5;
					}
					waitms(100);
					j = readADC(ADC_CHSELR_CHSEL7);
					a = (j*3.3)/0x1000;
					printf("pwm5: %d\n\r", pwm5);
					
				}
			}
			else if (a > 3.1){
				while(a > 3.1){
					pwm5 = pwm5 - 5;
					if(pwm5 <= 0){
						pwm5 = 250;
					}
					waitms(100);
					j = readADC(ADC_CHSELR_CHSEL7);
					a = (j*3.3)/0x1000;
					printf("pwm5: %d \n\r", pwm5);
				}
			}
			
			// Reads VRx Channel
			j = readADC(ADC_CHSELR_CHSEL6);
			a = (j*3.3)/0x1000;
			
			if(a > 3.0){
				LED_state = 0;
				waitms(500);
			}
			else if (a < 0.1){
				LED_state = 2;
				waitms(500);
			}
		}

		if (s4 == 1 && s3 == 0 && s2 == 1 && s1 == 0) {
			count=GetPeriod(100);
			if(count>0 && f < 120000) {
				T=count/(F_CPU*100.0); // Since we have the time of 100 periods, we need to divide by 100
				f=1.0/T;
				cap = (1.44/(f*(5000*3)));
				if (cap <= 0.000000002) {
					sprintf(lcd_buff, "Cap: %.3fnF", cap*1000000000);
					LCDprint(lcd_buff, 2, 1);
				}
				if (cap > 0.000000002 && cap <= 0.000001) {
					sprintf(lcd_buff, "Cap: %.3fuF", cap*1000000);
					LCDprint(lcd_buff, 2, 1);
				}
				if (cap > 0.000001 && cap <= 0.001) {
					sprintf(lcd_buff, "Cap: %.3fmF", cap*1000);
					LCDprint(lcd_buff, 2, 1);
				}
			}
			else if (f > 120000) {
				T=count/(F_CPU*100.0); // Since we have the time of 100 periods, we need to divide by 100
				f=1.0/T;
				sprintf(lcd_buff, "NO CAPACITOR            \r");
				LCDprint(lcd_buff, 2, 1);
			}
			fflush(stdout); // GCC printf wants a \n in order to send something.  If \n is not present, we fflush(stdout)
			waitms(200);
			j=readADC(ADC_CHSELR_CHSEL8);
			a=(j*3.3)/0x1000;
			sprintf(lcd_buff,"R=%5.0f%c %c:%d", 1000*(3.3/a -1) 
											  , 0b11110100
											  , (LED_state == 3) ? 'b' : (LED_state == 2) ? 'g' : (LED_state == 1) ? 'r' : ' '
											  , (LED_state == 3) ? pwm5 : (LED_state == 2) ? pwm4 : (LED_state == 1) ? pwm3 : ' ');
			LCDprint(lcd_buff, 1, 1);
		}
		else {
			sprintf(lcd_buff, "%d %d %d %d", !s4, !s3, !s2, !s1);
			LCDprint(lcd_buff, 2, 1);
		 	LCDprint("LOCKED          ", 1, 1);
		}
		
		
	
	}
}

