#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ThingSpeak.h>
#include <TinyGPS++.h>

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

ESP8266WebServer server(80);
WiFiClient client;

unsigned long myChannelNumber = 2278350;
const char* myWriteAPIKey = "YourWriteAPIKey";
const char* myReadAPIKey = "YourReadAPIKey";

uint8_t LED1pin = D0;
bool LED1status = LOW;
double led1Latitude = 0.0;
double led1Longitude = 0.0;
int ldrPin1 = A0;
int val1;
int voltageThreshold = 500;

TinyGPSPlus gps;

const char* serverSSID = "NodeMCU";
const char* serverPassword = "12345678";

IPAddress serverLocal_ip(192, 168, 1, 1);
IPAddress serverGateway(192, 168, 1, 1);
IPAddress serverSubnet(255, 255, 255, 0);

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(LED1pin, OUTPUT);
  pinMode(ldrPin1, INPUT);
  WiFi.softAP(serverSSID, serverPassword);
  WiFi.softAPConfig(serverLocal_ip, serverGateway, serverSubnet);
  delay(100);

  Serial.println("HTTP server started");
  Serial.print("Connect to Wi-Fi network: ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  ThingSpeak.begin(client);

  server.on("/", HTTP_GET, handle_OnConnect);
  server.on("/led1on", HTTP_GET, handle_led1on);
  server.on("/led1off", HTTP_GET, handle_led1off);
  server.on("/update", HTTP_GET, handle_UpdateThingSpeak);
  server.onNotFound(handle_NotFound);

  server.begin();
}

void loop() {
  server.handleClient();

  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
  }

  val1 = analogRead(ldrPin1);

  int ledVoltage = analogRead(A1);
  if (ledVoltage < voltageThreshold) {
    LED1status = LOW;
    server.send(200, "text/html", GenerateHTML("Voltage fault detected!"));
  }

  digitalWrite(LED1pin, LED1status);

  delay(1000);
}

void handle_OnConnect() {
  server.send(200, "text/html", GenerateHTML());
}

void handle_led1on() {
  LED1status = HIGH;
  led1Latitude = gps.location.lat();
  led1Longitude = gps.location.lng();
  server.send(200, "text/html", GenerateHTML());
}

void handle_led1off() {
  LED1status = LOW;
  server.send(200, "text/html", GenerateHTML());
}

void handle_UpdateThingSpeak() {
  ThingSpeak.writeField(myChannelNumber, 1, led1Latitude, myWriteAPIKey);
  ThingSpeak.writeField(myChannelNumber, 2, led1Longitude, myWriteAPIKey);

  server.send(200, "text/html", "ThingSpeak Updated");
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

String GenerateHTML(String additionalInfo = "") {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>LED Control</title>\n";
  ptr += "<style>\n";
  ptr += "body {\n";
  ptr += "  background: #F0F4F7;\n";
  ptr += "}\n";
  ptr += ".container {\n";
  ptr += "  display: flex;\n";
  ptr += "  flex-direction: column;\n";
  ptr += "  align-items: center;\n";
  ptr += "  justify-content: center;\n";
  ptr += "  height: 100vh;\n";
  ptr += "}\n";
  ptr += ".card {\n";
  ptr += "  background: rgba(255, 255, 255, 0.25);\n";
  ptr += "  backdrop-filter: blur(10px);\n";
  ptr += "  border-radius: 20px;\n";
  ptr += "  padding: 20px;\n";
  ptr += "}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div class=\"container\">\n";
  ptr += "<div class=\"card\">\n";
  ptr += "<h1>ESP8266 Web Server</h1>\n";
  ptr += "<h3>Using Station(STA) Mode</h3>\n";

  if (LED1status) {
    if (val1 == HIGH) {
      ptr += "<p style=\"color: red;\">Fault Detected: Check the light conditions for LED1!</p>\n";
      ptr += "<p>Latitude: " + String(led1Latitude, 6) + "</p>\n";
      ptr += "<p>Longitude: " + String(led1Longitude, 6) + "</p>\n";
      ptr += "<p>" + additionalInfo + "</p>\n";
    } else {
      ptr += "<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/led1off\">OFF</a>\n";
    }
  } else {
    ptr += "<p>LED1 Status: OFF</p><a class=\"button button-on\" href=\"/led1on\">ON</a>\n";
  }

  ptr += "</div>\n";
  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}
