#include <Adafruit_Sensor.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AS_BH1750.h>
#include <Adafruit_BMP280.h>

// MESH DEFINE
#define   MESH_PREFIX     "IGEL" //name for your MESH
#define   MESH_PASSWORD   "SCHNECKE" //password for your MESH
#define   MESH_PORT       5555 //default port

// DISPLAY DEFINE
#define OLED_I2C_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define BMP280_I2C_ADDRESS  0x76

// GLOBALE VARIABLEN ---------------------------------------------------
int nodeNumber = 1;
int page = 6;
bool regen = false;

float lux;
AS_BH1750 sensor;
int rainData;
bool waterRunning;
int wasserflussDauer = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_BMP280 bmp280;

// für MESH
String readings; //String to send to other nodes
Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;
void sendMessage() ; // Prototype so PlatformIO doesn't complain
String getReadings(); // Prototype for sending sensor readings

double node2soil = 0;
double node3soil = 0;

//Create tasks: to send messages and get readings;
Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);

// SETUP -------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  // Relais Pin
  pinMode(0, 1);

  // display vorbereitung
  display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.display();

  // Temperatur, Druck, Höhe Sensor
  bmp280.begin(BMP280_I2C_ADDRESS);

  // UV Sensor
  sensor.begin();

  // MESH
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // startup messages
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendMessage);
  //taskSendMessage.enable();
}

// LOOP --------------------------------------------------------------
void loop() {
  mesh.update();

  displayStart();
  mesh.update();


  relaisSchaltung();


  // Page 1
  temperature();
  mesh.update();

  // Page 2
  pressure();
  mesh.update();

  // Page 3
  light();
  mesh.update();

  // Page 4
  rain();
  mesh.update();

  // Page 5
  soilNode2();
  mesh.update();

  // Page 6
  soilNode3();
  mesh.update();

  // Page 7

}


// eigene METHODEN ------------------------------------------------------------
void displayStart() {
  display.clearDisplay();
  display.setTextSize(2); // Normal 1:1 pixel scale
  display.setTextColor(WHITE, BLACK); // Draw white text
  display.setCursor(0, 0); // Start at top-left corner
}

void temperature() {
  float temperature = bmp280.readTemperature();  // get temperature

  display.print("Local P");
  display.print(page - 5);
  display.print("/");
  display.println(page);

  display.print("Temp in ");
  display.print((char)247);
  display.println("C");
  display.print(temperature);

  display.display();
  delay(2000);
  display.clearDisplay();
  display.setCursor(0, 0);
}

void pressure() {
  float pressure = bmp280.readPressure();

  display.print("Local P");
  display.print(page - 4);
  display.print("/");
  display.println(page);

  display.print("Druck hPa:");
  display.println(pressure / 100);

  display.display();
  delay(2000);
  display.clearDisplay();
  display.setCursor(0, 0);
}

/*
  void altitude() {
  float altitude   = bmp280.readAltitude(1025) + 75;
  display.println("Local P3/7");
  display.print("H");
  display.print("\224");
  display.print("he in m:");
  display.println(altitude);
  display.println(" m");

  display.display();
  delay(2000);
  display.clearDisplay();
  display.setCursor(0, 0);
  }
*/
void light() {
  lux = sensor.readLightLevel();

  display.print("Local P");
  display.print(page - 3);
  display.print("/");
  display.println(page);

  display.print("UV in lux:");
  display.print(String(lux));

  display.display();
  delay(2000);
  display.clearDisplay();
  display.setCursor(0, 0);
}

void rain() {
  rainData = analogRead(A0);
  regenKenner();
  display.print("Local P");
  display.print(page - 2);
  display.print("/");
  display.println(page);

  display.print("Rain in V:");
  display.println(rainData);

  display.display();
  delay(2000);
  display.clearDisplay();
  display.setCursor(0, 0);

}

void soilNode2() {
  display.print("Local P");
  display.print(page - 1);
  display.print("/");
  display.println(page);
  display.println("Soil:");
  display.print(node2soil);

  display.display();
  delay(2000);
  display.clearDisplay();
  display.setCursor(0, 0);

}

void soilNode3() {
  display.print("Local P");
  display.print(page);
  display.print("/");
  display.println(page);
  display.println("Soil:");
  display.print(node3soil);

  display.display();
  delay(2000);
  display.clearDisplay();
  display.setCursor(0, 0);

}

void relaisOn() {
  digitalWrite(0, HIGH);
  waterRunning = true;
}

void relaisOff() {
  digitalWrite(0, LOW);
  waterRunning = false;
}

void relaisSchaltung() {
  if (waterRunning == false) {
    if (regen == false) {
      if (lux < 1000 && node2soil > 400) {
        relaisOn();

      }
    }
  }
  if (waterRunning == true) {
    wasserflussDauer++;
  }
  if (waterRunning == true && wasserflussDauer == 2) {
    relaisOff();
    wasserflussDauer = 0;
  }
  //  if (regen == true) {
  //    if (lux >= 1000 && node2soil <= 450 && regen == false) {
  //    relaisOff();
  //  }

}


void regenKenner() {

  if (rainData < 1000) {
    regen = true;
  } else {
    regen = false;
  }
}

// MESH - Methoden ------------------------------------------------------------
String getReadings () {
  return "";
}

void sendMessage () {
  String msg = getReadings();
  mesh.sendBroadcast(msg);
}

// Needed for painless library
// LESEN
void receivedCallback( uint32_t from, String & msg ) {
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
  // ifs für weitere SensorStationen
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
