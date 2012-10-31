#define DEBUG       true              // if true shows available memory
#define USER_INFO   true               // displays and logs the user name set in shield.h
#define GPS_USED    true                // if true GPS used instead of RTC
#define GPS_SKYTRAQ false               // for EM-406A set to false

// date and time formatting options - apply to both the display and the log. 
#define DATE_FMT         DDMMYY         // pick date format - DDMMYY, MMDDYY, YYMMDD
#define SPEED_KMPH       true           // speed in km/hr if true - else MPH
#define AM_PM_FMT        false          // 12H format - if true must not disp seconds if used
#define NO_SECONDS       false          // set to true if seconds are not displayed or logged
#define AUTO_DST         false          // adjust for US DST if true & GPS used
// END USER PARAMETER AREA


#include <Time.h>                       // time functions
#include <SD.h>                         // for SD Card
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <TinyGPS.h>                    // MODIFIED LIB - parses GPS sentences
// MODIFIED LIB - _GPS_NO_STATS uncommented (and resulting compile error fixed) saves 80 bytes
#include <EEPROM.h>                     // ATmega328's EEPROM used to store settings
#include <avr/pgmspace.h>               // for using PROGMEM

// PIN MAP - Each I/O pin (use or unused) is defined . . .
// I2C Pins for RTC - D19 (SCL), D18 (SDA)
#define BUZZER_PIN       16             // sw jumper for buzzer
#define LED_PIN          14             // L1 LED on Logger Shield
// D13 (CLK), D12 (MISO), D11 (MOSI)    SPI Pins for SD card
#define CS_PIN           10             // Logger Shield Chip Select pin for the SD card
//                        D9 & D8          available
//                        D2               Interrupt 1 for Geiger 
//                        D1 & D0          serial comm to GPS

// do not change these - use DATE_FMT #define on main tab
#define DDMMYY            1             // used with DATE_FMT below
#define YYMMDD            2             // used with DATE_FMT below
#define MMDDYY            3             // used with DATE_FMT below

#define DATE_SEPARATOR_US false // true means "/" false means '.'

// These are DEFAULTS! - only used if menu has not been run
#define DOSE_PARAM        1             // defaults to Dose Mode ON
// #define PRI_RATIO       175             // defaults to SBM-20 ratio
#define PRI_RATIO       122             // defaults to SI-29BG ratio
#define DISP_PERIOD    5000.0           // defaults to 5 sec sample & display
#define LOGGING_PEROID    1             // defaults a 1 min logging period

// EEPROM Address for menu inputs - common for RTC and GPS 
#define DISP_PERIOD_ADDR  0
#define DOSE_ADDR         2
#define LOG_PERIOD_ADDR   4
#define USV_RATIO_ADDR    6

// other defines . . .
#define LOW_VCC          4200 //mV      // if Vcc < LOW_VCC give low voltage warning
#define GPIO_ADDR        0x20           // PCA8574 GPIO w/ all addr pins low
#define ONE_MIN_MAX         5           // overflow for oneMinute
#define TEN_MIN_MAX        59           // overflow for tenMinute
#define TIMERBASE_RC5     1778          // 1 bit time of RC5 element


////////////////////////////////// globals /////////////////////////////////////

// These hold the local values that have been read from EEPROM
unsigned long DoseParam;                // period for sample & display
unsigned long LoggingPeriod;            // mS between writes to the card
float uSvRate;                          // holds the rate selected by jumper
boolean doseParam = false;

byte MenuPos;                           // current menu position 0 based
boolean doseMode = false;               // true when dose mode on - SW_1 on or param
boolean SD_OK = true;                   // assume SD is OK until init
boolean lowVcc = false;                 // true when Vcc < LOW_VCC
boolean DoseDispOn = false;             // flag to keep 2nd line clear after dose display

// variables for counting periods and counts . . .
unsigned long dispPeriodStart, dispPeriod; // for display period
unsigned long dispCnt, dispCPM;         // to count and display CPM

unsigned long logPeriodStart;           // for logging period
unsigned long logCnt, logCPM;           // to count and log CPM

