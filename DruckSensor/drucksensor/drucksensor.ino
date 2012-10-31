/*
  Auslesen des MKPX5100A Sensors - Ausgabe absoluter Druck - an Pin 0
  VOUT = VS (P x 0.009 - 0.095)
± (Pressure Error x Temp. Mult. x 0.009 x VS)
  P 15kPA bis 115 kPA
  Auslesen des MKPX5999D Sensors - Ausgabe absoluter Druck - an Pin 1
  VOUT = VS (P x 0.000901 + 0.2)
  P 0kPA bis 1000 kPA
  
*/
int sensorPin1 = A0;    // select the input pin for the potentiometer
int sensorPin2 = A1;    // select the input pin for the potentiometer
int ledPin = 13;      // select the pin for the LED
int sensorValue1;  // variable to store the value coming from the sensor
int sensorValue2;  // variable to store the value coming from the sensor
int ledstate;
int delayFact;
float pascal1, pascal2;
int mindelta;
int maxdelta;
int delta;
int minvalue1;
int maxvalue1;
int minvalue2;
int maxvalue2;
int incoming;
int lastvalue1, lastvalue2;

void setup() {
  Serial.begin(9600);
  Serial.println("Freescale MPX5100A/MPX5999D Drucksensor...");
  Serial.println("Gleichzeitige Aufnahme der Werte zweier Sensoren");
  
  analogReference(DEFAULT); // 5V Referenz!
  // declare the ledPin as an OUTPUT:
  pinMode(ledPin, OUTPUT);  
  ledstate = HIGH;
  delayFact = 3000;
  minvalue1=1024;
  maxvalue1=-1; 
  minvalue2=1024;
  maxvalue2=-1; 
  mindelta=1024;
  maxdelta=-1;
  lastvalue1=-1; 
  lastvalue2=-1;
  // wait while everything stabilizes. 
  delay(1000); 
  Serial.println("Starte Messung...");
}

void loop() {
  // read the value from the sensor:
  sensorValue1 = analogRead(sensorPin1);
  if (sensorValue1<minvalue1) minvalue1=sensorValue1;
  if (sensorValue1>maxvalue1) maxvalue1=sensorValue1;

  sensorValue2 = analogRead(sensorPin2);
  if (sensorValue2<minvalue2) minvalue2=sensorValue2;
  if (sensorValue2>maxvalue2) maxvalue2=sensorValue2;
  // Interne Auflösung des ADC: 10 bit = 2 10 = 1024
  delta = abs(sensorValue1 - sensorValue2);
  if (delta<mindelta) mindelta=delta;
  if (delta>maxdelta) maxdelta=delta;
  
  if ((lastvalue1 != sensorValue1) || (lastvalue2 !=sensorValue2))
  {
    pascal1 = pressure(sensorValue1, 1);
    pascal2 = pressure(sensorValue2, 2);
    Serial.print("[5100] ");
    Serial.print(sensorValue1);
    Serial.print(" = " );
    Serial.print(pascal1, DEC);
    Serial.print("kPa -- [5999] " );
    Serial.print(sensorValue2);
    Serial.print(" = " );
    Serial.print(pascal2, DEC);
    Serial.println("kPa" );
  }
  lastvalue1 = sensorValue1;
  lastvalue2 = sensorValue2;
  
  digitalWrite(ledPin, ledstate);  
  if ( ledstate==HIGH ){ ledstate = LOW; } else { ledstate=HIGH; }
  if (Serial.available() > 0){ incoming = Serial.read(); } else { incoming=0; }
  if (incoming == 's')
   {
     Serial.println("Statistik Sensor 1:");
     Serial.print("Minimum: ");  
     stat1 ( minvalue1, 1 );
     Serial.print("Maximum: ");  
     stat1 ( maxvalue1, 1 );
     Serial.println("Statistik Sensor 2:");
     Serial.print("Minimum: ");  
     stat1 ( minvalue2, 2 );
     Serial.print("Maximum: ");  
     stat1 ( maxvalue2, 2 );
     Serial.print ("Abweichungen: ");
     Serial.print( mindelta );
     Serial.print(" .. ");  
     Serial.println ( maxdelta );

   }
  else if (incoming == 'r')
   {
  minvalue1=1024;
  maxvalue1=-1; 
  minvalue2=1024;
  maxvalue2=-1;
  mindelta=1024;
  maxdelta=-1; 
     Serial.println("Statistik zurückgesetzt!" );
   }  
  delay(delayFact);          
}

float pressure ( int adcvalue, int sensorType ) 
{
  float res, volt, minP, maxP, vOffs, voltrange; 
  
  if (sensorType==1) 
  {
    // MPX5100A
    minP=15; maxP = 115; vOffs=0.0; voltrange = 4.5;
  }
  else
  {
    // MPX5999D
    minP=0; maxP = 1000; vOffs=0.0; voltrange = 4.5;
  };  
  volt=(adcvalue / 1024.0 * 5.0) - vOffs;
  res = minP + volt / voltrange * (maxP - minP);
  return res;
}

void stat1 (int adcvalue, int sensorType)
{
  float pasc;
  float mb;
     pasc = pressure(adcvalue, sensorType);
     mb = 10 * pasc;
     Serial.print(adcvalue);
     Serial.print(" = " );
     Serial.print(pasc, DEC);
     Serial.print("kPa" );
     Serial.print(" = " );
     Serial.print(mb, DEC);
     Serial.println("mBar" );
}
