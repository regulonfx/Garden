#include <Adafruit_Sensor.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AS_BH1750.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// MESH
#define   MESH_PREFIX     "IGEL" //name for your MESH
#define   MESH_PASSWORD   "SCHNECKE" //password for your MESH
#define   MESH_PORT       5555 //default port

// Display
#define OLED_I2C_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Temperatursensor
#define BMP280_I2C_ADDRESS  0x76

// globale Variablen ---------------------------------------------------------
//Number for this mesh-node
int nodeNumber = 2;
// Lichtsenor
AS_BH1750 sensor;
int val_analogique;
// Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// Temperatur
Adafruit_BMP280 bmp280;

//String to send to other nodes with sensor readings
String readings;

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// von Mesh
void sendMessage() ; // Prototype so PlatformIO doesn't complain
String getReadings(); // Prototype for sending sensor readings

// Variablen für empfangene SoilSensordaten
double node2soil = 0;
double node3soil = 0;
double node4soil = 0;
double node5soil = 0;

//Create tasks: to send messages and get readings;
Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);

String getReadings () {
  return "";
}

void sendMessage () {
  String msg = getReadings();
  mesh.sendBroadcast(msg);
}


// Needed for painless library
// LESEN
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  JSONVar myObject = JSON.parse(msg.c_str());

  int node = myObject["node"];
  if (node == 2) {
    node2soil = myObject["soil"];
    Serial.println("Node 2: ");
    Serial.println(node2soil);
  }
  if (node == 3) {
    node3soil = myObject["soil"];
    Serial.println("Node 3: ");
    Serial.println(node3soil);
  }
  if (node == 4) {
    node4soil = myObject["soil"];
    Serial.println("Node 4: ");
    Serial.println(node4soil);
  }
  if (node == 5) {
    node5soil = myObject["soil"];
    Serial.println("Node 5: ");
    Serial.println(node5soil);
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);

  //Added
  // display vorbereitung
  display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.display();

  // bmp280 vorbereitung
  bmp280.begin(BMP280_I2C_ADDRESS);

  // UV Sensor
  sensor.begin();

  mesh.setDebugMsgTypes( ERROR | STARTUP );  // startup message

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendMessage);
  //taskSendMessage.enable();
}

void loop() {
  mesh.update();

  display.clearDisplay();
  display.setTextSize(2); // Normal 1:1 pixel scale
  display.setTextColor(WHITE, BLACK); // Draw white text
  display.setCursor(0, 0); // Start at top-left corner


  // Page 7
  display.println("Local P7/7");
  // 1: print Soil Node 3
  display.setCursor(0, 0);
  display.println("Soil:");
  display.print(node3soil);

}
