#include <string.h>
#define F_CPU 16000000
#define __AVR_ATmega328__
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define BUTTON_PORT PORTC

typedef struct record {
    uint32_t frame_nr;
    uint8_t button_state[4];
}record;
volatile uint8_t volume=0;
volatile uint8_t button_state[5];
volatile uint8_t last_button_state[5];
volatile uint8_t number_of_notes;
volatile uint8_t recording = 0;
volatile uint8_t playback  = 0;
volatile record  recording_array[128];
volatile uint8_t recording_len=0;
volatile uint32_t frame_nr=0;
volatile uint32_t recording_frame_count;
volatile uint8_t current_record=0;

//volatile uint8_t freq[12]={0,0,0,0,0,0,0,0,0,0,0,0};
volatile uint32_t clock =0;
uint8_t keyboard_col=0;
const uint8_t wavelen[24]={
    135/1.795/2, 143/1.795/2, 152/1.795/2,161/1.795/2,170/1.795/2,180/1.795/2,191/1.795/2,202/1.795/2,214/1.795/2,227/1.795/2,241/1.795/2,255/1.795/2,
    135/1.795,143/1.795,152/1.795,161/1.795,170/1.795,180/1.795,191/1.795,202/1.795,214/1.795,227/1.795,241/1.795,255/1.795
};
volatile uint8_t freq[6]={0,0,0,0,0,0};
volatile uint8_t wave_state=0;
volatile uint8_t oct_up = 1;
/*
ISR(TIMER1_COMPA_vect){
}
*/
ISR(TIMER0_COMPA_vect){
    OCR0A +=freq[0];
    wave_state ^= 1<<0; 
    PORTB += ((wave_state << 1) & 2) -1;
}
ISR(TIMER0_COMPB_vect){
    OCR0B +=freq[1];
    wave_state ^= 1<<1; 
    PORTB += ((wave_state >> 0) & 2) -1;
}
ISR(TIMER1_COMPA_vect){
    OCR1A +=freq[2];
    wave_state ^= 1<<2; 
    PORTB += ((wave_state >> 1) & 2) -1;
}
ISR(TIMER1_COMPB_vect){
    OCR1B +=freq[3];
    wave_state ^= 1<<3; 
    PORTB += ((wave_state >> 2) & 2) -1;
}
ISR(TIMER2_COMPA_vect){
    OCR2A +=freq[4];
    wave_state ^= 1<<4; 
    PORTB += ((wave_state >> 3) & 2) -1;
}
ISR(TIMER2_COMPB_vect){
    OCR2B +=freq[5];
    wave_state ^= 1<<5; 
    PORTB += ((wave_state >> 4) & 2) -1;
}


