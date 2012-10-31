#define GPIO_ADDR        0x20           // PCA8574 GPIO w/ all addr pins low
#define IR_RC5           false               // use Phillips RC5 IR protocol instead of Sony 
#define IR_PIN            3             // Interrupt 2 for IR sensor
#define LED_PIN          14             // L1 LED on Logger Shield
#define TIMERBASE_RC5     1778          // 1 bit time of RC5 element

#include <Wire.h>                       // for RTC
#include <LiquidCrystal_I2C.h>          // I2C LCD display driver

// instantiate the library and pass pins for (RS, Enable, D4, D5, D6, D7)
LiquidCrystal_I2C lcd(GPIO_ADDR,16,2);  // set the LCD address, 16 chars, 2 lines
volatile boolean IR_Header = false;     // flag set if IR header was received
volatile boolean IR_Avail = false;      // flag set if IR has been read
volatile unsigned int IR_Cmnd = 0;      // returns IR command code received
volatile byte IR_Dev = 0;               // returns IR device code received
volatile unsigned int ir_mask = 0;      // for masking ir_string 
volatile unsigned int ir_bit_seq = 0;   // for bit number in ir_string
volatile unsigned int ir_string = 0;    // stores bits received
unsigned int IRvalue;                   // value built from digits sent from IR receiver
volatile byte RC5_telgr = 0;            // RC5 telegramm number

void setup()
{
  attachInterrupt(1,IR_ISR,FALLING);    // Catch IR sensor on D3 with interrupt 
  lcd.init();                           // initialize the lcd 
  lcd.backlight();                      // turn on the backlight  
  lcd.setCursor(0, 0);
  lcd.print("IR-Test");
}
void loop()
{
  Check_IR();  
}

///////////////////////////////// Menu and IR Functions Here ///////////////////////////////

void Check_IR(){ // check if remote used and process the menu    
  // IR_Dev not used - only IR_Cmnd - accept any device (i.e. TV, VCR, etc.) command.
  byte IRdigit;                         // normalized digit input from remote

  if(!IR_Avail)return;                  // just get out if a key on IR has not been pressed
  //detachInterrupt(0);                 // uncomment to not count while in menu
  lcd.setCursor(0, 1);
  lcd.print("Code ");
  lcd.print(IR_Cmnd,DEC);
  lcd.print("  ");
  IR_Avail = false;                     // allow IR again
}

#if (IR_RC5 == 0)                       // Sony Remote is used
void IR_ISR(){ // This ISR is called for EACH pulse of the IR sensor
  if(IR_Header == false){               // check for the long start pulse
    for(int i = 0; i<20; i++){          // see if it lasts at least 2 ms
      delayMicroseconds(100);           // delay in chunks to get out ASAP
      if(digitalRead(IR_PIN)) return;   // low active went high so not start pulse
    }  
    IR_Header = true;                   // mark that the start pulse was received
    ir_mask = 1;                        // set up a mask for the next bits
    return;                             // next time ISR called it will start below
  }
  delayMicroseconds(900);               // wait 900 us and test 
  if(!digitalRead(IR_PIN)) ir_string = ir_string | ir_mask;  // LOW is '1' bit (else '0' bit)
  ir_mask = ir_mask << 1;               // receiving LSB first - shift ir_mask to the left 
  ir_bit_seq++;                         // inc the bit counter
  if(ir_bit_seq == 12){                 // after remote sends 12 bits it's done
    IR_Cmnd = ir_string & B1111111;     // mask for the last 7 bits - the command
    ir_string = ir_string >> 7;         // shift the device bits over
    IR_Dev = ir_string & B11111;        // mask for the last 5 bits - the device
    IR_Avail = true;                    // indicate new command received
    digitalWrite(LED_PIN, HIGH);        // turn on LED to show you got something
    ir_bit_seq = 0;                     // clean up . . .
    ir_string = 0;
    ir_mask = 0;
    IR_Header = false;
    for(int i = 0; i<25; i++){  // stop repeat! (10ms/loop) 100ms keychain - 250ms others
      delayMicroseconds(10000);         // 16383 is maximum value so need to repeat
    }
    digitalWrite(LED_PIN, false); 
  }
}
#else                                  // Phillips RC5 Remote is used
void IR_ISR(){ // This ISR is called for EACH pulse of the IR sensor
  detachInterrupt(1);                   // timing is done inside the routine           
    if(RC5_telgr == 0){                     // check for first telegramm only
      while(digitalRead(IR_PIN)==1){         //skip this high period
        delayMicroseconds(10);
      }
      ir_string = 0;                            //preset result string
      delayMicroseconds(TIMERBASE_RC5/4);       //we are in fist "low", wait 1/4 bit
      for (ir_bit_seq = 0; ir_bit_seq < 14; ir_bit_seq++){  //13 bit (2 half bit are lost at start/end)
        if(digitalRead(IR_PIN)==0) ir_string++; //pin low? store: bit is "1"
        delayMicroseconds(TIMERBASE_RC5);       // wait one bit time
        ir_string = ir_string << 1;
      }
      ir_string = ir_string >> 1;
//      Serial.println(ir_string, BIN);        //debug gerard
      IR_Cmnd = ir_string & B111111;          // mask for the last 6 bits - the command
      if(!(ir_string && 4096)) IR_Cmnd = IR_Cmnd + 64 ; // bit 13 ist command extention in RC5
      IR_Avail = true;
      digitalWrite(LED_PIN, HIGH);           // turn on LED to show you got something
//      Serial.println(IR_Cmnd, DEC);        //debug gerard
//      Serial.println(" ");                 //debug gerard
      RC5_telgr++ ;                          // telegramm counter
    }
    else{                                    //second to xth telegramm
      for(int i = 0; i<25; i++){             // stop repeat! (25ms/loop) 
        delayMicroseconds(10000);            // 16383 is maximum value so need to repeat
      }
      RC5_telgr++ ;                          // telegramm counter
      if(RC5_telgr == 3) RC5_telgr = 0;      // last (dummy) telegramm
    }
  attachInterrupt(1,IR_ISR,FALLING);
}
#endif


