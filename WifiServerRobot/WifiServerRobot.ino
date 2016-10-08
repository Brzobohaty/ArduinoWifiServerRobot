#include <Process.h>
#include <Bridge.h> 
#include <YunServer.h>
#include <YunClient.h>
#include <Servo.h>
#include <EEPROM.h>

YunServer server; //REST API server //https://www.arduino.cc/en/Tutorial/Bridge
int hedgehog_x, hedgehog_y; //aktuální souřadnice připojeného majáku
char hedgehog_x_buffer[8], hedgehog_y_buffer[8]; //buffery pro získání souřadnic z Linuxu v textové podobě
char hedgehog_error_buffer[1]; //buffer pro získání error hlášky z Linuxu
Servo servoRaid; //servo pro zatáčení
Servo motor; //motor pro pohon vozíku
int raidValue = 90; //aktuální hodnota raidu
const int buttonPin = 10; //pin na který je připojeno tlačítko
int buttonState = 0; //aktuální stav tlačítka
int lastButtonState = 0; //předchozí stav tlačítka
const int sizeOfLastPositionMemory = 40; //velikost paměti pro ukládání pozicí při zmáčknutí tlačítka
int lastXPositionBuffer[sizeOfLastPositionMemory], lastYPositionBuffer[sizeOfLastPositionMemory]; //posledních 60 naměřen7ch souřadnic při stisknutí tlačítka
int indexOfLastPosition = -1; //index naposledy naměřené pozice v bufferu

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
  
  pinMode(buttonPin, INPUT);
  
  delay(500);
}

void loop() {
  readPosition();
  readButtonState();
  
  //waiting for REST API request
  YunClient client = server.accept();
    
  if (client) {
    process(client);
    client.stop();
  }
}

//čte polohu ze streamu posílaného z Linuxu (tady ve výsledku z majáku)
void readPosition(){
  Bridge.get("error", hedgehog_error_buffer, 1);
  if(hedgehog_error_buffer[0] == 'N'){
    Bridge.get("x", hedgehog_x_buffer, 8);
    Bridge.get("y", hedgehog_y_buffer, 8); 
    hedgehog_x = atoi(hedgehog_x_buffer);
    hedgehog_y = atoi(hedgehog_y_buffer);
  }else{
    hedgehog_x = 0;
    hedgehog_y = 0;  
  }
}

//při stisku tlačítka zaznamená pozici
void readButtonState(){
  buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState) {
    if (buttonState == LOW && indexOfLastPosition < sizeOfLastPositionMemory) {
      indexOfLastPosition++;
      lastXPositionBuffer[indexOfLastPosition] = hedgehog_x;
      lastYPositionBuffer[indexOfLastPosition] = hedgehog_y;
      if(digitalRead(13) == HIGH){
        digitalWrite(13, LOW);
      }else{
        digitalWrite(13, HIGH);
      }
    }
  }
  lastButtonState = buttonState;
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
  if (command == "writeEEPROM") {
    writeEEPROM(client);
  }
  if (command == "readEEPROM") {
    readEEPROM(client);
  }
  if (command == "read-map-points") {
    readMapPoints(client);
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

//arduino/raid/[value]
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

//arduino/drive/[value]
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

//arduino/writeEEPROM/[variable]/[values]...
//Zapíše do perzistentní paměti proměnnou
//arduino/writeEEPROM/name/[name] - název zařízení
//arduino/writeEEPROM/alias/[alias] - zkratka zařízení
//arduino/writeEEPROM/distances/[distanceFromHedghogToBackOfCart]/[distanceFromHedghogToFrontOfCart]/[distanceFromHedghogToLeftSideOfCart]/[distanceFromHedghogToRightSideOfCart] - vzdálenosti od majáku
void writeEEPROM(YunClient client){
  String var = client.readStringUntil('/');
  if(var == "name"){
    //char deviceName[50]; 
    String deviceName = client.readStringUntil('\r');
    for (int i=0; i <= 49; i++){
        EEPROM.write(i, deviceName[i]);
    }
  }else if(var == "alias"){
    //char deviceAlias[20]; 
    String deviceAlias = client.readStringUntil('\r');
    for (int i=50; i <= 69; i++){
        EEPROM.write(i, deviceAlias[i-50]);
    }
  }else if(var == "distances"){
    EEPROM.write(70, client.parseInt());
    EEPROM.write(71, client.parseInt());
    EEPROM.write(72, client.parseInt());
    EEPROM.write(73, client.parseInt());
  }
}

//arduino/readEEPROM
//Přečte z perzistentní paměti proměnnou
//Pokud proměnná neexistuje, vrací NOT SET
//arduino/writeEEPROM/name - název zařízení
//arduino/writeEEPROM/alias - zkratka zařízení
//arduino/writeEEPROM/distances - vzdálenosti od majáku (distanceFromHedghogToBackOfCart,distanceFromHedghogToFrontOfCart,distanceFromHedghogToLeftSideOfCart,distanceFromHedghogToRightSideOfCart)
void readEEPROM(YunClient client){
  String var = client.readStringUntil('\r');
  if(var == "name"){
    if(EEPROM.read(0) == 255){
      client.println("NOT SET");
      return;
    }
    char deviceName[50];
    for (int i=0; i <= 49; i++){
        deviceName[i] = EEPROM.read(i);
    }
    client.println(deviceName);
  }else if(var == "alias"){
    if(EEPROM.read(50) == 255){
      client.println("NOT SET");
      return;
    }
    char deviceAlias[20];
    for (int i=50; i <= 69; i++){
        deviceAlias[i-50] = EEPROM.read(i);
    }
    client.println(deviceAlias);
  }else if(var == "distances"){
    if(EEPROM.read(70) == 255){
      client.println("NOT SET");
      return;
    }
    int distanceFromHedghogToBackOfCart = EEPROM.read(70);
    int distanceFromHedghogToFrontOfCart = EEPROM.read(71);
    int distanceFromHedghogToLeftSideOfCart = EEPROM.read(72);
    int distanceFromHedghogToRightSideOfCart = EEPROM.read(73);
    
    String result = String(distanceFromHedghogToBackOfCart)+","+String(distanceFromHedghogToFrontOfCart)+","+String(distanceFromHedghogToLeftSideOfCart)+","+String(distanceFromHedghogToRightSideOfCart);
    
    client.println(result);
  }
}

//arduino/read-map-point
//Přečte naposledy zaznamenané pozice při stisku tlačítka ("NOT SET", pokud nebyla žádná pozice zatím zaznamenána)
void readMapPoints(YunClient client) {
  if(indexOfLastPosition != -1){
    String s = "";
    for (int i=0; i <= indexOfLastPosition; i++){
        s.concat(lastXPositionBuffer[i]);
        s.concat(",");
        s.concat(lastYPositionBuffer[i]);
        if(i != indexOfLastPosition){
          s.concat(";");
        }
    }
    client.println(s);
    indexOfLastPosition = -1;
  }else{
    client.println(F("NO_SET"));
  }
}
