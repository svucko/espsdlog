
#include <Wire.h>
#include <RtcDS1307.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>


RtcDS1307<TwoWire> Rtc(Wire);

// Data wire is plugged into port D3 on the NodeMcu (GPIO0)
#define ONE_WIRE_BUS 0
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

File myFile;
String fileName="";

void setup() {
    Serial.begin(115200);
    Serial.println("");
    sensors.begin();//DS18b20
    Rtc.Begin();
    Rtc.SetSquareWavePin(DS1307SquareWaveOut_High);

    if (!Rtc.GetIsRunning()){
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Cuases:
        //    1) the battery on the device is low or even missing and the power line was disconnected
        Serial.println("RTC lost confidence in the DateTime!");
    }else {
        RtcDateTime now = Rtc.GetDateTime();
        Serial.println("");
        printDateTime(now);
    
        fileName=currentFileName(now);
        Serial.print("fileName=");
        Serial.println(fileName);
     }


    Serial.print("Initializing SD card...");
    if (!SD.begin(SS)) {
       Serial.println("initialization failed!");
       return;
    }
  Serial.println("initialization done.");
    
}

void loop() {
if (!Rtc.IsDateTimeValid()) 
    {
        // Common Cuases:
        //    1) the battery on the device is low or even missing and the power line was disconnected
        Serial.println("RTC lost confidence in the DateTime!");
    }
  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now);

  

  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.print(" ");
  Serial.print(sensors.getTempCByIndex(0));
  Serial.println("C");

  delay(10000); // ten seconds



  String dataString = "";
  dataString += stringDateTime(now);
  dataString += (" ");
  dataString += String(sensors.getTempCByIndex(0));


  //Serial.print("dataString=");
  //Serial.println(dataString);

  
  Serial.println(fileName);
  myFile = SD.open(fileName, FILE_WRITE);
  if (myFile) {
    myFile.println(dataString);
    myFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.print("error opening ");
    Serial.println(fileName);
  }
  

  
}


#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%04u.%02u.%02u;%02u:%02u:%02u;"),
            dt.Year(),
            dt.Month(),
            dt.Day(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}

String stringDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%04u.%02u.%02u-%02u:%02u:%02u"),
            dt.Year(),
            dt.Month(),
            dt.Day(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    return datestring;
}

String currentFileName(const RtcDateTime& dt)
{
    char fileName[14];

    snprintf_P(fileName, 
            countof(fileName),
            PSTR("%04u%02u%02u.log"),
            dt.Year(),
            dt.Month(),
            dt.Day());
    return fileName;
}