long runCountStart;                     // timer for running average
volatile unsigned long runCnt;          // counter for running averages
int oneMinute[6];                       // array holding counts for 1 minute running average
int tenMinute[60];                      // array holding counts for 10 minute running average
int oneMinuteIndex = 0;                 // index to 1 minute array
int tenMinuteIndex = 0;                 // index to 10 minute array
unsigned long tempSum;                  // for summing running count
boolean dispOneMin = false;             // indicates 1 minute average is available
boolean dispTenMin = false;             // indicates 10 minute average is available

float uSv = 0.0;                        // display CPM converted to VERY APPROXIMATE uSv
float uSvLogged = 0.0;                  // logging CPM converted to VERY APPROXIMATE uSv
float avgCnt;                           // holds the previous moving average count
byte sampleCnt;                         // the number of samples making up the moving average
float temp_uSv = 0.0;                   // for converting CPM to uSv/h for running average
byte altDispCnt = 0;                    // counter to disp running average once / 4 displays
int Vcc_mV;                             // mV of Vcc from last check 

char timeString[10];                    // time format determined by #defines above
char dateString[10];                    // time format determined by #defines above
byte day_of_week,DST;                   // if daylight savings time, DST == true
byte Status;                            // GPS status  'A' = fix / 'V' = no fix NOT USED


////////////////////////////////// FOR GPS MENUS /////////////////////////////////////
#define MAX_MENU  8
#define ZONE_ADDR 10                    // EEPROM Address + 40 for time zone menu input

prog_char string_0[] PROGMEM = "SEC DISP PERIOD";   // "String 0" etc are strings to store - change to suit.
prog_char string_1[] PROGMEM = "1=DOSE MODE ON";
prog_char string_2[] PROGMEM = "MIN LOGGING";
prog_char string_3[] PROGMEM = "CPM->uSv RATIO";
prog_char string_4[] PROGMEM = "ALARM > CPM";
prog_char string_5[] PROGMEM = "ZONE (>12= +)";
prog_char string_6[] PROGMEM = "Toggle Buzzer";
prog_char string_7[] PROGMEM = "Battery voltage:";

PROGMEM const char *menu_table[] = 	   // change "string_table" name to suit
{   
  string_0,
  string_1,
  string_2,
  string_3,
  string_4,
  string_5,
  string_6,
  string_7  
};

char dispBuff[30];    // progmem buffer - sized for GPS mode
int GMT_Offset;


////////////////////////////////// PROGMEM LITS ///////////////////////////////
// ========== in Main Tab =============
prog_char lit_0[] PROGMEM = "GEIGER LOGGER ";
prog_char lit_1[] PROGMEM = " CPM to uSv";
prog_char lit_2[] PROGMEM = "Running at ";
prog_char lit_3[] PROGMEM = "RAM Avail: ";
// ==========in SD Logging Tab =============
prog_char lit_4[] PROGMEM = "Date,Time,";
prog_char lit_5[] PROGMEM = "latitude,longitude,";
prog_char lit_6[] PROGMEM = "CPM,uSv,Vcc";
// ==========modified text =================
prog_char lit_7[] PROGMEM = "Version: 4.0d   ";
prog_char lit_8[] PROGMEM = "Owner: J.Pirnay ";
prog_char lit_9[] PROGMEM = "Prj.Stratosphere";
PROGMEM const char *lit_table[] = 	   // change "string_table" name to suit
{   
  lit_0, 
  lit_1,
  lit_2,
  lit_3,
  lit_4,
  lit_5, 
  lit_6,
  lit_7, 
  lit_8,
  lit_9
};

///////////////////////////// for EM-406A GPS /////////////////////////////////
#if (GPS_SKYTRAQ)     // SkyTraq st22 
#define GPSRATE  9600 // baud rate of GPS 
#define RMC_OFF     4 // set binary mode (turns off NMEA output}
#define RMC_READ    5 // set NMEA mode (turns on NMEA output}
#define NMEA_OFF    6 // turn off all NMEA telegramm except RMC


