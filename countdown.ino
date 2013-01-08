/*
Countdown-Timer für Zünder...
*/
#include <Bounce.h>

const int LED_PIN = 13;
const int FUSE_PIN = 4;
// const int TEST_PIN = 3;
const int BTN_PIN = 2;     // the number of the pushbutton pin

const long cdwn_len = 10 * 1000;
const long fuse_len = 15 * 1000;

long start, blinker;

Bounce bouncer = Bounce(BTN_PIN, 5);

byte mode; // 0 waiting, 1 counting, 2 fusing
byte LED;

void togglecountdown()
{
   if (mode==0)
   {
      startcountdown();
   }
   else
   {
      stopcountdown();
   }  
}

void startcountdown()
{
   start = millis();
   mode = 1; // Counting down      
   LED = HIGH;
   digitalWrite(LED_PIN, LED);
   blinker = start;
}

void stopcountdown()
{
    stopfuse();
}  

void startfuse()
{
    start = millis();
    mode = 2;
    // set fuse pin
    digitalWrite(FUSE_PIN, HIGH);    
}  

void stopfuse()
{
    // reset fuse pin
    // digitalWrite(TEST_PIN, LOW);    
    digitalWrite(FUSE_PIN, LOW);    
    digitalWrite(LED_PIN, LOW);    
    start = 0;
    mode = 0;
    blinker = 0;
    LED = LOW;
}  

void setup()
{ 
  Serial.begin(9600);
  pinMode (LED_PIN, OUTPUT);
  pinMode (FUSE_PIN, OUTPUT);
  // pinMode (TEST_PIN, OUTPUT);
  pinMode (BTN_PIN, INPUT);
  digitalWrite(FUSE_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  stopfuse();
}

void loop()
{
   long now = millis();
   // check button...
   bouncer.update();
   int btnvalue = bouncer.read();
   // digitalWrite(TEST_PIN, btnvalue);
   if (bouncer.risingEdge()) 
   {
     if (mode == 0) 
       { startcountdown(); } 
     else 
       { stopcountdown(); }
   }
    
   if (mode==1)
   {
     if (now-blinker > 500)
     {
       if (LED==HIGH) {LED = LOW;} else {LED = HIGH;}
       digitalWrite(LED_PIN, LED);
       blinker = now;
     }  
     if (now - start > cdwn_len)
      {
         startfuse();
      }  
   }
   if (mode==2)
   {
      if (now-blinker > 100)
      {
        if (LED==HIGH) {LED = LOW;} else {LED = HIGH;}        
        digitalWrite(LED_PIN, LED);
        blinker = now;
      }  
      if (now - start > fuse_len)
      {
         stopfuse();
         stopcountdown();
      }  
   }  
}
