#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <Adafruit_HTU21DF.h>
#include <EEPROM.h>
#include <SimpleTimer.h>
#include <ping.h>
 
typedef struct {
  const char *ssid;
  const char *password;
  const char *shortname;
}aps_t;

const aps_t *myaps = NULL;

const aps_t aps[] = {
  {
  .ssid = "xxxx",
  .password = "xxxxx",
  .shortname = "esp8266-xxx"
  },
  {
  .ssid = "yyyyyy",
  .password = "yyyyy",
  .shortname = "esp8266-yyyy"
  },
};


MDNSResponder mdns;

ESP8266WebServer server(80);
SimpleTimer timer;
int configTimerId = -1;


typedef struct {
  bool autocontrol;
  bool manual;
  int     desiredval;
  int     validMagic;
}t_settings;

t_settings settings;

#define VALIDMAGIC  0xcccc
#define WATCHCOUNT  10
const t_settings ref_settings = {
  .autocontrol = false,
  .manual = false,
  .desiredval = 60,
  .validMagic = VALIDMAGIC
};

int watch_count = WATCHCOUNT;

float lastTemp = 0.0;
float lastHum = 0.0;

bool serverLock = false;

const float hist = 0.5;

#define SDA_PIN 2
#define SCL_PIN 0
#define RELAY_PIN 4
#define CONTAINER_PIN 16


Adafruit_HTU21DF htu = Adafruit_HTU21DF(SDA_PIN,SCL_PIN);

void homePage();
void mainCss();
void mainJs();
void listJson();
void controlFun();
void wifiwatch();
void saveEEPROM();
bool isContainerFull();

void setup(void){
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, 0);
  Serial.begin(115200);

  pinMode(CONTAINER_PIN, INPUT);

  
  EEPROM.begin(sizeof(t_settings));
  for(int ectr = 0; ectr < sizeof(t_settings);ectr++) {
    uint8 tmp_8 = EEPROM.read(ectr);
    (reinterpret_cast<uint8*>(&settings))[ectr] = tmp_8;
  }
  if(settings.validMagic != VALIDMAGIC) {
    settings = ref_settings;
    Serial.println("Settings restored from reference");
  }else {
    Serial.println("Settings restored from EEPROM");
  }
  
  Serial.println("EEPROM values:");
  Serial.print("autocontrol: ");
  Serial.println(settings.autocontrol);
  Serial.print("manual: ");
  Serial.println(settings.manual);
  Serial.print("desiredval: ");
  Serial.println(settings.desiredval);

  bool foundWifi  = false;
  int foundctr = 20;
  while(!foundWifi) {
    int8_t numAPs = WiFi.scanNetworks();
    for(int8_t idxAP = 0; (idxAP < numAPs) && (!foundWifi); idxAP ++ ) {
      const char* foundAP = WiFi.SSID(idxAP);
      Serial.print("Searching for known AP with name '");
      Serial.print(foundAP);
      Serial.println("'.");
      for(int idxS = 0; idxS < (sizeof(aps) / sizeof(aps_t));idxS++) {
        Serial.print(".");
        if(!strcmp(foundAP,aps[idxS].ssid)) {
          Serial.println("SSID match!");
          myaps = &aps[idxS];
          WiFi.begin(myaps->ssid, myaps->password);
          foundWifi = true;          
          break;
        }
      }
    }
    /* We try for 20 sec then enable the POWER and mocking a standard dehumidifier without intelligence */
    if(foundctr) {
      foundctr--;      
    }else {
      digitalWrite(RELAY_PIN, 1);
    }
    delay(1000);
  }
  //WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected.");
//  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (mdns.begin(myaps->shortname)) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/", homePage);
  server.on("/main.css", mainCss);
  server.on("/main.js", mainJs);
  server.on("/list.json", listJson);
  server.on("/control.cgi", controlCgi);
  
 
  server.begin();
  Serial.println("HTTP server started");

  if (!htu.begin()) {
   Serial.println("Couldn't find sensor!");
  }
  configTimerId = timer.setInterval(15000, saveEEPROM);
  timer.disable(configTimerId);
  timer.setInterval(1000, controlFun);
  timer.setInterval(30000,wifiwatch);
}