// CHECKSUM CALCULATOR: http://www.hhhh.org/wiml/proj/nmeaxor.html
prog_char cmnd_0[] PROGMEM = "$PSRF103,00,00,00,01*24\r\n"; //GGA_OFF
prog_char cmnd_1[] PROGMEM = "$PSRF103,01,00,00,01*27\r\n"; //GLL_OFF
prog_char cmnd_2[] PROGMEM = "$PSRF103,02,00,00,01*26\r\n"; //GSA_OFF
prog_char cmnd_3[] PROGMEM = "$PSRF103,03,00,00,01*27\r\n"; //GSV_OFF
prog_char cmnd_4[] PROGMEM = {0xA0,0xA1,0x00,0x02,0x09,0x02,0x0B,0x0D,0x0A}; //ser minary mode (RMC_OFF)
prog_char cmnd_5[] PROGMEM = {0xA0,0xA1,0x00,0x02,0x09,0x01,0x08,0x0D,0x0A}; //set NMEA mode (RMC_READ)
prog_char cmnd_6[] PROGMEM = {0xA0,0xA1,0x00,0x09,0x08,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x09,0x0D,0x0A}; // turn off all NMEA except RMC
prog_char cmnd_7[] PROGMEM = "$PSRF105,00*3F\r\n"; //DDM_OFF
prog_char cmnd_8[] PROGMEM = "$PSRF151,00*3E\r\n"; //WAAS_OFF

#else                 // EM-406A 
#define GPSRATE  9600 // baud rate of GPS - usually 48,N,8,1
#define GGA_OFF     0 // GGA-Global Positioning System Fixed Data, message 103,00
#define GLL_OFF     1 // GLL-Geographic Position-Latitude/Longitude, message 103,01
#define GSA_OFF     2 // GSA-GNSS DOP and Active Satellites, message 103,02
#define GSV_OFF     3 // GSV-GNSS Satellites in View, message 103,03
#define RMC_OFF     4 // RMC-Recommended Minimum Specific GNSS Data, message 103,04
#define RMC_READ    5 // RMC-Recommended Minimum Specific GNSS Data, message 103,04
#define VTG_OFF     6 // VTG-Course Over Ground and Ground Speed, message 103,05
#define DDM_OFF     7 // GPS diagnostics off
#define WAAS_OFF    8 // WAAS mode off - increases time for a fix


// CHECKSUM CALCULATOR: http://www.hhhh.org/wiml/proj/nmeaxor.html
prog_char cmnd_0[] PROGMEM = "$PSRF103,00,00,00,01*24\r\n"; //GGA_OFF
prog_char cmnd_1[] PROGMEM = "$PSRF103,01,00,00,01*27\r\n"; //GLL_OFF
prog_char cmnd_2[] PROGMEM = "$PSRF103,02,00,00,01*26\r\n"; //GSA_OFF
prog_char cmnd_3[] PROGMEM = "$PSRF103,03,00,00,01*27\r\n"; //GSV_OFF
prog_char cmnd_4[] PROGMEM = "$PSRF103,04,00,00,01*20\r\n"; //RMC_OFF
prog_char cmnd_5[] PROGMEM = "$PSRF103,04,01,00,01*21\r\n"; //RMC_READ
prog_char cmnd_6[] PROGMEM = "$PSRF103,05,00,00,01*21\r\n"; //VTG_OFF
prog_char cmnd_7[] PROGMEM = "$PSRF105,00*3F\r\n"; //DDM_OFF
prog_char cmnd_8[] PROGMEM = "$PSRF151,00*3E\r\n"; //WAAS_OFF
#endif
PROGMEM const char *cmnd_table[] = 	   // change "string_table" name to suit
{   
  cmnd_0,
  cmnd_1,
  cmnd_2,
  cmnd_3,
  cmnd_4,
  cmnd_5, 
  cmnd_6,
  cmnd_7,
  cmnd_8
};

TinyGPS gps;                            // GPS conversion library
File logfile;                           // the logging file
File dbgfile;                           // the debugging file
File gpsfile;                           // the debugging file
Adafruit_BMP085 bmp;                    // barometric sensor
char serialinput;

