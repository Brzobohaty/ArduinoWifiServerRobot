#include <Process.h>
#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

//https://www.arduino.cc/en/Tutorial/Bridge

YunServer server;

void setup() {
  Serial.begin(9600);  // initialize serial communication
  //while (!Serial);     // do nothing until the serial monitor is opened

  Serial.println("Starting bridge...\n");
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  Bridge.begin();  // make contact with the linux processor
  digitalWrite(13, LOW);

  // start REST API server
  Serial.println("Starting server...\n");
  digitalWrite(13, HIGH);
  server.listenOnLocalhost();
  server.begin();
  digitalWrite(13, LOW);
  Serial.println("Server started...\n");

  delay(2000);  // wait 2 seconds
}

void loop() {
  //waiting for REST API request
  YunClient client = server.accept();
  
  if (client) {
    process(client);
    client.stop();
  }

  delay(50); 
}

//REST API definition
void process(YunClient client) {
  String command = client.readStringUntil('/');
  command.trim();
  if (command == "check") {
    checkCommand(client);
    Serial.println("check");
  }
  if (command == "analog") {
    Serial.println("analog");
  }
  if (command == "mode") {
    Serial.println("mode");
  }
}

//checking connection to Arduino
void checkCommand(YunClient client) {
  client.println(F("NVC8mK73kAoXzLAYxFMo"));
}
