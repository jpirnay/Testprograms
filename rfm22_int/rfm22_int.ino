/*
 Interrupt Driven RTTY TX Demo
  
 Transmits data via RTTY with an interupt driven subroutine.
  
 By Anthony Stirk M0UPU
  
 October 2012 Version 5
  
 Thanks and credits :
 Evolved from Rob Harrison's RTTY Code.
 Compare match register calculation by Phil Heron.
 Thanks to : http://www.engblaze.com/microcontroller-tutorial-avr-and-arduino-timer-interrupts/
 RFM22B Code from James Coxon http://ukhas.org.uk/guides:rfm22b
 Suggestion to use Frequency Shift Registers by Dave Akerman (Daveake)/Richard Cresswell (Navrac)
  
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
  
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
  
 See <http://www.gnu.org/licenses/>.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/crc16.h>
#include <SPI.h>
#include <RFM22.h>
 
#define ASCII 7          // ASCII 7 or 8
#define STOPBITS 2       // Either 1 or 2
#define TXDELAY 0        // Delay between sentence TX's
#define RTTY_BAUD 50    // Baud rate for use with RFM22B Max = 600
#define RADIO_FREQUENCY 434.198
#define REVERSE false  
 
#define RFM22B_SDN 3
#define RFM22B_PIN 9
 
char datastring[80];
char txstring[80];
volatile int txstatus=1;
volatile int txstringlength=0;
volatile char txc;
volatile int txi;
volatile int txj;
unsigned int count=0;

int AvailRam(){ 
  int memSize = 2048;                   // if ATMega328
  byte *buf;
  while ((buf = (byte *) malloc(--memSize)) == NULL);
  free(buf);
  return memSize;
} 
 
rfm22 radio1(RFM22B_PIN);
 
void setup()
{
//  Serial.begin(9600);
  initialise_interrupt();
  setupRadio();
//  Serial.print("Avail Ram: ");
//  Serial.println(AvailRam());
}
 
void loop()
{
 
  sprintf(datastring,"$$PYSY,%04u,RTTY TEST BEACON - RAM %04u bytes",count, AvailRam()); // Puts the text in the datastring
  unsigned int CHECKSUM = gps_CRC16_checksum(datastring);  // Calculates the checksum for this datastring
  char checksum_str[6];
  sprintf(checksum_str, "*%04X\n", CHECKSUM);
  strcat(datastring,checksum_str);
  delay(1000);
  count++;
}
 
ISR(TIMER1_COMPA_vect)
{
  switch(txstatus) {
  case 0: // This is the optional delay between transmissions.
    txj++;
    if(txj>(TXDELAY*RTTY_BAUD)) {
      txj=0;
      txstatus=1;
//      Serial.println("Delay");
    }
    break;
  case 1: // Initialise transmission, take a copy of the string so it doesn't change mid transmission.
//    Serial.println("Start");
    strcpy(txstring,datastring);
    txstringlength=strlen(txstring);
    txstatus=2;
    txj=0;
    break;
  case 2: // Grab a char and lets go transmit it.
    if ( txj < txstringlength)
    {
      txc = txstring[txj];
//      Serial.print(txc);
      txj++;
      txstatus=3;
      rtty_txbit (0); // Start Bit;
      txi=0;
    }
    else
    {
      txstatus=0; // Should be finished
      txj=0;
    }
    break;
  case 3:
    if(txi<ASCII)
    {
      txi++;
      if (txc & 1) rtty_txbit(1);
      else rtty_txbit(0);  
      txc = txc >> 1;
      break;
    }
    else
    {
      rtty_txbit (1); // One stop Bit in any case, let "case 4" decide, whether we need another one...
      txstatus=4;
      txi=0;
      break;
    }
  case 4:
    if(STOPBITS==2)
    {
      rtty_txbit (1); // Stop Bit
      txstatus=2;
      break;
    }
    else
    {
      txstatus=2;
      break;
    }
 
  }
}
 
void rtty_txbit (int bit)
{
  if (bit)
  {
#if REVERSE
    radio1.write(0x73,0x00); // Low
#else
    radio1.write(0x73,0x03); // High
#endif
}
  else
  {
#if REVERSE
    radio1.write(0x73,0x03); // High
#else
    radio1.write(0x73,0x00); // Low
#endif
  }
}
 
void setupRadio(){
//  pinMode(RFM22B_SDN, OUTPUT);    // RFM22B SDN is on ARDUINO A3
//  digitalWrite(RFM22B_SDN, LOW);
  delay(1000);
  rfm22::initSPI();
  radio1.init();
  radio1.write(0x71, 0x00); // unmodulated carrier
  //This sets up the GPIOs to automatically switch the antenna depending on Tx or Rx state, only needs to be done at start up
  radio1.write(0x0b,0x12);
  radio1.write(0x0c,0x15);
  radio1.setFrequency(RADIO_FREQUENCY);
//  radio1.write(0x6D, 0x07);// turn tx high power 17/20db
  radio1.write(0x6D, 0x02);// turn tx high power 17/20db
  radio1.write(0x07, 0x08);
  delay(500);
}
 
uint16_t gps_CRC16_checksum (char *string)
{
  size_t i;
  uint16_t crc;
  uint8_t c;
 
  crc = 0xFFFF;
 
  // Calculate checksum ignoring all $s
  for (i = 0; i < strlen(string); i++)
  {
    c = string[i];
    if (c!='$') crc = _crc_xmodem_update (crc, c);
  }
 
  return crc;
}   
void initialise_interrupt()
{
  // initialize Timer1
  cli();          // disable global interrupts
  TCCR1A = 0;     // set entire TCCR1A register to 0
  TCCR1B = 0;     // same for TCCR1B
  OCR1A = F_CPU / 1024 / RTTY_BAUD - 1;  // set compare match register to desired timer count:
  TCCR1B |= (1 << WGM12);   // turn on CTC mode:
  // Set CS10 and CS12 bits for:
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);
  // enable timer compare interrupt:
  TIMSK1 |= (1 << OCIE1A);
  sei();          // enable global interrupts
}
