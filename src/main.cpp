/*
  Controlled Aquisition for ATMega328p

  In this example, instead of acquiring ADC samples as de
  conversion finishes, ATMega328p will wait until timer
  interrup is finished. This program allows the user to acquire
  samples at 44.100Hz. Some advantages are:

  1. No oversampling, causing stress in both LabVIEW and serial port
  2. Increased Stability for serial bus
  3. Lower overhead for ATMega328p CPU

 */


#include <avr/io.h>
#include <avr/interrupt.h>

/* Baudrate Definitions */
#define F_CPU 16000000
#define BAUD  1000000
#define BRC   ((F_CPU/16/BAUD) - 1)

/* Port Output Definitions */
#define    CLR(port,pin)  (port &= (~(1<<pin)))
#define    SET(port,pin)  (port |= ( (1<<pin)))
#define TOGGLE(port,pin)  (port ^= ( (1<<pin)))

/* ADC Definitions */
#define MUXMASK 0x07
#define ADC0 0b0000
#define ADC1 0b0001
#define ADC2 0b0010
#define ADC3 0b0011
#define ADC4 0b0100
#define ADC5 0b0101
#define ADC6 0b0110
#define ADC7 0b0111

volatile uint8_t ADC_SPL_COUNT = 0;
#define ADC_SPL_TH 128

void USART_Transmit(uint8_t data)               // (p. 184)
{
  /* Wait until TX data frame is empty */
  while ( ! ( UCSR0A & (1 << UDRE0)))           // (p. 195)
  {;}

  /* Fill Tx data frame with data*/
  UDR0 = data;                                  // (p. 195)
}

ISR(ADC_vect)
{
  /* ADC Conversion Interrupt  */
  /*
     As soon as the ADIF register is high,
     this interrupt service is triggered
  */
  ADC_SPL_COUNT++;
  SET(PORTB, 4);
  /* Send conversion results via USART */
                                 // (p. 265)
  USART_Transmit(ADCH);


  if(ADC_SPL_COUNT >= ADC_SPL_TH)
  {
    USART_Transmit('\n');
    ADC_SPL_COUNT = 0;
  }

  CLR(PORTB, 4);

}

/* Main Code */
int main()
{
  //* Setup USART Interface *//
  /* Set Baudrate for TX & RX*/                         // (p. 183)
  UBRR0H = (BRC >> 8);
  UBRR0L = (BRC     );

  /* Enable Transmitter*/
  UCSR0B = (1 << TXEN0);                                // (p. 183)

  /*  */
  UCSR0A |= (1 << U2X0);

  /* Set up frame size for TX & RX to 8-bit*/
  UCSR0C = (1<< UCSZ01) | (1<< UCSZ00);                 // (p. 198)

  //* Setup ADC *//
  /* Clear ADCSRA & ADCSRB registers */                 // (p. 263)
  ADCSRA = 0x00;
  ADCSRB = 0x00;
  ADMUX  = 0x00;

  /* Set Analog pin */
  ADMUX |= (ADC0 & MUXMASK);                            // (p. 262)

  /* Set Reference Voltage */
  ADMUX |= (1 << REFS0);                                // (p. 262)

  /* Set ADC value alignment */
  ADMUX |= (1 << ADLAR);                                // (p. 262, 265)

  /* Set up ADC clock via prescaler */
  /* Fs = F_CPU/(13.5*prescaler) */
  ADCSRA |= (1 << ADPS2) | (0 << ADPS1) | (0 << ADPS0); // (p. 264, 255)

  /* Set up ADC Auto Trigger Source to Free Running */
  ADCSRB |= (0 << ADTS2) | (0 << ADTS1) | (0 << ADTS0); // (p. 265, 266, 253)

  /* Enable ADC Auto Trigger Mode */
  ADCSRA |= (1 << ADATE);                               // (p. 264)

  /* Enable ADC Interrrupt when measurement is completed */
  ADCSRA |= (1 << ADIE);                                // (p. 264)

  /* Enable ADC */
  ADCSRA |= (1 << ADEN);                                // (p. 263)

  /* Start ADC Conversion */
  ADCSRA |= (1 << ADSC);                                // (p. 263)
}
