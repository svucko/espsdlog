/*
  SDWebServer - Example WebServer with SD Card backend for esp8266
  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WebServer library for Arduino environment.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  Have a FAT Formatted SD Card connected to the SPI port of the ESP8266
  The web root is the SD Card root folder
  File extensions with more than 3 charecters are not supported by the SD Library
  File Names longer than 8 charecters will be truncated by the SD library, so keep filenames shorter
  index.htm is the default index (works on subfolders as well)
  upload the contents of SdRoot to the root of the SDcard and access the editor by going to http://esp8266sd.local/edit
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SPI.h>
#include <SD.h>

#include <Ticker.h>


#include <OneWire.h>
#include <DallasTemperature.h>

//#include <ESP8266HTTPUpdateServer.h>

#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS1307.h>
RtcDS1307<TwoWire> Rtc(Wire);

RtcDateTime now;


// Data wire is plugged into port D3 on the NodeMcu (GPIO0)
#define ONE_WIRE_BUS 0
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);


Ticker ticker;




#define DBG_OUTPUT_PORT Serial

const char* ssid = "SV";
const char* password = "deadbeef";
const char* host = "esp8266";

ESP8266WebServer server(80);

char datestring[20];
String message, javaScript, XML;

File myFile;
String logFileName = "";


static bool hasSD = false;
File uploadFile;


void returnOK() {
  server.send(200, "text/plain", "");
}

void returnFail(String msg) {
  server.send(500, "text/plain", msg + "\r\n");
}

bool loadFromSdCard(String path) {
  String dataType = "text/plain";
  if (path.endsWith("/")) {
    path += "index.htm";
  }

  if (path.endsWith(".src")) {
    path = path.substring(0, path.lastIndexOf("."));
  } else if (path.endsWith(".htm")) {
    dataType = "text/html";
  } else if (path.endsWith(".css")) {
    dataType = "text/css";
  } else if (path.endsWith(".js")) {
    dataType = "application/javascript";
  } else if (path.endsWith(".png")) {
    dataType = "image/png";
  } else if (path.endsWith(".gif")) {
    dataType = "image/gif";
  } else if (path.endsWith(".jpg")) {
    dataType = "image/jpeg";
  } else if (path.endsWith(".ico")) {
    dataType = "image/x-icon";
  } else if (path.endsWith(".xml")) {
    dataType = "text/xml";
  } else if (path.endsWith(".pdf")) {
    dataType = "application/pdf";
  } else if (path.endsWith(".zip")) {
    dataType = "application/zip";
  }

  File dataFile = SD.open(path.c_str());
  if (dataFile.isDirectory()) {
    path += "/index.htm";
    dataType = "text/html";
    dataFile = SD.open(path.c_str());
  }

  if (!dataFile) {
    return false;
  }

  if (server.hasArg("download")) {
    dataType = "application/octet-stream";
  }

  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    DBG_OUTPUT_PORT.println("Sent less data than expected!");
  }

  dataFile.close();
  return true;
}

void handleFileUpload() {
  if (server.uri() != "/edit") {
    return;
  }
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (SD.exists((char *)upload.filename.c_str())) {
      SD.remove((char *)upload.filename.c_str());
    }
    uploadFile = SD.open(upload.filename.c_str(), FILE_WRITE);
    DBG_OUTPUT_PORT.print("Upload: START, filename: "); DBG_OUTPUT_PORT.println(upload.filename);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
    DBG_OUTPUT_PORT.print("Upload: WRITE, Bytes: "); DBG_OUTPUT_PORT.println(upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
    }
    DBG_OUTPUT_PORT.print("Upload: END, Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void deleteRecursive(String path) {
  File file = SD.open((char *)path.c_str());
  if (!file.isDirectory()) {
    file.close();
    SD.remove((char *)path.c_str());
    return;
  }

  file.rewindDirectory();
  while (true) {
    File entry = file.openNextFile();
    if (!entry) {
      break;
    }
    String entryPath = path + "/" + entry.name();
    if (entry.isDirectory()) {
      entry.close();
      deleteRecursive(entryPath);
    } else {
      entry.close();
      SD.remove((char *)entryPath.c_str());
    }
    yield();
  }

  SD.rmdir((char *)path.c_str());
  file.close();
}

void handleDelete() {
  if (server.args() == 0) {
    return returnFail("BAD ARGS");
  }
  String path = server.arg(0);
  if (path == "/" || !SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }
  deleteRecursive(path);
  returnOK();
}

void handleCreate() {
  if (server.args() == 0) {
    return returnFail("BAD ARGS");
  }
  String path = server.arg(0);
  if (path == "/" || SD.exists((char *)path.c_str())) {
    returnFail("BAD PATH");
    return;
  }

  if (path.indexOf('.') > 0) {
    File file = SD.open((char *)path.c_str(), FILE_WRITE);
    if (file) {
      file.write((const char *)0);
      file.close();
    }
  } else {
    SD.mkdir((char *)path.c_str());
  }
  returnOK();
}

void printDirectory() {
  if (!server.hasArg("dir")) {
    return returnFail("BAD ARGS");
  }
  String path = server.arg("dir");
  if (path != "/" && !SD.exists((char *)path.c_str())) {
    return returnFail("BAD PATH");
  }
  File dir = SD.open((char *)path.c_str());
  path = String();
  if (!dir.isDirectory()) {
    dir.close();
    return returnFail("NOT DIR");
  }
  dir.rewindDirectory();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/json", "");
  WiFiClient client = server.client();

  server.sendContent("[");
  for (int cnt = 0; true; ++cnt) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }

    String output;
    if (cnt > 0) {
      output = ',';
    }

    output += "{\"type\":\"";
    output += (entry.isDirectory()) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += entry.name();
    output += "\"";
    output += "}";
    server.sendContent(output);
    entry.close();
  }
  server.sendContent("]");
  dir.close();
}

void handleNotFound() {
  if (hasSD && loadFromSdCard(server.uri())) {
    return;
  }
  String message = "SDCARD Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  DBG_OUTPUT_PORT.print(message);
}






//=============================================================================================
void buildXML() {
  //=============================================================================================
  RtcDateTime now = Rtc.GetDateTime();
  //RtcTemperature temp = Rtc.GetTemperature();
  float temp = 11.5;
  XML = "<?xml version='1.0'?>";
  XML += "<t>";
  XML += "<rYear>";
  XML += now.Year();
  XML += "</rYear>";
  XML += "<rMonth>";
  XML += now.Month();
  XML += "</rMonth>";
  XML += "<rDay>";
  XML += now.Day();
  XML += "</rDay>";
  XML += "<rHour>";
  if (now.Hour() < 10) {
    XML += "0";
    XML += now.Hour();
  } else {
    XML += now.Hour();
  }
  XML += "</rHour>";
  XML += "<rMinute>";
  if (now.Minute() < 10) {
    XML += "0";
    XML += now.Minute();
  } else {
    XML += now.Minute();
  }
  XML += "</rMinute>";
  XML += "<rSecond>";
  if (now.Second() < 10) {
    XML += "0";
    XML += now.Second();
  } else {
    XML += now.Second();
  }
  XML += "</rSecond>";
  XML += "<rTemp>";
  //XML+= temp.AsFloat();
  XML += "21.22";
  XML += "</rTemp>";
  XML += "</t>";
}



//=============================================================================================
void handleXML() {
  //=============================================================================================
  buildXML();
  server.send(200, "text/xml", XML);
}




void setTime() {

  Serial.println("setTime_start");

  if (hasSD && loadFromSdCard(server.uri())) {

    Serial.println("setTime_return");
    //return;
  }

  Serial.println("setTime_else");

  // Fecha--------------------------------------------------------------------
  if (server.hasArg("date")) {

    uint16_t ano;
    uint8_t mes;
    uint8_t dia;
    Serial.print("ARGdate");
    Serial.println(server.arg("date"));
    String sd = server.arg("date");
    String lastSd;
    //   int someInt = someChar - '0';
    ano = ((sd[0] - '0') * 1000) + ((sd[1] - '0') * 100) + ((sd[2] - '0') * 10) + (sd[3] - '0');
    mes = ((sd[5] - '0') * 10) + (sd[6] - '0');
    dia = ((sd[8] - '0') * 10) + (sd[9] - '0');
    if (sd != lastSd) {
      RtcDateTime now = Rtc.GetDateTime();
      uint8_t hour = now.Hour();
      uint8_t minute = now.Minute();
      Rtc.SetDateTime(RtcDateTime(ano, mes, dia, hour, minute, 0));
      lastSd = sd;
    }
    // Serial.println(fa);

    //httpServer.send ( 404 ,"text", message );
  }//if has date

  //Hora ------------------------------------------------
  if (server.hasArg("time")) {
    Serial.println(server.arg("time"));
    String st = server.arg("time");
    String lastSt;
    //   Serial.print("st ");
    //   Serial.println(st);
    //int someInt = someChar - '0';
    uint8_t hora = ((st[0] - '0') * 10) + (st[1] - '0');
    uint8_t minuto = ((st[3] - '0') * 10) + (st[4] - '0');
    if (st != lastSt) {
      RtcDateTime now = Rtc.GetDateTime();
      uint16_t year = now.Year();
      uint8_t month = now.Month();
      uint8_t day = now.Day();
      Rtc.SetDateTime(RtcDateTime(year, month, day, hora, minuto, 0));
      lastSt = st;
    }
    //server.send ( 404 ,"text", message );

  }//if has time
}

















void setup(void) {
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.setDebugOutput(true);
  DBG_OUTPUT_PORT.print("\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(ssid);

  // Wait for connection
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) {//wait 10 seconds
    delay(500);
  }
  if (i == 21) {
    DBG_OUTPUT_PORT.print("Could not connect to");
    DBG_OUTPUT_PORT.println(ssid);
    while (1) {
      delay(500);
    }
  }
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  if (MDNS.begin(host)) {
    MDNS.addService("http", "tcp", 80);
    DBG_OUTPUT_PORT.println("MDNS responder started");
    DBG_OUTPUT_PORT.print("You can now connect to http://");
    DBG_OUTPUT_PORT.print(host);
    DBG_OUTPUT_PORT.println(".local");
  }


  // RTC ---------------------------------------------------------------------------------------
  Rtc.Begin();

  if (!Rtc.GetIsRunning()) {
    Rtc.SetIsRunning(true);
  }
  RtcDateTime now = Rtc.GetDateTime();
  Rtc.SetSquareWavePin(DS1307SquareWaveOut_High);


  if (!Rtc.IsDateTimeValid())
  {
    // Common Cuases:
    //    1) the battery on the device is low or even missing and the power line was disconnected
    Serial.println("RTC lost confidence in the DateTime!");
  } else {
    RtcDateTime now = Rtc.GetDateTime();
    Serial.println("");
    printDateTime(now);

    logFileName = currentFileName(now);
    //Serial.print("fileName=");
    //Serial.println(fileName);
  }



  sensors.begin();//DS18b20


  server.on("/list", HTTP_GET, printDirectory);
  server.on("/edit", HTTP_DELETE, handleDelete);
  server.on("/edit", HTTP_PUT, handleCreate);
  server.on("/edit", HTTP_POST, []() {
    returnOK();
  }, handleFileUpload);

  server.on ( "/rtc_xml", handleXML) ;
  server.on ( "/rtc/rtc_xml", handleXML);

  server.on ( "/rtc/stime.htm", setTime);
  //server.on ( "/rtc/rtc_xml", handleXML);

  server.onNotFound(handleNotFound);

  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");

  if (SD.begin(SS)) {
    DBG_OUTPUT_PORT.println("SD Card initialized.");
    hasSD = true;
  }


  ticker.attach_ms(1000, doTemp);



}//end setup()

long c = 0;

void loop(void) {
  server.handleClient();

  /*
    c++;
    //;delay(15000);

    if (c>2000000){
    c=0;
    Serial.println("tick");
    RtcDateTime now = Rtc.GetDateTime();
    //  printDateTime(now);
    //Serial.println("rtc string end");



    sensors.requestTemperatures(); // Send the command to get temperatures
    //Serial.print(" ");
    //Serial.print(sensors.getTempCByIndex(0));
    //Serial.println("C");




    Serial.print("data string=");
    String dataString = "";
    dataString += stringDateTime(now);
    dataString += (" ");
    dataString += String(sensors.getTempCByIndex(0));
    Serial.print(dataString);
    //Serial.println("data string end");


    Serial.print("fileName=");
    Serial.println(fileName);
    myFile = SD.open(fileName, FILE_WRITE);
    if (myFile) {
      myFile.println(dataString);
      myFile.close();
      // print to the serial port too:
      //Serial.println(dataString);
    }
    // if the file isn't open, pop up an error:
    else {
      Serial.print("error opening ");
      Serial.println(fileName);
    }
    //Serial.println("fileName end");

    }
  */


}






void doTemp(void) {
  //Serial.println("tick");
  RtcDateTime now = Rtc.GetDateTime();
  
  sensors.requestTemperatures(); // Send the command to get temperatures
  
  String dataString = "";
  dataString += stringDateTime(now);
  dataString += (", ");
  dataString += String(sensors.getTempCByIndex(0));
  Serial.println(dataString);


  logFileName = currentFileName(now);

  Serial.print("logFileName=");
  Serial.println(logFileName);
  myFile = SD.open(logFileName, FILE_WRITE);
  if (myFile) {
    myFile.println(dataString);
    myFile.close();
    // print to the serial port too:
    //Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.print("error opening ");
    Serial.println(logFileName);
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
             PSTR("%04u-%02u-%02u %02u:%02u:%02u"),
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
  char fileName[20];

  snprintf_P(fileName,
             countof(fileName),
             PSTR("/logs/%04u%02u%02u.log"),
             dt.Year(),
             dt.Month(),
             dt.Day());
  return fileName;
}
