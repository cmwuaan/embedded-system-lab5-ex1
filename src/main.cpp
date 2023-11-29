#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncTCP.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <DHT_U.h>

#define MESH_PREFIX "Nhom06.Mesh1"
#define MESH_PASSWORD "Nhom06.Mesh1"
#define MESH_PORT 5555

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;
StaticJsonDocument<2048> data;

const int dhtPin = 2;
const int dhtType = DHT11;

DHT dht(dhtPin, dhtType);

// User stub
void sendMessage(); // Prototype so PlatformIO doesn't complain

Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void sendMessage()
{
    JsonObject msgData = data.createNestedObject("data");
    data["node"] = mesh.getNodeId();
    msgData["temp"] = dht.readTemperature();
    msgData["hum"] = dht.readHumidity();
    String message = "team_6";
    serializeJson(data, message);
    mesh.sendBroadcast(message);
    taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

// Needed for painless library
void receivedCallback(uint32_t from, String &msg)
{
    StaticJsonDocument<256> doc;

    DeserializationError err = deserializeJson(doc, msg);

    if (err)
    {
        Serial.print("ERROR: ");
        Serial.println(err.c_str());
        return;
    }

    float temp = doc["data"]["temp"];
    float humid = doc["data"]["hum"];

    Serial.printf("Received from %u: temp = %f, humid = %f\n", from, temp, humid);
}

void newConnectionCallback(uint32_t nodeId)
{
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback()
{
    Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset)
{
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup()
{
    Serial.begin(115200);

    // mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
    mesh.setDebugMsgTypes(ERROR | STARTUP); // set before init() so that you can see startup messages

    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive(&receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.onChangedConnections(&changedConnectionCallback);
    mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

    userScheduler.addTask(taskSendMessage);
    taskSendMessage.enable();
}

void loop()
{
    // it will run the user scheduler as well
    mesh.update();
}