int main(void) {
    //button_state[1]=0x01;
	TCCR0B= 0;
	TIMSK0= (1<<OCIE0A) | (1<<OCIE0B);
	TCNT0 = 0;


	TCCR1B= 0;
	TIMSK1= (1<<OCIE1A) | (1<<OCIE1B);
	TCNT1 = 0;


	TCCR2B= 0;
	TIMSK2= (1<<OCIE2A) | (1<<OCIE2B);
	TCNT2 = 0;



    DDRC=0x00;
	DDRB=0xFF;
	DDRD=0xFF;
	PORTC=0xFF;
    PORTD=0x00;
    PORTB=0;
	sei();
    PORTD |= _BV(0);
    for (uint8_t keyboard_col = 0 ; keyboard_col < 4 ; keyboard_col++){
      PORTD |= _BV(1);
      PORTD &= ~_BV(1);
    }
    PORTD &= ~_BV(0);
          
	while (1){
 //       cli();

        PORTD &= ~_BV(0);
        for (uint8_t keyboard_col = 0 ; keyboard_col < 5 ; keyboard_col++){
            PORTD |= _BV(1);
            PORTD &= ~_BV(1);
            button_state[keyboard_col]= ~PINC & 0b111111;
        }
        if (recording && (
            button_state[0] != last_button_state[0] ||
            button_state[1] != last_button_state[1] ||
            button_state[2] != last_button_state[2] ||
            button_state[3] != last_button_state[3] 
        )        ){
                recording_array[recording_len].frame_nr=frame_nr;
                recording_array[recording_len].button_state[0]=button_state[0];
                recording_array[recording_len].button_state[1]=button_state[1];
                recording_array[recording_len].button_state[2]=button_state[2];
                recording_array[recording_len].button_state[3]=button_state[3];
                recording_len++;
        }
        if (!((last_button_state[4] >> 0) & 1) && ((button_state[4] >> 0) & 1) ){
            recording=!recording;
            if(!recording) recording_frame_count=frame_nr;
            if(recording) {
                recording_array[0].frame_nr=0;
                recording_array[0].button_state[0]=button_state[0];
                recording_array[0].button_state[1]=button_state[1];
                recording_array[0].button_state[2]=button_state[2];
                recording_array[0].button_state[3]=button_state[3];
                recording_len=1;
            }
            PORTD ^= 1<<2;
            frame_nr=1;
            recording_len=1;
            current_record=0;
        }
        if (!((last_button_state[4] >> 1) & 1) && ((button_state[4] >> 1) & 1) ){
            playback=!playback;
            PORTD ^= 1<<3;
            frame_nr=0;
            current_record=0;
        }

        if (playback){
            button_state[0] |= recording_array[current_record].button_state[0]; 
            button_state[1] |= recording_array[current_record].button_state[1]; 
            button_state[2] |= recording_array[current_record].button_state[2]; 
            button_state[3] |= recording_array[current_record].button_state[3]; 
            if (frame_nr > recording_frame_count){
                frame_nr=0;
                current_record=0;
            }
            if (frame_nr==recording_array[(current_record+1)%recording_frame_count].frame_nr){
                current_record=(current_record+1)%recording_frame_count;
            }
        }
        
        if ((button_state[0]) || (button_state[1]) || (button_state[2]) || (button_state[3]) ){
	TIMSK0= (1<<OCIE0A) | (1<<OCIE0B);
	TIMSK1= (1<<OCIE1A) | (1<<OCIE1B);
	TIMSK2= (1<<OCIE2A) | (1<<OCIE2B);
            uint8_t freqnr=0;

            for (uint8_t i = 0 ; i < 6 ; i++){
                if (button_state[0] & (1<<i)) {
                    freq[freqnr++]=wavelen[i];
                    if (freqnr > 5) break;
                }

                if (button_state[1] & (1<<i)) {
                    freq[freqnr++]=wavelen[i+6];
                    if (freqnr > 5) break;
                }
                if (button_state[2] & (1<<i)) {
                    freq[freqnr++]=wavelen[i+12];
                    if (freqnr > 5) break;
                }

                if (button_state[3] & (1<<i)) {
                    freq[freqnr++]=wavelen[i+18];
                    if (freqnr > 5) break;
                }
            }

            //OCR0A = wavelen[freq1]; 
            //OCR0B = wavelen[freq2]; 
//           TCCR0B= 4 + ((button_state[2] >> 6) & 1);
	        //TCCR0A= ((freqnr>0)<<COM0A0) | ((freqnr>1)<<COM0B0);
	        //TCCR1A= ((freqnr>2)<<COM1A0) | ((freqnr>3)<<COM1B0);
	        //TCCR2A= ((freqnr>4)<<COM2A0) | ((freqnr>5)<<COM2B0);
            

	        TCCR0A= 0;
	        TCCR1A= 0;
	        TCCR2A= 0;
	TIMSK0= (1<<OCIE0A) | (1<<OCIE0B);
	TIMSK1= (1<<OCIE1A) | (1<<OCIE1B);
	TIMSK2= (1<<OCIE2A) | (1<<OCIE2B);
            TCCR0B = (4+1*((button_state[4] >> 4) &1) -1*((button_state[4] >> 2) &1))* (freqnr>0);
            TCCR1B = (4+1*((button_state[4] >> 4) &1) -1*((button_state[4] >> 2) &1))* (freqnr>2);
            TCCR2B = (6+1*((button_state[4] >> 4) &1) -2*((button_state[4] >> 2) &1))* (freqnr>4);
            PORTD|= 1<<4;
        }
        else {
            PORTD &= ~(1<<4);
	        TCCR0A= 0;
	        TCCR1A= 0;
	        TCCR2A= 0;
            TCCR0B = 0;
            TCCR1B = 0;
            TCCR2B = 0;
	TIMSK0= 0;
	TIMSK1= 0;
	TIMSK2= 0;
            TCNT0=0;
            TCNT0=0;
            TCNT0=0;

        }
  //      sei();
        last_button_state[0]=button_state[0];
        last_button_state[1]=button_state[1];
        last_button_state[2]=button_state[2];
        last_button_state[3]=button_state[3];
        last_button_state[4]=button_state[4];
        frame_nr++;
        _delay_us(5000);
	}

	return 0;
}
