/* Brandon Bernier
 * ECE 3525 Embedded Systems
 * Final Project
 * Ultrasonic Rangefinder
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#define BAUD_VAL 23		// 9600 bps for 3.6864 MHz
#define Xtal 3686400
#define prescaler 1
#define N_samples 128
#define Fclk Xtal/prescaler //PWM frequency

//************************** SINE TABLE *************************************
// Samples table : one period sampled on 128 samples and
// quantized on 7 bit
//**************************************************************************
const unsigned char sine_table[N_samples] PROGMEM = {
    64,67,70,73,76,79,82,85,88,91,94,96,99,102,104,106,109,111,113,115,117,118,120,121,123,124,125,126,126,127,127,127,
    127,127,127,127,126,126,125,124,123,121,120,118,117,115,113,111,109,106,104,102,99,96,94,91,88,85,82,79,76,73,70,67,
    64,60,57,54,51,48,45,42,39,36,33,31,28,25,23,21,18,16,14,12,10,9,7,6,4,3,2,1,1,0,0,0,0,0,
    0,0,1,1,2,3,4,6,7,9,10,12,14,16,18,21,23,25,28,31,33,36,39,42,45,48,51,54,57,60};

// Calculate X_SW
// X_SW = round(4*N_samples*f*510/Fclk)
// X_SW = round(f/14.12)
// A4 = 440Hz   31
// B4 = 494Hz   35
// C5 = 523Hz   37
// D5 = 587Hz   42
// E5 = 657Hz   47
// F5 = 698Hz   50
// G5 = 784Hz   56
// A5 = 880Hz   62

const unsigned char auc_frequency[8] = {
    31,35,37,42,47,50,56,62};

//Global Variables
volatile unsigned char X_SW = 0;         // Step size for sine increment
unsigned char i_CurSinVal = 0;  // Current index for sine
unsigned char i_TmpSinVal = 0;  // Temp index for sine

volatile unsigned char count=0;
volatile unsigned char d=0,dim=0;

//Function Prototypes
void initialize(void);
unsigned char decode(unsigned char x);
unsigned char decode_sw(unsigned char x);

//USART Function Definitions
int usart_putchar(char c, FILE *stream){
	if( c == '\n' ) usart_putchar('\r', stream);
	while( !(UCSRA&(1<<UDRE)) );            // Wait until UDR is empty
	UDR = c;
	return 0;
}

int usart_getchar(FILE *stream){
	if( UCSRA&(1<<RXC) ) return UDR;
	else return 0;
}

FILE usart_str = FDEV_SETUP_STREAM(usart_putchar, usart_getchar, _FDEV_SETUP_RW);


int main(void)
{	
	initialize();
	printf("\n\nDistance:\n");
	while(1);
	return 0;
}

ISR(USART_RXC_vect){
    unsigned char x,y;
	x = decode( getchar() );
	if( x == 'R' )
		d = 0;
	else if( x == '\r' )
			printf("%c",x);
	else
	{
		if( count == 1 )
			d += x*100;
		else if( count == 2)
				d+= x*10;
		else if( count == 3){
				d += x;
				if( (d<36) )
        		{
					if(d<9){ y=1; dim=5; PORTB=0x7F;}
					else if(d<12){ y=2; dim=36; PORTB=0x3F;}
					else if(d<15){ y=3; dim=72; PORTB=0x1F;}
					else if(d<18){ y=4; dim=108; PORTB=0x0F;}
					else if(d<21){ y=5; dim=144; PORTB=0x07;}
					else if(d<24){ y=6; dim=180; PORTB=0x03;}
					else if(d<27){ y=7; dim=216; PORTB=0x01;}
					else{ y=8; dim=255; PORTB=0x00;}
		            X_SW = auc_frequency[y-1];
		            TCCR1A |= (1<<COM1A1);      // Turn on PWM
		        }
		        else{
		           	TCCR1A &= ~(1<<COM1A1);     // Turn off PWM
					dim = 0;
					PORTB = 0xFF;
				}
				printf("\t%2d ft %2d in\t",d/12,d%12);
				_delay_ms(1);
		}
	}	
	count = (count+1)%5;
}

ISR(TIMER1_OVF_vect)
{
    i_CurSinVal += X_SW;                                // Increment index based on frequency
    i_TmpSinVal = (char)(((i_CurSinVal+2)>>2)&0x7F);    // Round after fixing resolution and then 7-bit mask
    OCR1A = pgm_read_byte(&sine_table[i_TmpSinVal]);    // Output to speaker
	OCR1B = dim;
}

void initialize(void)
{
    DDRA    =   0x00;
    PORTA   =   0xFF;
	DDRB	=	0xFF;
	PORTB	=	0xFF;

	//PWM
    DDRD    =   (1<<PD4)|(1<<PD5);    // OCR1A as output
    TIMSK   |=  (1<<TOIE1);  // Timer 1 interrupt enable
    TCCR1A  |=  (1<<COM1A1)|(1<<COM1B1); // set at top
    TCCR1A  |=  (1<<WGM10);  // phase-correct, 8 bit PWM
    TCCR1B  |=  (1<<CS10);   // PWM prescaler = 1
	
	//USART
	stdout = stdin = &usart_str;
    UBRRH   =   (unsigned char)(BAUD_VAL>>8);   // Set Baud Rate
	UBRRL   =   (unsigned char)(BAUD_VAL&0x00FF);
    UCSRB   =   (1<<RXEN)|(1<<TXEN);            // Enable RX & TX
    UCSRB   |=  (1<<RXCIE);                     // Enable RXC Interrupt
    UCSRC   =   (1<<URSEL)|(3<<UCSZ0);          // Set 8-N-1
    sei();
}

unsigned char decode(unsigned char x)
{
	unsigned char temp;
	switch(x){
		case '0': temp=0; break;
		case '1': temp=1; break;
		case '2': temp=2; break;
		case '3': temp=3; break;
		case '4': temp=4; break;
		case '5': temp=5; break;
		case '6': temp=6; break;
		case '7': temp=7; break;
		case '8': temp=8; break;
		case '9': temp=9; break;
		case 'R': temp='R'; break;
		case '\r': temp='\r'; break;
		default: break;
	}
	return temp;
}

unsigned char decode_sw(unsigned char x)
{
    unsigned char y;
    switch(x){
        case 0xFE:  y = 1;  break;
        case 0xFD:  y = 2;  break;
        case 0xFB:  y = 3;  break;
        case 0xF7:  y = 4;  break;
        case 0xEF:  y = 5;  break;
        case 0xDF:  y = 6;  break;
        case 0xBF:  y = 7;  break;
        case 0x7F:  y = 8;  break;
        default:    y = 0;  break;
    }
    return y;
}
