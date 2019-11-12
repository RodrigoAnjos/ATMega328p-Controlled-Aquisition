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

//#include "Arduino.h"
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

static void USART_Transmit(uint8_t data)               // (p. 184)
{
  /* Wait until TX data frame is empty */
  while ( ! ( UCSR0A & (1 << UDRE0)))           // (p. 195)
  {;}

  /* Fill Tx data frame with data*/
  UDR0 = data;                                  // (p. 195)
}

/* Timer 0 Comparator A Interrupt  */
ISR(TIMER0_COMPA_vect)
{
  /* Restart ADC Conversion */
  ADCSRA |= (1 << ADSC);
  /*
      Set ports HIGH for Osciloscope Tracing
      of the code
  */
  SET(PORTB, 4);
  SET(PORTB, 5);

  /* Check to see if conversion is complete */
  if ( ! ( ADCSRA & (1 << ADIF)))
  {
    ADC_SPL_COUNT++;
    /* Transmit Acquired Sample */
    USART_Transmit(ADCH);
    if (ADC_SPL_COUNT >= ADC_SPL_TH)
    {
      /* Transmit termination character */
      /*
          Termination Char will be responsible
          for LabVIEW or other listening
          applications organization while
          listening Serial port. In receiving
          a termination char the program willknow
          that a data block has been transmitted.

          Carefull choice of termination char will
          reduce data loss. Choose a value that
          wont coincide with ADCH values.
      */
      USART_Transmit('\n');

      /* Reset ADC Sample Counter */
      ADC_SPL_COUNT = 0;
    }
  }
  CLR(PORTB, 4);
}

ISR(ADC_vect)
{
  CLR(PORTB, 5);
}


/* Main Code */
int main(void)
{
  //* Setup AVR Core *//
  /* Disable Global Interrupts */
  SREG &= ~(1 << 7);


  //* Setup Hardware Interface *//
  /* Setup pins, PB4 & PB5 as output pins */            // (p. 76)
  /* Reset port B configurations */
  PORTB = 0x00;
  DDRB  = 0x00;
  /* Setup pins 4 & 5 as outputs */
  DDRB  = (1 << DDB4) | (1 << DDB5);

  //* Setup USART Interface *//
  /* Set Baudrate for TX & RX*/                         // (p. 183)
  UBRR0H = (BRC >> 8);
  UBRR0L = (BRC     );
  /* Enable Transmitter */
  UCSR0B = (1 << TXEN0);                                // (p. 183)
  /* Enable double speed USART operation */
  UCSR0A |= (1 << U2X0);
  /* Set up frame size for TX & RX to 8-bit */
  UCSR0C = (1<< UCSZ01) | (1<< UCSZ00);                 // (p. 198)


  //* Steup Timer 0 *//
  /* Clear Previous Configuration */
  TCCR0A  = 0x00;
  TCCR0B  = 0x00;
  TIMSK0  = 0x00;
  /* Put Timer 0 in CTC Mode*/                          // (p. 108, 100)
  TCCR0A |= (0 << WGM00) | (1 << WGM01);
  TCCR0B |= (0 << WGM02);
  /*
      Counter Frequency is determinded by
      F_TIMER0 = F_CPU / (Prescaler*(OCR0A+1))
  */
  /* Setup Timer 0 Prescaler */
  TCCR0B |= (0 << CS02) | (1 << CS01) | (0 << CS00);
  /* Setup Output Compare Value */
  /*
     Getting 48.804kHz from this sketch, hence F_CPU must be 15.617280 MHz
     a 382.720 kHz deviation from the specification. Therefore it is
     generally speaking a good ideia to calibrate and compensate via OCR0A
     the clock deviation.
  */
  OCR0A   = 39; // yields aproximatly 50kHz for my cristal
  /* Setup interrupt Mask */
  TIMSK0 |= (1 << OCIE0A);



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
  /* Set up ADC Auto Trigger Source to Timer 0 Comparator A */
  ADCSRB |= (0 << ADTS2) | (1 << ADTS1) | (1 << ADTS0); // (p. 265, 266, 253)
  /* Enable ADC Auto Trigger Mode */
  ADCSRA |= (1 << ADATE);                               // (p. 264)
  /* Enable ADC Interrrupt when measurement is completed */
  ADCSRA |= (1 << ADIE);                                // (p. 264)
  /* Enable ADC */
  ADCSRA |= (1 << ADEN);                                // (p. 263)
  /* Start ADC Conversion */
  ADCSRA |= (1 << ADSC);                                // (p. 263)

  //* Finalize configurations *//
  /* Re-Enable Global Interrupts */
  /* Disable Global Interrupts */
  SREG |= (1 << 7);

  while (1)
  {

  }
  return 0;
}
/*
void loop()
{

}
*/