void setup()
{
  SD_OK = false;
  Serial.begin(9600);                // comspec for GPS is 48,N,8,1

  attachInterrupt(0,GetEvent,FALLING);  // Geiger event on D2 triggers interrupt
//  digitalWrite(2, HIGH);              // set pull up on INT0
  pinMode(LED_PIN, OUTPUT);             // setup LED pin
  pinMode(10, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);          // sw control buzzer
  digitalWrite(BUZZER_PIN, HIGH);       // buzzer = on

  Get_Settings();                       // get the settings stored in EEPROM
  Blink(LED_PIN,3);                     // show it's alive (LED is on pin 14)

  Serial.print("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
  initCard();
 
  // open the file for reading:
  dumpFile("test.txt");
  writeFile("wtest.txt");
  dumpFile("wtest.txt");
}

void writeFile(const char *filename)
{
  File myFile;
  Serial.println("----------------------------");
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open(filename, FILE_WRITE);
 
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to '");
    Serial.print(filename);
    Serial.println("'");
    
    myFile.println("testing 1, 2, 3.");
	// close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("Couldn't write to '");
    Serial.print(filename);
    Serial.println("'");
  }
}

void dumpFile(const char *filename)
{
  File myFile;
  Serial.println("----------------------------");
  myFile = SD.open(filename);
  if (myFile) {
    Serial.print("Content of: '");
    Serial.print(filename);
    Serial.println("'");
  
    // read from the file until there's nothing else in it:
    while (myFile.available()) {
    	Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
  	// if the file didn't open, print an error:
    Serial.print("error opening '");
    Serial.print(filename);
    Serial.println("'");
  }

}
/* --------------------------------------------------------------------------- */

void LogCount(unsigned long lcnt){
  // SD File format: "Date", "Time", latitude, longitude, speed, CPM, uSv/hr, Vcc, temperature, pressure1, pressure2
  bool newdata = false; //TODO
  float flat, flon;
  float bmp_temp;
  int bmp_pascal, bmp_raw_pascal;
  unsigned long age;

  if (!SD_OK) return;
  if (LoggingPeriod==0) LoggingPeriod = 1;
  if (uSvRate==0) uSvRate= 1;
  
  
  bmp_temp = bmp.readTemperature();
  bmp_pascal = bmp.readPressure();
  bmp_raw_pascal = bmp.readRawPressure();
  
  
  FormatDateTime();                     // format the latest time into the strings
  Vcc_mV = readVcc();                   // check voltage
  if (Vcc_mV <= LOW_VCC) lowVcc = true; // check if Vcc is low 
  else lowVcc = false;

  // Date & Time . . .
  logfile.print("\"");                  // log the time and date (quoted)
  logfile.print(dateString);            // formatted date string
  logfile.print("\",\"");               // and comma delimited
  logfile.print(timeString);            // formatted time string
#if (AM_PM_FMT)                         // print AM/PM if defined
  if (isPM()) logfile.print("pm"); 
  else logfile.print("am");
#endif
  logfile.print("\"");                 

  // GPS . . .
  readGPS();                              // get a reading
  newdata = true; // FORCING THIS FOR NOW

  gps.f_get_position(&flat, &flon, &age); // get the current GPS position
  logfile.print(",");                     // comma delimited
  logfile.print(flat,8);                  // now log the lat from the GPS

  logfile.print(",");                     // comma delimited
  logfile.print(flon,8);                  // now log the lon from the GPS

  logfile.print(",");                     // comma delimited
  logfile.print(gps.f_speed_kmph(),1);    // now log the speed from the GPS

  logfile.print(",");                     // comma delimited
  logfile.print(gps.altitude());           // now log the altitude from the GPS

  logfile.print(",");                     // comma delimited
  logfile.print(gps.satellites());           // now log the # of sats used from the GPS

  // Log counts . . .
  logCPM = float(lcnt) / (float(LoggingPeriod) / 60000);
  uSvLogged = logCPM / uSvRate;         // make uSV conversion
  logfile.print(",");                     // comma delimited
  logfile.print(logCPM,DEC); // log the CPM

  logfile.print(",");                   // comma delimited
  logfile.print(uSvLogged,4);

  logfile.print(",");                   // comma delimited
  logfile.print(Vcc_mV/1000. ,2);       //convert to Float, divide, and print 2 dec places
// pressure data
  logfile.print(","); // comma delimited
  logfile.print(bmp_temp, 2);           // 2 decimal places should be enough :-)

  logfile.print(","); // comma delimited
  logfile.print(bmp_pascal);           // Pressure in Pascal

  logfile.print(","); // comma delimited
  logfile.print(bmp_raw_pascal);           // Pressure in Pascal
  
  logfile.println();

  // really need to tell if the GPS has a fix
  if (newdata = true) Blink(LED_PIN,1); // show it's receiving
  logfile.flush();                      // close the file
}

void initCard(){   // initialize the SD card
  char filename[]="pysy_000.txt";
  pinMode(10, OUTPUT);                  // must set DEFAULT CS pin output, even if not used

  if (!SD.begin(10)) { // see if the card is present and can be initialized:
    error("Card");
  }
  Serial.println("Setting Logfilename");
  for (uint8_t i = 0; i < 250; i++) {
    filename[5] = i/100 + '0';
    filename[6] = (i/10)/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      dbgfile = SD.open(filename, O_CREAT | O_WRITE);  // only open a new file if it doesn't exist
      break;  // leave the loop!
    }
  }
  if (! dbgfile) {
    error("Msg-File");
    return;
  }

  Serial.println("Setting CSVfilename");
  filename[9] = 'c';
  filename[10] = 's';
  filename[11] = 'v';

  for (uint8_t i = 0; i < 250; i++) {
    filename[5] = i/100 + '0';
    filename[6] = (i/10)/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      logfile = SD.open(filename, O_CREAT | O_WRITE);  // only open a new file if it doesn't exist, attention speedup for sd write you have to flush yourself!!
      break;  // leave the loop!
    }
  }
  if (! logfile) {
    error("Log-File");
    return;
  }
  logPrint("Log File:");               // display filename you're are logging to . . .
  logPrintln(filename);                // this file name 

  Serial.println("Setting GPSfilename");

  filename[9] = 'g';
  filename[10] = 'p';
  filename[11] = 's';

  for (uint8_t i = 0; i < 250; i++) {
    filename[5] = i/100 + '0';
    filename[6] = (i/10)/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      gpsfile = SD.open(filename, O_CREAT | O_WRITE);  // only open a new file if it doesn't exist, attention speedup for sd write you have to flush yourself!!
      break;  // leave the loop!
    }
  }
  if (! logfile) {
    error("Log-File");
    return;
  }
  logPrint("GPS File:");               // display filename you're are logging to . . .
  logPrintln(filename);                // this file name 

  Serial.println("Writing Header to CSV");
  // Begin header for Excel file - using PROGMEM for some of the header lits

  logfile.println("Date,Time,Lat,Lon,KMPH,Alt,Sats,CPM,uSv,Vcc,TEMP,PRESS1,PRESS2");
  logfile.flush();

  Serial.println("initCard() ready");
  
  SD_OK = true;
}

