#include <AutoPID.h>

#include <Adafruit_MAX31865.h>
//#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>


#define PRES_SENSOR 3

#define HEATER 32

#define PUMP 35

#define BTN_1 12
#define BTN_2 33
#define BTN_3 13

#define LED_1 18
#define LED_2 19
//#define LED_3 48

//pid settings and gains
#define OUTPUT_MIN 0
#define OUTPUT_MAX 255
#define KP .12
#define KI .0003
#define KD 0

#define RREF 430

#define BOUNCE_DURATION 1000 // define an appropriate bounce time in ms for your switches
//#ifdef ESP32
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>


char storedSSID[20] = "";//size appropriately
char storedPassword[20] = "";//size appropriately
AsyncWebServer server(80);

volatile unsigned long bounceTime = 10; // variable to hold ms count to debounce a pressed switch



//hw_timer_t * timer = NULL;
 int iCafeTemp = 100;
const float fCafePres = 100;

 int iLecheTemp = 120;
const float fLechePres = 9;

bool bLecheTemp = 0;
bool bCafeTemp = 0;

typedef enum
{
  REPOSO,
  CALENTANDO_CAFE,
  CALENTANDO_LECHE,
  CAFE_CALIENTE,
  LECHE_CALIENTE,
  ECHAR_CAFE,
  ECHAR_LECHE,
} State;
const char* stateStr[] = {"Reposo",  
"CALENTANDO_CAFE",
"CALENTANDO_LECHE",
 "CAFE_CALIENTE",
  "LECHE_CALIENTE",
  "ECHAR_CAFE",
  "ECHAR_LECHE"}; 

typedef enum
{
  NO_COMMAND,
  BOTON_CAFE,
  BOTON_LECHE,
  BOTON_START
} Command;


State iCurrentState = REPOSO;
Command iCommand = NO_COMMAND;



//pid
//pid settings and gains
#define OUTPUT_MIN 0
#define OUTPUT_MAX 1
#define KP .12
#define KI .0003
#define KD 0
double  Dtemperature;
double setPoint=iCafeTemp;
double outputVal;
AutoPID myPID(&Dtemperature, &setPoint, &outputVal, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

unsigned long lastTempUpdate; 

//cosas wifi*******************************************************************************************
IPAddress local_IP(192, 168, 25, 129);
// Set your Gateway IP address
IPAddress gateway(192, 168, 25, 1);

IPAddress subnet(255, 255, 255, 0);
//setup*******************************************************************************************
void setup(void)
{
  Serial.begin(115200);

  pinMode(HEATER, OUTPUT);
  pinMode(PUMP, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  //  pinMode(LED_3, OUTPUT);

  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP);
// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 pt = Adafruit_MAX31865(27, 14, 25, 26);
  //if temperature is more than 4 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  myPID.setBangBang(4);
  //set PID update interval to 4000ms
  myPID.setTimeStep(300);

 
pt.begin(MAX31865_3WIRE);
  WiFi.mode(WIFI_STA);
// aqui lo del OTA...

InitOTA();


//doc7umentosssss

  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  };
  //abrir archivo credenciales

  memcpy(storedSSID, readFile(SPIFFS, "/SSID.txt").c_str(), strlen(readFile(SPIFFS, "/SSID.txt").c_str()) + 1);
  memcpy(storedPassword, readFile(SPIFFS, "/Password.txt").c_str(), strlen(readFile(SPIFFS, "/Password.txt").c_str()) + 1);
  Serial.println(storedSSID);
  //---------------------
  WiFi.begin(storedSSID, storedPassword);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(storedSSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());

  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/esto.html");
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", temperature().c_str());
  });
   server.on("/status", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", stateStr[iCurrentState]);
   }); 
