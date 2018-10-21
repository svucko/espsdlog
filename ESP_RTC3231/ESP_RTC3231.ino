// CONNECTIONS:
// DS3231 SDA --> SDA=============nodemcu D1
// DS3231 SCL --> SCL=============nodemcu D2
// DS3231 VCC --> 3.3v or 5v
// DS3231 GND --> GND

#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS1307.h>
RtcDS1307<TwoWire> Rtc(Wire);

const char* host = "esp8266";
const char* ssid = "SV";
const char* password = "deadbeef";

char datestring[20];
String message,javaScript,XML;
    
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

RtcDateTime now;

//=============================================================================================
void buildJavascript(){
//=============================================================================================
  javaScript="<SCRIPT>\n";
  javaScript+="var xmlHttp=createXmlHttpObject();\n";

  javaScript+="function createXmlHttpObject(){\n";
  javaScript+=" if(window.XMLHttpRequest){\n";
  javaScript+="    xmlHttp=new XMLHttpRequest();\n";
  javaScript+=" }else{\n";
  javaScript+="    xmlHttp=new ActiveXObject('Microsoft.XMLHTTP');\n";// code for IE6, IE5
  javaScript+=" }\n";
  javaScript+=" return xmlHttp;\n";
  javaScript+="}\n";

  javaScript+="function process(){\n";
  javaScript+=" if(xmlHttp.readyState==0 || xmlHttp.readyState==4){\n";
  javaScript+="   xmlHttp.open('PUT','xml',true);\n";
  javaScript+="   xmlHttp.onreadystatechange=handleServerResponse;\n"; // no brackets?????
  javaScript+="   xmlHttp.send(null);\n";
  javaScript+=" }\n";
  javaScript+=" setTimeout('process()',1000);\n";
  javaScript+="}\n";
 
  javaScript+="function handleServerResponse(){\n";
  javaScript+=" if(xmlHttp.readyState==4 && xmlHttp.status==200){\n";
  javaScript+="   xmlResponse=xmlHttp.responseXML;\n";
  
  javaScript+="   xmldoc = xmlResponse.getElementsByTagName('rYear');\n";
  javaScript+="   message = xmldoc[0].firstChild.nodeValue;\n";
  javaScript+="   document.getElementById('year').innerHTML=message;\n";
  
  javaScript+="   xmldoc = xmlResponse.getElementsByTagName('rMonth');\n";
  javaScript+="   message = xmldoc[0].firstChild.nodeValue;\n";
  javaScript+="   document.getElementById('month').innerHTML=message;\n";
  
  javaScript+="   xmldoc = xmlResponse.getElementsByTagName('rDay');\n";
  javaScript+="   message = xmldoc[0].firstChild.nodeValue;\n";
  javaScript+="   document.getElementById('day').innerHTML=message;\n";
  
  javaScript+="   xmldoc = xmlResponse.getElementsByTagName('rHour');\n";
  javaScript+="   message = xmldoc[0].firstChild.nodeValue;\n";
  javaScript+="   document.getElementById('hour').innerHTML=message;\n";
  
  javaScript+="   xmldoc = xmlResponse.getElementsByTagName('rMinute');\n";
  javaScript+="   message = xmldoc[0].firstChild.nodeValue;\n";
  javaScript+="   document.getElementById('minute').innerHTML=message;\n";
  
  javaScript+="   xmldoc = xmlResponse.getElementsByTagName('rSecond');\n";
  javaScript+="   message = xmldoc[0].firstChild.nodeValue;\n";
  javaScript+="   document.getElementById('second').innerHTML=message;\n";
  
  javaScript+="   xmldoc = xmlResponse.getElementsByTagName('rTemp');\n";
  javaScript+="   message = xmldoc[0].firstChild.nodeValue;\n";
  javaScript+="   document.getElementById('temp').innerHTML=message;\n";
 
  javaScript+=" }\n";
  javaScript+="}\n";
  javaScript+="</SCRIPT>\n";
}
//=============================================================================================
void buildXML(){
//=============================================================================================
  RtcDateTime now = Rtc.GetDateTime();
  //RtcTemperature temp = Rtc.GetTemperature();
  float temp=11.5;
  XML="<?xml version='1.0'?>";
  XML+="<t>"; 
    XML+="<rYear>";
    XML+=now.Year();
    XML+="</rYear>";
    XML+="<rMonth>";
    XML+=now.Month();
    XML+="</rMonth>";
    XML+="<rDay>";
    XML+=now.Day();
    XML+="</rDay>";
    XML+="<rHour>";
      if(now.Hour()<10){
        XML+="0";
        XML+=now.Hour();
      }else{    XML+=now.Hour();}
    XML+="</rHour>";
    XML+="<rMinute>";
      if(now.Minute()<10){
        XML+="0";
        XML+=now.Minute();
      }else{    XML+=now.Minute();}
    XML+="</rMinute>";
    XML+="<rSecond>";
      if(now.Second()<10){
        XML+="0";
        XML+=now.Second();
      }else{    XML+=now.Second();}
    XML+="</rSecond>";
    XML+="<rTemp>";
    //XML+= temp.AsFloat();
    XML+= "21.22";
    XML+="</rTemp>";
  XML+="</t>"; 
}
//=============================================================================================
void handleRoot() {
//=============================================================================================
  buildJavascript();
  IPAddress ip = WiFi.localIP();
  String ipStr = (String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]));

