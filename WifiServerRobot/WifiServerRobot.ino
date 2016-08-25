#include <Process.h>
#include <Bridge.h> 
#include <YunServer.h>
#include <YunClient.h>
#include <Servo.h>

YunServer server; //REST API server //https://www.arduino.cc/en/Tutorial/Bridge
int hedgehog_x, hedgehog_y; //aktuální souřadnice připojeného majáku
char hedgehog_x_buffer[8], hedgehog_y_buffer[8]; //buffery pro získání souřadnic z Linuxu v textové podobě
char hedgehog_error_buffer[1]; //buffer pro získání error hlášky z Linuxu
Servo servoRaid; //servo pro zatáčení
Servo motor; //motor pro pohon vozíku
int raidValue = 90; //aktuální hodnota raidu

void setup() {
  servoRaid.attach(9); //servo zatáčení připojeno na digitální port 9
  motor.attach(11); //motor pro pohon vozíku připojen na digitální port 11
  delay(500);
  
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  Bridge.begin();  // make contact with the linux processor
  digitalWrite(13, LOW);

  delay(500);

  // start REST API server
  digitalWrite(13, HIGH);
  server.listenOnLocalhost();
  server.begin();
  digitalWrite(13, LOW);
  
  delay(500);
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
  if (command == "position") {
    getBeaconPosition(client);
  }
  if (command == "raid") {
    raidStep(client);
  }
  if (command == "drive") {
    driveStep(client);
  }
}

//***REST API***//

//arduino/check
//checking connection to Arduino
void checkCommand(YunClient client) {
  client.println(F("NVC8mK73kAoXzLAYxFMo"));
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
    String s = "";
    s.concat(hedgehog_x);
    s.concat(",");
    s.concat(hedgehog_y);
    client.println(s);
  }else if(hedgehog_error_buffer[0] == 'E'){
    client.println(F("NO_HEDGEHOG_CONNECTION_OR_NO_POSITION_UPDATE"));
  }else if(hedgehog_error_buffer[0] == 'S'){
    client.println(F("NO_HEDGEHOG_CONNECTION_OR_NO_POSITION_UPDATE"));
  }else{
    client.println(F("NO_RESPONSE_FROM_LINUX"));
  }
}

//arduino/raid/value
//Zatočí přední kola vozíku o danou hodnota
void raidStep(YunClient client){
  int value;
  value = client.parseInt();
  raidValue = raidValue+value;
  
  if(raidValue < 75){
    raidValue = 75;
  }else if (raidValue > 105){
    raidValue = 105;
  }
  servoRaid.write(raidValue);
  delay(15);
}

//arduino/drive/value
//Pohne s robotem dopředu nebo dozadu podle dané hodnoty (záporná pihybuje s robotem dozadu)
//Pohyb není nijak měřen a hodnota určuje pouze čas po který se bude vozík pohybovat (hodnota tedy určuje relativní míru kroku)
//Robot se bude pohybovat nejmenší možnou rychlostí
void driveStep(YunClient client){
    //31 - 90 - dopředu
    //90 - brzda
    //100 - 159 - dozadu
    int value;
    bool reverse = false;
    value = client.parseInt(); 
    
    if(value < 0){
      reverse = true;
      value = -value; 
    }
    
    if(reverse){
      motor.write(104);
    }else{
      motor.write(80);
    }
    
    delay(value);
    
    motor.write(90);
}
