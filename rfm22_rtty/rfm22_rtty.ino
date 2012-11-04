/* Testprogramm für RFM 22b Modul */

#include <SPI.h>
#include <RFM22.h>
 
//Setup radio on SPI with NSEL on pin 9
rfm22 radio1(9);
char superbuffer [80];
#define RF22_REG_0F_ADC_CONFIGURATION           0x0f
#define RF22_REG_10_ADC_SENSOR_AMP_OFFSET               0x10
#define RF22_REG_11_ADC_VALUE                           0x11
#define RF22_REG_12_TEMPERATURE_SENSOR_CALIBRATION      0x12
#define RF22_REG_13_TEMPERATURE_VALUE_OFFSET            0x13

#define RF22_ENTSOFFS                           0x20
#define RF22_ADCSTART                           0x80
#define RF22_ADCDONE                            0x80
#define RF22_ADCSEL                             0x70
#define RF22_ADCSEL_INTERNAL_TEMPERATURE_SENSOR 0x00
#define RF22_ADCSEL_GPIO0_SINGLE_ENDED          0x10
#define RF22_ADCSEL_GPIO1_SINGLE_ENDED          0x20
#define RF22_ADCSEL_GPIO2_SINGLE_ENDED          0x30
#define RF22_ADCSEL_GPIO0_GPIO1_DIFFERENTIAL    0x40
#define RF22_ADCSEL_GPIO1_GPIO2_DIFFERENTIAL    0x50
#define RF22_ADCSEL_GPIO0_GPIO2_DIFFERENTIAL    0x60
#define RF22_ADCSEL_GND                         0x70
#define RF22_ADCREF                             0x0c
#define RF22_ADCREF_BANDGAP_VOLTAGE             0x00
#define RF22_ADCREF_VDD_ON_3                    0x08
#define RF22_ADCREF_VDD_ON_2                    0x0c
#define RF22_ADCGAIN                            0x03

const float freq = 434.19;


void setupRadio(){
 
//  digitalWrite(5, LOW); // Radio anschalten falls SDN mit Pin 5 verbunden
 
  delay(1000);
 
  rfm22::initSPI();
 
  radio1.init();
 
  radio1.write(0x71, 0x00); // unmodulated carrier
 
  //This sets up the GPIOs to automatically switch the antenna depending on Tx or Rx state, only needs to be done at start up
  radio1.write(0x0b,0x12);
  radio1.write(0x0c,0x15);
 
  radio1.setFrequency(freq);
 
  //Quick test
  radio1.write(0x07, 0x08); // turn tx on
  delay(1000);
  radio1.write(0x07, 0x01); // turn tx off
  radio1.write(0x6d, 0x07); // set output power: RF22_REG_6D_TX_POWER: 00 low , 07 high
  /*
 #define RF22_TXPOW_1DBM                         0x00
#define RF22_TXPOW_2DBM                         0x01
#define RF22_TXPOW_5DBM                         0x02
#define RF22_TXPOW_8DBM                         0x03
#define RF22_TXPOW_11DBM                        0x04
#define RF22_TXPOW_14DBM                        0x05
#define RF22_TXPOW_17DBM                        0x06
#define RF22_TXPOW_20DBM                        0x07
*/
 
}



// RTTY Functions - from RJHARRISON's AVR Code
void rtty_txstring (char * string)
{
 
	/* Simple function to sent a char at a time to 
	** rtty_txbyte function. 
	** NB Each char is one byte (8 Bits)
	*/
	char c;
	c = *string++;
	while ( c != '\0')
	{
		rtty_txbyte (c);
		c = *string++;
	}
        Serial.println();
}
 
void rtty_txbyte (char c)
{
	/* Simple function to sent each bit of a char to 
	** rtty_txbit function. 
	** NB The bits are sent Least Significant Bit first
	**
	** All chars should be preceded with a 0 and 
	** proceded with a 1. 0 = Start bit; 1 = Stop bit
	**
	** ASCII_BIT = 7 or 8 for ASCII-7 / ASCII-8
	*/
	int i;
        Serial.print(c);
        
	rtty_txbit (0); // Start bit
	// Send bits for for char LSB first	
	for (i=0;i<7;i++)
	{
		if (c & 1) rtty_txbit(1); 
			else rtty_txbit(0);	
		c = c >> 1;
	}
	rtty_txbit (1); // Stop bit
        rtty_txbit (1); // Stop bit
}
 
void rtty_txbit (int bit)
{
		if (bit)
		{
		  // high
        radio1.setFrequency(freq);
		}
		else
		{
		  // low
        radio1.setFrequency(freq + 0.0005);
		}
  delayMicroseconds(10000); // For 50 Baud uncomment this and the line below. 
  delayMicroseconds(10150); // For some reason you can't do 20150 it just doesn't work.
 
}

void setup()
{
   Serial.begin(9600);
   Serial.println("RFM22 Test");
   delay(1000);
   setupRadio();
}

void loop(){
      radio1.write(0x07, 0x08); // turn tx on
      delay(5000);
      rtty_txstring("$$PYSY, A, B, C");
//      rtty_txstring(superbuffer);
      radio1.write(0x07, 0x01); // turn tx off
}