//  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest * request) {
//    request->send_P(200, "text/plain", pressure().c_str());
//  });

  //  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  attachInterrupt(digitalPinToInterrupt(BTN_1), cafe, FALLING); // trigger when button pressed, but not when released.
  attachInterrupt(digitalPinToInterrupt(BTN_2), leche, FALLING); // trigger when button pressed, but not when released.
  attachInterrupt(digitalPinToInterrupt(BTN_3), start, FALLING); // trigger when button pressed, but not when released.




}
// botonessssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
void cafe() {
  if (millis() > bounceTime) {
    iCommand = BOTON_CAFE;
    Serial.println(" BOTON_CAFE  ");
    bounceTime = millis() + BOUNCE_DURATION; // set whatever bounce time in ms is appropriate
  }
}

void leche() {
  if (millis() > bounceTime) {
    iCommand = BOTON_LECHE;
    Serial.println(" BOTON_LECHE  ");
    bounceTime = millis() + BOUNCE_DURATION; // set whatever bounce time in ms is appropriate
  }
}

void start() {
  if (millis() > bounceTime) {
    iCommand = BOTON_START;
    Serial.println(" BOTON_STAR  ");
    bounceTime = millis() + BOUNCE_DURATION; // set whatever bounce time in ms is appropriate
  }
}
// analog read-----------------------------------------------------------------------------
String pressure(void) {

  //Serial.print("p ");
  // Serial.println(analogRead(PRES_SENSOR)*3.3/4095);
 // return String(analogRead(PRES_SENSOR) / 4095 * 200 * 0.06894757);

}

String temperature(void) {


     Adafruit_MAX31865 pt = Adafruit_MAX31865(27, 14, 25, 26);

 uint16_t rtd = pt.readRTD();
   float ratio = rtd;
  ratio /= 32768;
  
  Dtemperature= pt.temperature(100, RREF);
    
 return String(Dtemperature);

}
// control--------------------------------------------------------------------------------
bool controlTemp (float fTempObj) {
 



  myPID.run(); //call every loop, updates automatically at certain time interval
  digitalWrite(HEATER, outputVal);
 return (myPID.atSetPoint(1));
 
  /*if (temperature().toFloat()  <  0.88 * fTempObj) {
    digitalWrite(HEATER, HIGH);
    Serial.print(" heater  ");Serial.println("1");
    return 0;
  }
  else {
    digitalWrite(HEATER, 0);
     Serial.print(" heater  ");Serial.println("0");
    return 1;
  }*/
}

