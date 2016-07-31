#include <Process.h>
#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

YunServer server; //REST API server
int hedgehog_x, hedgehog_y; //aktuální souřadnice připojeného majáku
char hedgehog_x_buffer[8], hedgehog_y_buffer[8]; //buffery pro získání souřadnic z Linuxu v textové podobě
char hedgehog_error_buffer[1]; //buffer pro získání error hlášky z Linuxu

void setup() {
  delay(2000);
  
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  Bridge.begin();  // make contact with the linux processor
  digitalWrite(13, LOW);

  delay(2000);

  // start REST API server
  digitalWrite(13, HIGH);
  server.listenOnLocalhost();
  server.begin();
  digitalWrite(13, LOW);
  
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
  }
  if (command == "beacon-adress") {
    getBeaconAdress(client);
  }
  if (command == "position") {
    getBeaconPosition(client);
  }
}

//***REST API***//

//arduino/check
//checking connection to Arduino
void checkCommand(YunClient client) {
  client.println(F("NVC8mK73kAoXzLAYxFMo"));
}

//arduino/beacon-adress
//Vrátí adresu majáku připojeného k tomuto zařízení
void getBeaconAdress(YunClient client){
  client.println(F("BEACON_NOT_CONNECTED"));
}

//arduino/position
//Vrátí polohu majáku připojeného k tomuto zařízení
void getBeaconPosition(YunClient client){
  Bridge.get("error", hedgehog_error_buffer, 1);
  if(hedgehog_error_buffer[0] == 'N'){
    Bridge.get("x", hedgehog_x_buffer, 8);
    Bridge.get("y", hedgehog_y_buffer, 8);
    hedgehog_x = atoi(hedgehog_x_buffer);
    hedgehog_y = atoi(hedgehog_y_buffer);
    String s = "X: ";
    s.concat(hedgehog_x);
    s.concat(", Y:");
    s.concat(hedgehog_y);
    client.println(s);
  }else if(hedgehog_error_buffer[0] == 'E'){
    client.println(F("NO_HEDGEHOG_CONNECTION_OR_NO_POSITION_UPDATE"));
  }else{
    client.println(F("NO_RESPONSE_FROM_LINUX"));
  }
}