void logPrint(char *message)
{
  dbgfile.print(message);
}  

void logPrintln(char *message)
{
  dbgfile.println(message);
  dbgfile.flush();
}  

void gpsdump(char c)
{
  if (c=='$') {
    gpsfile.println();
    gpsfile.flush();
  }	
  gpsfile.print(c);
}

void error(char *str){
  Serial.println("CARD!");
  Serial.println(str);                       // display this error or status
  SD_OK = false;                        // don't try to write to the SD card
}

///////////////////////////////// Time Functions Here ///////////////////////////////

void FormatDateTime(){  // get the time and date from the GPS and format it
  int dispYear;
  int i;
  memset(timeString,0,sizeof(timeString));  // initialize the strings
  memset(dateString,0,sizeof(dateString));

  // convert Sun=1 format to Sun=7 format (DST calc is based on this)
  day_of_week = (weekday()==1) ? 7: (weekday() - 1);

  dispYear = year() -2000;                 // (creates a bug in 89 years)
  if (dispYear <0) dispYear =0;            // no negative year if no fix

  // (The time lib will deal with AM/PM and 12 hour clock)
  // make time string
#if (AM_PM_FMT)
  AppendToString (hourFormat12(),timeString); // add 12 hour time to string
#else
  AppendToString (hour(),timeString);         // add 24 hour time to string
#endif
  strcat(timeString,":");
  if (minute() < 10) strcat(timeString,"0");
  AppendToString (minute(),timeString);       // add MINUTES to string
  if (NO_SECONDS == false){
    strcat(timeString,":");
    if (second() < 10) strcat(timeString,"0");
    AppendToString (second(),timeString);     // add SECONDS to string
  }
  // make date string
  if (DATE_FMT == DDMMYY) i=day();          // add DAY to string
  else if(DATE_FMT == MMDDYY)  i = month();  // add MONTH to string  
  else {                                                             // add YEAR to string
    i =dispYear;
  }  
 if (i < 10) strcat(dateString,"0");
 AppendToString (i,dateString);                          // add MONTH to string  
#if (DATE_SEPARATOR_US) 
  strcat(dateString, "/");
#else 
  strcat(dateString, ".");
#endif
  if (DATE_FMT == MMDDYY)
  { 
    i = day(); // add DAY to string
  }   
  else
 { 
    i = month();                          // add MONTH to string  
 }
 if (i < 10) strcat(dateString,"0");
 AppendToString (i,dateString);                          // add MONTH to string  
 
#if (DATE_SEPARATOR_US) 
  strcat(dateString, "/");
#else 
  strcat(dateString, ".");
#endif
  if (DATE_FMT == YYMMDD)
  {  
    i = day();
    if (i < 10) strcat(dateString,"0");
    AppendToString (i,dateString);          // add DAY to string
  }
  else {
    if (dispYear < 10) strcat(dateString,"0");
    AppendToString (dispYear,dateString);                            // add YEAR to string
  }
}


