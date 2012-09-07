/*
 * passthru.c
 * 
 * Exposes important signals for use with Basys Board, etc.
 * 
 * Created: 9/6/2012 1:20:41 PM
 *  Author: Jennifer Temkin
 */ 

#include <avr/io.h>
#include <pololu/3pi.h>

unsigned int read_trimpot(void);

	/*
	Motor 1 (left):
	PD5	PD6	Function
	 0	 0	 off
	 0	 1	 forward
	 1	 0	 reverse
	 1	 1	 brake (both motor inputs grounded)
	 
	Motor 2 (right):
	PD3	PD6	Function
	 0	 0	 off
	 0	 1	 forward
	 1	 0	 reverse
	 1	 1	 brake (both motor inputs grounded)
	
	Light sensors are analog inputs on PC0-PC4.
	IR LEDs are on PC5.
	Buzzer on PB2.
	Trimpot on ADC7.
	Battery voltage * 2/3 on ADC6.
	*/ 
	 
	/*
	Motor values are forwarded directly.
	M1A -> PB0
	M1B -> PB1
	M2A -> PB4
	M2B -> PB5
	
	Light sensor output values are 1 when sensor > trimpot, 0 otherwise.
	LS1 -> PD0
	LS2 -> PD1
	LS3 -> PD2
	LS4 -> PD4
	LS5 -> PD7
	*/

int main(void)
{
	//Analog readings from light sensors.
	unsigned int sensors[5];
	//Digital values of light sensors.
	unsigned char sensor_bool[5];
	//Analog reading from trimpot.
	unsigned int trimpot;

	//Motor control signals.
	char m1a, m1b, m2a, m2b;
	
	//Set up pin polarity.
	//B: 0011 00xx
	//D: 1111 1111
	DDRD = 0xFF;
	DDRB = 0x30;

	
	//Initialize sensors; spend 5000 timer ticks * 0.4us/tick = 2000 us per conversion
	pololu_3pi_init(5000);	
	
	
    while(1)
    {
		//Read motor input values.
		m1a = PORTB & ~(1 << 0);
		m1b = PORTB & ~(1 << 1);
		m2a = PORTB & ~(1 << 4);
		m2b = PORTB & ~(1 << 5);
		
		//Read light sensors and trimpot.
        read_line_sensors(sensors, IR_EMITTERS_ON_AND_OFF);
		trimpot = read_trimpot();
		
		//Process analog sensor data.
		for (int i = 0; i < 5; ++i)
		{
			//Bring sensor readings within range of trimpot reading and compare to trimpot.
			sensor_bool[i] = (sensors[i] >> 2) > trimpot;
		}
		
		//Output values on port D.
		PORTD = (sensor_bool[4] << 7) | (m1b << 6) | (m1a << 5) | (sensor_bool[3] << 4) | (m2a << 3) | (sensor_bool[2] << 2) | (sensor_bool[1] << 1) | sensor_bool[0];
		
		//Output low on buzzer.
		PORTB &= ~(1 << 2);	
    }
}




unsigned int read_trimpot(void)
{
	unsigned int retval = 0;
	
	//Wait for any current conversion to finish
	while (ADCSRA & (1 << ADSC));
	
	//ADCSRA: 1000 0111
	// bit 7 set: ADC enabled
	// bit 6 clear: don't start conversion
	// bit 5 clear: disable autotrigger
	// bit 4: ADC interrupt flag
	// bit 3 clear: disable ADC interrupt
	// bits 0-2 set: ADC clock prescaler is 128
	ADCSRA = 0x87;
	
	//ADMUX: 0100 0111
	// bit 7-6 = 01: Use AVCC as reference
	// bit 5 clear: right-adjust conversion result
	// bit 4 clear: reserved bit
	// bit 3-0: select which ADC channel to use for conversion (ADC0 = 0000, ADC1 = 0001, etc)
	ADMUX = 0x47;
	

	ADCSRA |= 1 << ADSC;				//Start conversion.
	while (!(ADCSRA & (1 << ADIF)));	//Wait until conversion is complete.
	retval = ADCL | (ADCH << 8);		//Read converted value.
	return retval;
}