bool controlPres (float fPresObj)
{
  if ( pressure().toFloat() < 0.8 * fPresObj) {
    digitalWrite(PUMP, 1);
    return 0;
  }
  else {
    digitalWrite(PUMP, 0);
    return 1;
  }
}//controlPress
/*LOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOoooooooooooooooooOOOOOOOOOOP*/
void loop(void)
{
  //server.handleClient();
  switch (iCurrentState) {
    case REPOSO:
      Serial.println(" Reposo  ");
      digitalWrite(HEATER, 0);
      digitalWrite(PUMP, 0);
       ArduinoOTA.handle();
      switch (iCommand) {
        case BOTON_CAFE:
          iCurrentState = CALENTANDO_CAFE;
          break;
        case BOTON_LECHE:
          iCurrentState = CALENTANDO_LECHE;
          break;
      }

      break;
    case CALENTANDO_CAFE:
      digitalWrite(PUMP, 0);
      Serial.println(" CALENTANDO_CAFE:  ");
      bCafeTemp = controlTemp(iCafeTemp);
      digitalWrite(LED_1, bCafeTemp);
      if (bCafeTemp == 0) {
        Serial.println("Heating ");
      }
      else {
        Serial.println("Cafe Temp reached, press start");
        iCurrentState = CAFE_CALIENTE;
        break;
      }
      switch (iCommand) {
        case BOTON_START: iCurrentState = REPOSO; break;
        case BOTON_LECHE: iCurrentState = CALENTANDO_LECHE; break;
      }
      digitalWrite(LED_1, false);
      break;

    case CALENTANDO_LECHE:
      digitalWrite(PUMP, 0);
      Serial.println(" CALENTANDO_LECHE:  ");
      bLecheTemp = controlTemp(iLecheTemp);
      digitalWrite(LED_2, bLecheTemp);
      if (bLecheTemp == 0) Serial.println("Heating");
      else {
        Serial.println("Leche temp reached, press start");
        iCurrentState = LECHE_CALIENTE;
        break;

      }
      switch (iCommand) {
        case BOTON_START: iCurrentState = REPOSO; break;
        case BOTON_CAFE: iCurrentState = CALENTANDO_CAFE; break;
      }
      break;
    case CAFE_CALIENTE:
      digitalWrite(PUMP, 0);
      Serial.println(" CAFE_CALIENTE:  ");

      bCafeTemp = controlTemp(iCafeTemp);
      digitalWrite(LED_2, bCafeTemp);
      if (bCafeTemp == 0) {
        iCurrentState = CALENTANDO_CAFE;
        break;
      }
      switch (iCommand) {
        case BOTON_START: iCurrentState = ECHAR_CAFE; iCommand = NO_COMMAND; break;
        case BOTON_LECHE: iCurrentState = CALENTANDO_LECHE; break;
      }
      break;

    case LECHE_CALIENTE:
      digitalWrite(PUMP, 0);
      Serial.println(" LECHE_CALIENTE:  ");
      bLecheTemp = controlTemp(iLecheTemp);
      digitalWrite(LED_2, bLecheTemp);
      if (bLecheTemp == 0) {
        iCurrentState = CALENTANDO_LECHE;
        break;
      }
      switch (iCommand) {
        case BOTON_START: iCurrentState = ECHAR_LECHE; iCommand = NO_COMMAND; break;
        case BOTON_CAFE: iCurrentState = CALENTANDO_CAFE; break;
      }
      break;

    case ECHAR_CAFE:
       digitalWrite(PUMP, 1);
        bCafeTemp = controlTemp(iCafeTemp);
      Serial.println(" ECHAR_CAFE:  ");
      switch (iCommand) {
        case BOTON_LECHE:
          iCurrentState = CALENTANDO_LECHE;
          break;
        case BOTON_START:
          iCurrentState = REPOSO;
          break;
      }
      digitalWrite(LED_1, controlTemp(iCafeTemp));
      controlPres(fCafePres);
      break;
    case ECHAR_LECHE:

      Serial.println(" ECHAR_LECHE:  ");
      switch (iCommand) {
        case BOTON_CAFE:
          iCurrentState = CALENTANDO_CAFE;
          break;
        case BOTON_START:
          iCurrentState = REPOSO;
          break;
      }
      digitalWrite(LED_2, controlTemp(iLecheTemp));
      controlPres(fLechePres);
      delay(1000);
      break;

  }


 // digitalWrite(HEATER, 0);
  digitalWrite(PUMP, 0);
  delay(1000);
}



void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

//------ SPIFFS file access procedures ------
String readFile(fs::FS &fs, const char * path) {
  //Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    Serial.printf("%s\r\n empty file or failed to open file", path);
    return String();
  }
  //Serial.println("- read from file:");
  String fileContent;
  while (file.available()) {
    fileContent += String((char)file.read());
  }

  return fileContent;
}

//ota
void InitOTA()
{
// Port defaults to 8266
// ArduinoOTA.setPort(8266);

// Hostname defaults to esp8266-[ChipID]
// ArduinoOTA.setHostname(hostname);

// No authentication by default
// ArduinoOTA.setPassword("admin");

// Password can be set with it's md5 value as well
// MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
// ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

ArduinoOTA.onStart([]() {
   String type;
   if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
   } else { // U_SPIFFS
      type = "filesystem";
   }

   // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
   Serial.println("Start updating " + type);
});

ArduinoOTA.onEnd([]() {
   Serial.println("\nEnd");
});

ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
   Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
});

ArduinoOTA.onError([](ota_error_t error) {
   Serial.printf("Error[%u]: ", error);
   if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
   } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
   } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
   } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
   } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
   }
});

ArduinoOTA.begin();
Serial.println("");
Serial.println("OTA iniciado");
}