void AppendToString (int iValue, char *pString){ // appends a byte to string passed
  char tempStr[6];
  memset(tempStr,'\0',sizeof(tempStr));
  itoa(iValue,tempStr,10);
  strcat(pString,tempStr);
}


void dispDateTime(){   // display date and time on Serial port
  FormatDateTime();                     // format the latest time into the strings
  logPrint(dateString);                // formatted date string
  logPrint(" ");
  logPrintln(timeString);                // formatted time string
}


void printDigits(int digits){   // prints preceding colon and leading 0
  logPrint(":");
  if(digits < 10) logfile.print('0');
  logfile.print(digits);
}


bool InDST(){  // Returns true if in DST - Caution: works for US standard only
  // DST starts the 2nd Sunday in March and ends the 1st Sunday in November
  byte DOWinDST, nextSun;
  int Dy, Mn;  
/*
  Dy = tmDay;
  Mn = tmMonth;
  //Dy = 27;  // FOR TESTING
  //Mn = 7;

  // Pre-qualify for in DST in the widest range (any date between 3/8 and 11/7) 
  if ((Mn == 3 && Dy >= 8) || (Mn > 3 && Mn < 11) || (Mn == 11 && Dy <= 7) ){
    // DST start could be as late as 3/14 and end as early as 11/1
    DOWinDST = true;                    // assume it's in DST until proven otherwise
    nextSun = Dy + (7 - day_of_week);   // find the date of the next Sunday
    if (nextSun == Dy) nextSun += 7;    // take care of today being Sunday
    if (Mn == 3 && (Dy <= nextSun)) DOWinDST = false;     // it's before the 2nd Sun in March
    if (Mn == 11 && (Dy >= nextSun -7)) DOWinDST = false; // it's after the 1st Sun in Nov.
    if (DOWinDST)return true;           // DOW is still OK so it's in DST
  }
 */ 
  return false;                         // Nope, not in DST
}

/////////////////////////////////// GPS Functions ////////////////////////////////
bool readGPS(){  //  request and get a sentence from the GPS
char c;
  while (Serial.available()){           // Get sentence from GPS
    c = Serial.read();
    if ( c=='$' ){
      gpsfile.println();
      gpsfile.flush();
    }  
    gpsfile.print(c);
    if (gps.encode(c)) return true;
  }
  return false;
}


time_t gpsTimeSync(){  //  returns time if avail from gps, else returns 0
  unsigned long fix_age = 0 ;
  gps.get_datetime(NULL,NULL, &fix_age);

  if(fix_age < 1000) return gpsTimeToArduinoTime();   // only return time if recent fix  
  return 0;
}