void loop(void){
  server.handleClient();
  yield();
  timer.run();
} 

void homePage() { /* http://www.miniwebtool.com/html-compressor/ */
    String message;
    message  = F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1' /><title>Dehumidifier</title>");
    message += F("<link rel=\"stylesheet\" href=\"http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.css\"><link rel='stylesheet' href='main.css' />");
    message += F("<script src=\"http://code.jquery.com/jquery-1.11.1.min.js\"></script><script src=\"http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.js\"></script>");
    message += F("<script src='//stevenlevithan.com/assets/misc/date.format.js'></script><script src='main.js'></script></head><body><div id='main' data-role='page'>");
    message += F("<div data-role='header' data-position='fixed'><h3>Temperature and Humidity data</h3></div><div data-role='content'>");
    message += F("<ul id='list' data-role='listview' data-inset='true'></ul><div class=\"ui-field-contain\"><label for=\"autocontrol\">Auto control:</label>");
    message += F("<select name=\"autocontrol\" id=\"autocontrol\" data-role=\"flipswitch\"><option value=\"off\">Off</option><option value=\"on\">On</option></select>");
    message += F("</div><div class=\"ui-field-contain\"><label for=\"mancontrol\">Manual control:</label><select name=\"mancontrol\" id=\"mancontrol\" data-role=\"flipswitch\">");
    message += F("<option value=\"off\">Off</option><option value=\"on\">On</option></select></div><div class=\"ui-field-contain\"><label for=\"water-container\">Water container:</label>");
    message += F("<select name=\"water-container\" id=\"water-container\" disabled = \"disabled\" data-role=\"flipswitch\"><option value=\"off\">OK</option><option value=\"on\">Full</option></select>");
    message += F("</div><form><label for=\"humset\">Desired humidity:</label><input name=\"humset\" id=\"humset\" min=\"0\" max=\"100\" type=\"range\"></form><p id='info'></p></div></div></body></html>");
    server.send(200, "text/html", message);
}

void mainCss() {  /* http://cssminifier.com/ */
  	String message;
    message += F(".ui-li-aside,.ui-li-aside>sup{font-size:x-large}.ui-li-aside{font-weight:700}#info{margin-top:10px;text-align:center;font-size:small}.custom-size-flipswitch.ui-flipswitch ");
    message += F(".ui-btn.ui-flipswitch-on{text-indent:-3.9em}.custom-size-flipswitch.ui-flipswitch ");
    message += F(".ui-flipswitch-off{text-indent:1em}.custom-size-flipswitch.ui-flipswitch{width:7.875em}.custom-size-flipswitch.ui-flipswitch.ui-flipswitch-active{padding-left:6em;width:1.875em}");
    message += F("@media (min-width:28em){.ui-field-contain>label+.custom-size-flipswitch.ui-flipswitch{width:1.875em}}");
    server.send(200, "text/css", message);
}

void mainJs() { /*  http://jscompress.com/ */
  	String message;
    message  = F("function processJson(){$.getJSON(\"./list.json\",function(t){var e=[];e.push('<li><a><h3>Temperature</h3><p class=\"ui-li-aside\"><sup>'+t.temp.toFixed(1)+'&deg;");
    message += F("C</sup></p></a></li><li><a><h3>Humidity</h3><p class=\"ui-li-aside\"><sup>'+t.hum.toFixed(1)+\"&#37</sup></p></a></li>\"),");
    message += F("$(\"#list\").html(e.join(\"\")).trigger(\"create\").listview(\"refresh\"),$(\"#info\").html(\"Uptime: \"+toTime(t.uptime)+\" (\"+t.free+\" bytes free)\"),");
    message += F("$(\"#autocontrol\").val(t.auto).flipswitch(\"refresh\"),$(\"#mancontrol\").val(t.manual).flipswitch(\"refresh\"),$(\"#water-container\").val(t.full).flipswitch(\"refresh\"),");
    message += F("$(\"#humset\").val(t.desired).slider(\"refresh\"),jsonLoaded=!0}),setTimeout(processJson,5e3)}function addnotify(){this.notify=function()");
    message += F("{jsonLoaded&&$.post(\"control.cgi\",{manual:$(\"#mancontrol\").val(),auto:$(\"#autocontrol\").val(),desired:$(\"#humset\").val()})},");
    message += F("$(\"#autocontrol\").change(function(){\"on\"===$(\"#autocontrol\").val()?($(\"#mancontrol\").flipswitch(\"disable\"),");
    message += F("$(\"#humset\").slider(\"enable\")):($(\"#mancontrol\").flipswitch(\"enable\"),$(\"#humset\").slider(\"disable\")),notify()}),$(\"#mancontrol\").change(notify),");
    message += F("$(\"#humset\").on(\"slidestop\",notify)}function toTime(t){var e=parseInt(t),o=\"\";this.toStr=function(t){var e=\"\";return 10>t&&(e=\"0\"),e+t.toString()};");
    message += F("var i=e%1e3;e-=i,e/=1e3;var n=e%60;e-=n,e/=60;var s=e%60;e-=s;var a=e/=60;return o=toStr(a)+\":\"+toStr(s)+\":\"+toStr(n)}$(document).ready(function()");
    message += F("{processJson(),addnotify()});var jsonLoaded=!1;");
    server.send(200, "application/javascript", message);
}


