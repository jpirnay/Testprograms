#include <Wire.h>
#include <MPL115A2.h>

MPL115A2 mpl115a2;

void setup(void) 
{
  Serial.begin(9600);
  Serial.println("Hello!");
  
  Serial.println("Getting barometric pressure ...");
  mpl115a2.begin();
  Serial.println("Init ready....");
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