time_t gpsTimeToArduinoTime(){  // returns time_t from GPS with GMT_Offset for hours
  tmElements_t tm;
  int year;
  readGPS();                            // request RMC from GPS and feed to tinyGPS
  gps.crack_datetime(&year, &tm.Month, &tm.Day, &tm.Hour, &tm.Minute, &tm.Second, NULL, NULL);
  tm.Year = year - 1970; 
  time_t time = makeTime(tm);
#if (AUTO_DST == 1)
  if (InDST()) time = time + SECS_PER_HOUR;
#endif
  return time + (GMT_Offset * SECS_PER_HOUR);
}


void GPSinit(){   // send commands to the GPS for what to output - uses PROGMEM for this

}

 

int AvailRam(){ 
  int memSize = 2048;                   // if ATMega328
  byte *buf;
  while ((buf = (byte *) malloc(--memSize)) == NULL);
  free(buf);
  return memSize;
} 

byte getLength(unsigned long number){
  byte length = 0;

  for (byte i = 1; i < 10; i++){
    if (number > pow(10,i)) length = i;
    else return length +1;
  }
}

void GetEvent(){   // ISR triggered for each new event (count)
  dispCnt++;
  logCnt++;
  runCnt++;
}

void Get_Settings(){ // read setting out of EEPROM and set local variables
  // set defaults if EEPROM has not been used yet
  dispPeriod = readParam(DISP_PERIOD_ADDR);
  if (dispPeriod == 0 || dispPeriod > 360){      // default if > 1 hr
    writeParam(DISP_PERIOD,DISP_PERIOD_ADDR);    // write EEPROM
    dispPeriod = DISP_PERIOD;
  }
  else dispPeriod = dispPeriod * 1000;

  DoseParam = readParam(DOSE_ADDR);
  if (DoseParam > 1000) DoseParam = DOSE_PARAM;
  if (DoseParam == 1) doseParam = true;          // start dose display mode if param set to 1
  else doseParam = false;                        // any other setting and dose mode is false


  uSvRate = readParam(USV_RATIO_ADDR);
  if (uSvRate == 0 || uSvRate > 2000){           // defult if 0 or > 2000
    writeParam(PRI_RATIO,USV_RATIO_ADDR);        // write EEPROM
    uSvRate = PRI_RATIO; 
  }

  GMT_Offset = readParam(ZONE_ADDR);
  // to avoid entering negative offsets 1-12 = neg zones, 13-24 = pos zones 1-12
  if (GMT_Offset >12) GMT_Offset = (GMT_Offset - 12);
  else GMT_Offset = GMT_Offset * -1;

  dispPeriodStart = 0;                  // start timing over when returning to loop"
  logPeriodStart = dispPeriodStart;     // start logging timer
}

void writeParam(unsigned int value, unsigned int addr){ // Write menu entries to EEPROM
  unsigned int a = value/256;
  unsigned int b = value % 256;
  if (addr >=12) addr = addr + 48;
  EEPROM.write(addr,a);
  EEPROM.write(addr+1,b);
}


unsigned int readParam(unsigned int addr){ // Read previous menu entries from EEPROM
  if (addr >=12) addr = addr + 48;
  unsigned int a=EEPROM.read(addr);
  unsigned int b=EEPROM.read(addr+1);
  return a*256+b; 
}

long readVcc() { // SecretVoltmeter from TinkerIt
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

void Blink(byte led, byte times){ // just to flash the LED
  for (byte i=0; i< times; i++){
    digitalWrite(led,HIGH);
    delay (120);
    digitalWrite(led,LOW);
    delay (90);
  }
}

void loop()
{
    if (millis() >= logPeriodStart + LoggingPeriod){ // LOGGING PERIOD
      if (SD_OK) LogCount(logCnt);      // pass in the counts to be logged
      logCnt = 0;                       // reset log event counter
      dispCnt = 0;                      // reset display event counter too
      dispPeriodStart = millis();       // reset display time too
      logPeriodStart = millis();        // reset log time
    }
}

