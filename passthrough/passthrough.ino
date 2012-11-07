// PAssthrough-Skecth, damit die Skytraq-SW mit dem Venus-Modul reden kann..

#include <SoftwareSerial.h> // change the parens to angle-brackets
#include <SdFat.h>                      // the complete SD library is too fat (pun intended)  
SdFat sd;
SdFile infile;
SdFile outfile;

SoftwareSerial GPS(6, 7); // 2 is RX, 3 is TX
char c;
int cnt;
char hexer[16];

void DumpIn(char ch)
{
  int i;
  char line[60];
    infile.print(ch);              // log the number
    infile.sync();
    cnt++;
    if (cnt>15)
    {
      infile.println();
      line[0]=0;
      for (i=0;i<16;i++)
      {
        sprintf (line, "%s%02X ", line, byte(hexer[i]));
      }   
      infile.println(line);
      hexer[0]=0;
    }
    hexer[cnt] = ch;  
    infile.sync();
}

void DumpOut(char ch)
{
    outfile.print(ch);              // log the number
    outfile.sync();
}

void setup()
{
  cnt = 0;
  hexer[0]=0;
  Serial.begin(9600);
  GPS.begin(9600);
  sd.begin(10, SPI_HALF_SPEED);
  infile.open("gps_in.log", O_WRONLY | O_CREAT);
  outfile.open("gps_out.log", O_WRONLY | O_CREAT);
}

void loop()
{
  if(GPS.available())
  {
      c = GPS.read();
      Serial.write(c);
      DumpOut(c);
  }    

  if(Serial.available())
  {
    c= Serial.read();
    GPS.write(c);
    DumpIn(c);
  }  
}