void controlCgi() {
    bool ret = false;
    noInterrupts();   //lock needed to avoid ESP8266 crash when more than one clients are trying to acccess the page
    if (serverLock) {
      ret = true;
    }else {
      serverLock = true;
    }
    interrupts();
    if(ret) return;
    
    settings.manual = server.arg("manual") == "on" ? true:false;
    settings.autocontrol = server.arg("auto")  == "on" ? true:false;
    settings.desiredval  = server.arg("desired").toInt();

    for(int ectr = 0; ectr < sizeof(t_settings);ectr++) {
        EEPROM.write(ectr,(reinterpret_cast<uint8*>(&settings))[ectr]);
    }
    timer.enable(configTimerId);
    if(settings.manual) {
        digitalWrite(RELAY_PIN, 1);
    }else {
        digitalWrite(RELAY_PIN, 0);
    }
    String message;
    message  = F("<!DOCTYPE html><html><body>OK</body></html>");
    server.send(200, "text/html", message);
    noInterrupts();
    serverLock = false;
    interrupts();
}

void listJson() {
  	String message;
    message  = F("{");
    message += F("    \"temp\":");
    message += String(lastTemp);
    message += F(",");
    message += F("  \"hum\":");
    message += String(lastHum);
    message += F(",");
    message += F("  \"uptime\":");
    message += String(millis());
    message += F(",");
    message += F("  \"free\":");
    message += String(ESP.getFreeHeap());
    message += F(",");
    message += F("  \"auto\":");
    message += settings.autocontrol == true ? "\"on\"" : "\"off\"";
    message += F(",");
    message += F("  \"manual\":");
    message += settings.manual  == true ? "\"on\"" : "\"off\"";
    message += F(",");
    message += F("  \"desired\":");
    message += String(settings.desiredval);
    message += F(",");
    message += F("  \"full\":");
    message += isContainerFull() ? "\"on\"" : "\"off\"";
    message += F("}");
    server.send(200, "application/json", message);
    timer.restartTimer(configTimerId);
}

void controlFun() {
    lastTemp = htu.readTemperature();
    yield();
    lastHum = htu.readHumidity();
    yield();
    if(settings.autocontrol) {
        float tmpDesired = (float)settings.desiredval;
        if((lastHum-tmpDesired) > hist) {
            digitalWrite(RELAY_PIN, 1);
            settings.manual = true;
        }else {
            digitalWrite(RELAY_PIN, 0);
            settings.manual = false;      
        }
    }
}

void saveEEPROM() {
  timer.disable(configTimerId);
  EEPROM.commit();
  noInterrupts();
  serverLock = false;
  interrupts();
  yield();
}


bool isContainerFull() {
  return (!digitalRead(CONTAINER_PIN));
}


void wifiwatch(void) {
  if(WiFi.status() != WL_CONNECTED) {
    watch_count--;
  }else {
    watch_count = WATCHCOUNT;
  }
  if(!watch_count) {
    ESP.restart();
  }
}

