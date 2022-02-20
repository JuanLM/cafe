
#define PRES_SENSOR 14

#define TEMP_SENSOR 35

#define HEATER 33

#define PUMP 32

#define BTN_1 12
#define BTN_2 23
#define BTN_3 13

#define LED_1 18
#define LED_2 19
//#define LED_3 48

#define BOUNCE_DURATION 500 // define an appropriate bounce time in ms for your switches
//#ifdef ESP32
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "DIGIF";
const char* password = "movistar";

AsyncWebServer server(80);

volatile unsigned long bounceTime = 10; // variable to hold ms count to debounce a pressed switch



//hw_timer_t * timer = NULL;
const int iCafeTemp = 450;
const float fCafePres = 100;

const int iLecheTemp = 300;
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

typedef enum
{
  NO_COMMAND,
  BOTON_CAFE,
  BOTON_LECHE,
  BOTON_START
} Command;

State iCurrentState = REPOSO;
Command iCommand = NO_COMMAND;
float temperature (void);
//cosas wifi*******************************************************************************************
void handleRoot() {
  /*
    char temp[400];
    int sec = millis() / 1000;
    int min = sec / 60;
    int hr = min / 60;

    snprintf(temp, 400,

             "<html>\
    <head>\
      <meta http-equiv='refresh' content='5'/>\
      <title>COFFEE</title>\
      <style>\
        body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
      </style>\
    </head>\
    <body>\
      <h1>NOW IN THE COFFEE MAKER</h1>\
      <p>Uptime: %02d:%02d:%02d</p>\
      <p>Temp: %02f</p>\
      <img src=\"/test.svg\" />\
    </body>\
    </html>",

             hr, min % 60, sec % 60, temperature()
            );
    server.send(200, "text/html", temp);
    //  digitalWrite(led, 0);
    }*/

  /*void handleNotFound() {

    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
  */
}
//setup*******************************************************************************************
void setup(void)
{
  Serial.begin(115200);
  pinMode(TEMP_SENSOR, INPUT);
  pinMode(HEATER, OUTPUT);
  pinMode(PUMP, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  //  pinMode(LED_3, OUTPUT);

  pinMode(BTN_1, INPUT_PULLUP);
  pinMode(BTN_2, INPUT_PULLUP);
  pinMode(BTN_3, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/esto.html");
    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readBMP180Temperature().c_str());
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readBMP180Pressure().c_str());

  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  };
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
float pressure(void) {
  float fValue = 0.01 * analogRead(PRES_SENSOR);

  Serial.print("Pressure = ");
  Serial.print(fValue);
  Serial.println(" bars  ");

  return fValue;
}

float temperature(void) {
  float fValue = analogRead(TEMP_SENSOR);

  // Serial.print("Temp = ");
  Serial.print(fValue);
  Serial.println(" ºC  ");

  return fValue;
}
// control--------------------------------------------------------------------------------
bool controlTemp (float fTempObj) {
  if (temperature()  <  0.8 * fTempObj) {
    digitalWrite(HEATER, 1);
    return 0;
  }
  else {
    digitalWrite(HEATER, 0);
    return 1;
  }
}

bool controlPres (float fPresObj)
{
  if ( pressure() < 0.8 * fPresObj) {
    digitalWrite(PUMP, 1);
    return 0;
  }
  else {
    digitalWrite(PUMP, 0);
    return 1;
  }
}
/*LOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOoooooooooooooooooOOOOOOOOOOP*/
void loop(void)
{
  //server.handleClient();
  switch (iCurrentState) {
    case REPOSO:
      Serial.println(" Reposo  ");
      digitalWrite(HEATER, 0);
      digitalWrite(PUMP, 0);
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
        case BOTON_START: iCurrentState = ECHAR_CAFE; break;
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
        case BOTON_START: iCurrentState = ECHAR_LECHE; break;
        case BOTON_CAFE: iCurrentState = CALENTANDO_CAFE; break;
      }
      break;

    case ECHAR_CAFE:
      iCommand = NO_COMMAND;
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
      iCommand = NO_COMMAND;
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


  digitalWrite(HEATER, 0);
  digitalWrite(PUMP, 0);
  delay(1000);
}



/*
void drawGraph() {
  String out = "";
  char temp[100];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = temperature();
  for (int x = 10; x < 390; x += 10) {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);
}*/