// message="<!DOCTYPE HTML>\n";
  message = "<html>";
//  message = "<html><head><meta http-equiv='refresh' content='5'/>";
  message = "<head>";
  message = javaScript;
  message += "<title>ESP8266 Demo</title>";
  message += "<style> body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }";
  message += "h1 {text-align:center;}";
  message += "h2 {text-align:center;}";
  message += "p {text-align:center;}";
  message += "table.center { width:80%; margin-left:10%; margin-right:10%;}";
  message += "</style>";
  message += "  </head>";
  message += "  <body onload='process()'>";
  message += "<table class='center'>";
  message += "  <tr>";
  message += "    <th>";
  message += "<h1>Fecha y Hora</h1>";
  message += "    </th> ";
  message += "  </tr>";
  message += "  <tr>";
  message += "    <td align='center'>";
  message += "    </td>";
  message += "  </tr>";

  
  message += "  <tr>";
  message += "    <td align='center'>";
  message += "<A id='year'></A>/<A id='month'></A>/<A id='day'></A>   <A id='hour'></A>:<A id='minute'></A>:<A id='second'></A><BR>";
  message += "    </td>";
  message += "  </tr>";
  
  message += "  <tr>";
  message += "    <td align='center'>";
  message += "Temp =<A id='temp'></A>C<BR>";
  message += "    </td>";
  message += "  </tr>";

  message += "  <tr>";
  message += "    <td>";
  message += "<H2><a href='/setTime'>Cambiar fecha y hora</a></H2>";
  message += "    </td>";
  message += "  </tr>";

  message += "  <tr>";
  message += "    <td align='center'>";
  message += "<BR>IP  ";
  message += ipStr;
  message += "    </td>";
  message += "  </tr>";
  message += "</table>";
 
  message += "<BR>";

  message += "";
 
  message += "</body></html>";

  httpServer.send ( 404 ,"text/html", message );
}

