#include <Wire.h>
#include <Adafruit_MPL115A2.h>

Adafruit_MPL115A2 mpl115a2;
const int m_shdnPin = 9;
void setup(void) 
{
  pinMode(m_shdnPin, OUTPUT);
  digitalWrite(m_shdnPin, HIGH);
  Serial.begin(9600);
  Serial.println("Hello!");
  
  Serial.println("Getting barometric pressure ...");
  mpl115a2.begin();
}

void loop(void) 
{
  float pressureKPA = 0, temperatureC = 0;    

  pressureKPA = mpl115a2.getPressure();  
  Serial.print("Pressure (kPa): "); Serial.print(pressureKPA, 4); Serial.println(" kPa");

  temperatureC = mpl115a2.getTemperature();  
  Serial.print("Temp (*C): "); Serial.print(temperatureC, 1); Serial.println(" *C");
  
  delay(1000);
}
