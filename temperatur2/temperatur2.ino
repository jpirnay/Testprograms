#include <OneWire.h> 
#include <DallasTemperature.h> 
 
// Data wire is plugged into pin 10 on the Arduino 
#define ONE_WIRE_BUS 10  
 
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
 
// Pass our oneWire reference to Dallas Temperature.  
DallasTemperature sensors(&oneWire); 

int sensorCount; 
void setup(void) 
{ 
int i, j;
DeviceAddress insideThermometer;
// start serial port 
Serial.begin(9600); 
Serial.println("Dallas Temperature IC Control Library Demo"); 
// Start up the library 
sensors.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement 
sensorCount = sensors.getDeviceCount();
Serial.print("Gefundene Sensoren: "); 
Serial.println(sensorCount);
  for (i=0;i<sensorCount;i++) {
    if (sensors.getAddress(insideThermometer, i) ) 
    {
      j = sensors.getResolution(insideThermometer);
      Serial.print ("Auflösung von Sensor ");
      Serial.print (i);
      Serial.print (": ");
      Serial.print (j);
      Serial.println (" bit");
    }
    else
    {
      Serial.print("Adresse zu Device ");
      Serial.print(i);
      Serial.println(" liess sich nicht ermitteln!");
    };  
  }  
} 
 
 
void loop(void) 
{  
int i;
// call sensors.requestTemperatures() to issue a global temperature  
// request to all devices on the bus 
Serial.print("Requesting temperatures..."); 
sensors.requestTemperatures(); // Send the command to get temperatures 
Serial.println("DONE"); 
for (int i=0; i<sensorCount; i++)
{
   Serial.print("Temperature for Device ");
   Serial.print(i);
   Serial.print(" is: "); 
   Serial.print(sensors.getTempCByIndex(i)); // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire 
   Serial.println();
 }
 delay(1000);
}