//=============================================================================================
void setTime() {
//=============================================================================================
   buildJavascript();
// message="<!DOCTYPE HTML>\n";
  message = "<html>";
//  message = "<html><head><meta http-equiv='refresh' content='5'/>";
  message = "<head>";
  message = javaScript;
  message += "<title>ESP8266 set time</title>";
  message += "<style> body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }";
  message += "h1 {text-align:center;}";
  message += "h2 {text-align:center;}";
  message += "p {text-align:center;}";
  message += "table.center { width:80%; margin-left:10%; margin-right:10%;}";
  message += "</style>";
  message += "  </head>";
  message += "  <body onload='process()'>";/////////////////////////////////////////

  message += "";
  
  message += "<table class='center'>";
  message += "  <tr>";
  message += "    <th>";
  message += "<h1>Cambiar fecha y hora</h1>";
  message += "    </th> ";
  message += "  </tr>";
  message += "  <tr>";
  message += "    <td align='center'>";
  message += "Fecha Atual  ";
  message += " <BR>   </td>";
  message += "  </tr>";
  message += "  <tr>";
  message += "    <td align='center'>";
  message += "<A id='year'></A>/<A id='month'></A>/<A id='day'></A><BR>";
  message += "    </td>";
  message += "  </tr>";
  message += "  <tr>";
  message += "    <td align='center'>";

  message += "<form >";
  message += "Pon fecha posterior a 2017-03-20<br><br>";
  message += "<input type='date' name='date' min='2017-03-20' style='height:75px; width:200px'><br><br>";
  message += "<input type='submit' value='Actualizar Fecha' style='height:75px; width:200px'> ";
  message += "</form>";
  

  message += "    </td>";
  message += "  </tr>";
  message += "  <tr>";
  message += "    <td align='center'>";
  message += "Hora Actal<BR><A id='hour'></A>:<A id='minute'></A>:<A id='second'></A><BR><BR>";
  message += "    </td>";
  message += "  </tr>";
  message += "  <tr>";
  message += "    <td align='center'>";
  message += "<form >";  
  message += "Nueva hora<br>";
  message += "<input type='TIME' name='time' style='height:75px; width:200px'><br><br>";
  message += "<input type='submit' value='Actualizar Hora' style='height:75px; width:200px'> ";
  message += "</form>";
  message += "    </td>";
  message += "  </tr>";
  message += "  <tr>";
  message += "    <td>";
  message += "<H2><a href='/'>ATRAS</a></H2>";
  message += "    </td>";
  message += "  </tr>";
  message += "</table>";


  message += "";
  
  message += "</body></html>";

  httpServer.send ( 404 ,"text/html", message );
  
// Fecha--------------------------------------------------------------------  
  if (httpServer.hasArg("date")) {
    
    uint16_t ano;
    uint8_t mes;
    uint8_t dia;
    Serial.print("ARGdate");
    Serial.println(httpServer.arg("date"));
    String sd=httpServer.arg("date");
    String lastSd;
//   int someInt = someChar - '0';
     ano = ((sd[0]-'0')*1000)+((sd[1]-'0')*100)+((sd[2]-'0')*10)+(sd[3]-'0');
     mes = ((sd[5]-'0')*10)+(sd[6]-'0');
     dia = ((sd[8]-'0')*10)+(sd[9]-'0');
    if (sd != lastSd){
      RtcDateTime now = Rtc.GetDateTime();
      uint8_t hour = now.Hour();
      uint8_t minute = now.Minute();
      Rtc.SetDateTime(RtcDateTime(ano, mes, dia, hour, minute, 0));
      lastSd=sd;}
  // Serial.println(fa);

   httpServer.send ( 404 ,"text", message );
}//if has date
// Hora ------------------------------------------------
  if (httpServer.hasArg("time")) {
    Serial.println(httpServer.arg("time"));
    String st=httpServer.arg("time");
    String lastSt;
 //   Serial.print("st ");
 //   Serial.println(st);
    //int someInt = someChar - '0';
    uint8_t hora = ((st[0]-'0')*10)+(st[1]-'0');
    uint8_t minuto = ((st[3]-'0')*10)+(st[4]-'0');
    if (st != lastSt){
       RtcDateTime now = Rtc.GetDateTime();
       uint16_t year = now.Year();
       uint8_t month = now.Month();
       uint8_t day = now.Day();
       Rtc.SetDateTime(RtcDateTime(year, month, day, hora, minuto, 0));
       lastSt=st;}
    httpServer.send ( 404 ,"text", message );

  }//if has time
}
//=============================================================================================
void handleXML(){
//=============================================================================================
  buildXML();
  httpServer.send(200,"text/xml",XML);
}
//=============================================================================================
void handleNotFound() {
//=============================================================================================
 String message = "<html><head>";
  message += "<title>ESP8266 Not Found</title>";
  message += "<style> body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }";
  message += "h1 {text-align:center;}";
  message += "h2 {text-align:center;}";
  message += "</style>";
  message += "  </head>";
  message += "  <body>";
  message += "<table style='width:80%'>";
  message += "<tr>";//fila
  message += "<th>";//columna encavezado
  message += "<h1>Not Found</h1>";
  message += "</th>";
  message += "<tr>";//fila 2
  message += "<td>";//columna normal
  message += "<H2><a href='/'>Back</a></H2>";
  message += "<td>";
  message += "</tr>";
  message += "</table>";
  message += "</body></html>";
  message += "";
  
  httpServer.send ( 404 ,"text", message );
}

//=============================================================================================
void setup(void){
//=============================================================================================

  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  while(WiFi.waitForConnectResult() != WL_CONNECTED){
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }
  if ( MDNS.begin ( "esp8266" ) ) {
    Serial.println ( "MDNS responder started" );
  }
  //MDNS.begin(host);

  httpUpdater.setup(&httpServer);
  
  
  httpServer.on ( "/", handleRoot );
  httpServer.on ( "/setTime", setTime );
  httpServer.on ( "/xml", handleXML) ;


  httpServer.onNotFound ( handleNotFound );

  Serial.println ( "HTTP server started" );

  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);

  Serial.print(WiFi.localIP());

// RTC ---------------------------------------------------------------------------------------
    Rtc.Begin();

    // if you are using ESP-01 then uncomment the line below to reset the pins to
    // the available pins for SDA, SCL
    //Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

//   RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    if (!Rtc.GetIsRunning()){
   //     Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);}
    //RtcDateTime now = Rtc.GetDateTime();
    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    //Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePin(DS1307SquareWaveOut_High); 
}

//=============================================================================================
void loop(void){
//=============================================================================================


// RTC ---------------------------------------------------------------------------------------
 //   RtcDateTime now = Rtc.GetDateTime();
 //   RtcTemperature temp = Rtc.GetTemperature();
  httpServer.handleClient();
}
