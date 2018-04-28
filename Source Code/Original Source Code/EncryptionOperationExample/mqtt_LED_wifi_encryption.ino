#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>

//The following include is for led FSMs
#include <FiniteStateMachine.h>
#include <LED.h>

//The following include is for button input
#include <Button.h>

#include <Base64.h>  //the arduinobase64 library had to be modified to remove the need for the avr/progmem library
#include <AES.h>
#include <AES_config.h>

//Built from the examples at: https://github.com/adamvr/arduino-base64 (modification to not use PROGMEM - an AVR function) and http://spaniakos.github.io/AES/ (a modification was to AES.CPP as requested to change type to const static variable to compile)
//see more details on this AES engine: http://spaniakos.github.io/AES/classAES.html

WiFiManager wifiManager;

LED redStatusLed=LED(5);
LED greenStatusLed=LED(0);
LED wifiLED=LED(4);
int buttonCount=0;
int meterCount = 0; 
boolean meterRead = false; 
float totalCurrent = 0; 
Button button = Button(2,PULLUP);
//int wifiMode=0;
int reconn_count=0;
/*
String RequestLEDoff1;
String RequestLEDon1;
String StatusLEDoff1;
String StatusLEDon1;

String RequestLEDoff2;
String RequestLEDon2;
String StatusLEDoff2;
String StatusLEDon2;
*/
String RequestLEDoff1 =  "RqstOFF1!!!!!!!!";
String RequestLEDon1 =   "RqstON1!!!!!!!!!";
String StatusLEDoff1 =   "StatusOFF1!!!!!!";
String StatusLEDon1 =    "StatusON1!!!!!!!";
bool LED1 = false;

String RequestLEDoff2 =  "RqstOFF2!!!!!!!!";
String RequestLEDon2 =   "RqstON2!!!!!!!!!";
String StatusLEDoff2 =   "StatusOFF2!!!!!!";
String StatusLEDon2 =    "StatusON2!!!!!!!";
bool LED2 = false;

//const char* ssid = "UCInet Mobile Access";
//const char* password = "";
//const char* mqtt_server = "mqtt://192.168.1.115";
const char* mqtt_server = "m10.cloudmqtt.com";

WiFiClient espClient;
//PubSubClient client(mqtt_server, 15710,callback, espClient);
//PubSubClient client(WiFi.localIP(),15710,espClient);
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
boolean change=false;


byte key[] = "2222222222222222"; 


/**********************Encryption***********************************/


AES aes ; //AES object

void print_value (const char * str, byte * a, int bits)
{
  char hex[] = "0123456789abcdef" ;
  Serial.print(str) ;
  bits >>= 3 ;
  for (int i = 0 ; i < bits ; i++)
    {
      byte b = a[i] ;
      Serial.print(hex [b >> 4]) ;
      Serial.print(hex [b & 15]) ;
    }
  Serial.println() ;
}

String encodeMessage(byte key[], String msg)
{

      byte plain[msg.length()+1];
      msg.getBytes(plain, msg.length()+1); 
      byte cipher [N_BLOCK] ; 
      aes.set_key (key, 128) ;
      aes.encrypt (plain, cipher) ;
    //  Serial.println("****Encryption*****");
     // print_value("Plain = ", plain, 128); 
    //  print_value("Cipher = ", cipher, 128); 
      aes.copy_n_bytes (plain, cipher, sizeof(plain)) ;
    /*
    //orig message
    Serial.print("Original Message: ");
    for (int i=0; i<sizeof(plain); i++)
    {
    Serial.print(char(plain[i]));
    }
    Serial.println();
    
    
    //cipher
    Serial.print("Encoded (Cipher): ");
    for (int i=0; i<sizeof(cipher); i++)
    {
      Serial.print(char(cipher[i]));
    }
    Serial.println();
    */
    //Base64 converter:
    
    char data [sizeof(cipher)+1] = {0}; //make null character array    
    for (int i=0; i<sizeof(cipher); i++) //trasfer cipher into null character array
    {
      data[i] = char (cipher[i]); //should there be a char typecast?
    }
    //int inputLen = sizeof(data);
    int inputLen = sizeof(cipher);
    int encodedLen = base64_enc_len(inputLen);
    char encoded[encodedLen];
    base64_encode(encoded, data, inputLen);    
   // Serial.print("Encoded (after Base64):");
   // Serial.println(encoded);

    return String(encoded);
}

String decodeMessage(byte key[], String msg, int msgLength)
{
//  Serial.println("***Decoded Message*** ");

//  Serial.print("Input String: ");
//  Serial.println(msg);
  
  char encoded[msg.length()+1]; 
  msg.toCharArray(encoded, msg.length()+1); 
/*  Serial.print("Encoded Message: ");
  for (int i=0; i < sizeof(encoded); i++)
  {
     Serial.print(encoded[i]);
  }
  Serial.println();
*/
  int input2Len = sizeof(encoded);
  int decodedLen = base64_dec_len(encoded, input2Len);
  char decoded[decodedLen];
     
  base64_decode(decoded, encoded, input2Len);
  /*
  Serial.print("Decoded (returned from Base64):");
  for (int i=0; i<sizeof(decoded); i++)
  {
    Serial.print(char(decoded[i]));
  }
  Serial.println();
*/
  String decodedString = String(decoded);

  byte cipher[decodedString.length()+1]; 
  decodedString.getBytes(cipher, decodedString.length()+1);

  byte check[msgLength]; 

  aes.set_key (key, 128) ;
  aes.decrypt(cipher, check) ;
/*
  Serial.print("Check: ");
  for (int i=0; i<sizeof(check); i++)
  {
    Serial.print(char(check[i]));
  }
  Serial.println();

  Serial.print("Size of Check: ");
  Serial.print(sizeof(check));

*/
  return String((char*) check).substring(0,msgLength); 
  
}



/***********************Current Calibration*****************************/
float currentLinearCalibration(float gain, float value, float offset)
{
    float current; 
    current = gain*value+offset; 
    return current; 
}

float currentQuadCalibration(float gain2, float gain1, float value, float offset)
{
    float current; 
    current = gain2*value*value+gain1*value+offset; 
    return current; 
}


/*********************Wifi***************************************/
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting a connection to MQTT...");

 //   if (client.connect("ESP8266Client", "yhpjclsx", "RwSAmqe-htvY")) {
    if (client.connect("ESP8266Client", "xpskpkpr", "qoxaerdSjzu5")) {
      Serial.println("connected");
      client.publish("topic/1", "publishing-yes");
      client.subscribe("topic/1");//subscribe to data from topic/2
      client.publish("topic/2", "publishing-yes");
      client.subscribe("topic/2");
      client.publish("topic/3", "publishing-yes");
      client.subscribe("topic/3");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" will try connecting again in 5 secs");
      delay(5000);
    }
    //if (reconn_count=2000){
     // setup_wifi();
    //}
  }
  reconn_count=0;
}

void mqtt_pub(){
  
    if(LED1){

      String eM = encodeMessage(key, StatusLEDon1);
      char state[eM.length()+1]; 
      eM.toCharArray(state, eM.length()+1); 
      
      snprintf (msg, 75, state, value);
     // snprintf (msg, 75, eM, value);
      Serial.print("E.D. Published: ");
      Serial.print(StatusLEDon1);
      Serial.print("\n");
      client.publish("topic/1", msg);
    }
    else{

      String eM = encodeMessage(key, StatusLEDoff1);
      char state[eM.length()+1]; 
      eM.toCharArray(state, eM.length()+1); 
      
      snprintf (msg, 75, state, value);
      Serial.print("E.D. Published: ");
      Serial.print(StatusLEDoff1);
      Serial.print("\n");
      client.publish("topic/1", msg);
    }

    if(LED2){

      String eM = encodeMessage(key, StatusLEDon2);
      char state[eM.length()+1]; 
      eM.toCharArray(state, eM.length()+1); 
      
      snprintf (msg, 75, state, value);
      Serial.print("E.D. Published: ");
      Serial.print(StatusLEDon2);
      Serial.print("\n");
      client.publish("topic/2", msg);
    }
    else{
      String eM = encodeMessage(key, StatusLEDoff2);
      char state[eM.length()+1]; 
      eM.toCharArray(state, eM.length()+1);       
      snprintf (msg, 75, state, value);
      Serial.print("E.D. Published: ");
      Serial.print(StatusLEDoff2);
      Serial.print("\n");
      client.publish("topic/2", msg);
    }
   

  
}


//STATUS LED FSM transition functions
void loadoff(){
 redStatusLed.off();
 greenStatusLed.off();
 LED1=false;
 LED2=false;
 digitalWrite(12,LOW); 
 digitalWrite(13,LOW);
 digitalWrite(14,LOW);
}

void load1(){
greenStatusLed.on();
redStatusLed.off();
LED1=true;
LED2=false;
digitalWrite(12,HIGH); 
digitalWrite(13,LOW);
digitalWrite(14,HIGH);
//mqtt_pub();
}
void load1off(){
  greenStatusLed.off(); 
  LED1=false;
  LED2=false;
  digitalWrite(12,LOW); 
//  digitalWrite(13,LOW);
  digitalWrite(14,LOW);
}

void load2(){ 
  greenStatusLed.off();
  redStatusLed.on();
  LED2=true;
  LED1=false;
  digitalWrite(12,LOW); 
  digitalWrite(13,HIGH);
  digitalWrite(14,HIGH);
}
void load2off(){
  redStatusLed.off();
  LED2=false;
  LED1=false;
//  digitalWrite(12,LOW); 
  digitalWrite(13,LOW);
  digitalWrite(14,LOW);
}


void powerBoth(){
 greenStatusLed.on();
  delay(50);
 greenStatusLed.off();
 delay(50);
 redStatusLed.on();
  delay(50);
  redStatusLed.off();
  delay(50);
  LED1=true;
  LED2=true;
  digitalWrite(12,HIGH); 
  digitalWrite(13,HIGH);
  digitalWrite(14,HIGH);
}

//STATUS LED FSM States
State loadsOFF = State(loadoff,loadoff,load1);
State load1ON=State(load1,load1,load1off);
State load2ON=State(load2,load2,load2off);
State bothLoads=State(powerBoth,powerBoth,loadoff);

//STATUS LED FSM
FSM loadStatus=FSM(loadsOFF);




//WIFI LED TRANSITION FUNCTIONS
void wifiConnected(){
    wifiLED.on();
    //Serial.println("Connected");
}

void wifi_AP(){
  wifiLED.on();
  delay(250);
   wifiLED.off();
   delay(500);
    wifiLED.on();
    delay(250);
   wifiLED.off();
   delay(250);
  //wifiLED.on();
 // Serial.println("AP MODE");
}

void wifiSearching(){
  wifiLED.blink(100);
 // Serial.println("Searching");
}

void upgradefirmware(){
  wifiLED.blink(70);
 // Serial.println("Firmware");
}


void wifiDisconnected(){
  wifiLED.off();
}

//WIFI LED STATES
State wifiDis=State(wifiDisconnected,wifiDisconnected,wifiConnected);
State wifiAP=State(wifi_AP,wifi_AP,wifiDisconnected);
State wifiConnect=State(wifiConnected,wifiConnected,wifiDisconnected);
State firmware=State(upgradefirmware);


//WIFI LED FSM
FSM wifiStatus=FSM(wifiDis);

void wifiUpdate(int Mode){
  //Serial.println("Wifi LED UPDATE");
   //Serial.println(Mode);
  switch(Mode){
    case 0:wifiStatus.transitionTo(wifiAP);wifiStatus.update();break;
    //case 1:wifiStatus.transitionTo(wifiSearch);wifiStatus.update();break;
    case 2:wifiStatus.transitionTo(wifiConnect);wifiStatus.update();break;
    case 3:wifiStatus.transitionTo(wifiDis);wifiStatus.update();break;
  }
  wifiStatus.update();

}


void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  wifiUpdate(0);
}


void resetWifi()
{
  wifiManager.resetSettings();
}


void setup_wifi() { 

/*  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());*/

  wifiManager.setAPCallback(configModeCallback);
  //wifiUpdate(0);
 Serial.println("Setting up Wifi");
    if (!wifiManager.autoConnect("IOTMLC","22team22")) {
      
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  
wifiUpdate(2);

  
}
/***********************************************************************************/
void callback(char* topic, byte* payload, unsigned int length1) {

  byte key[] = "2222222222222222";
  String encrypted_msg = (char *) payload; 
 // encrypted_msg = encrypted_msg.substring(0,16);
  Serial.print("Payload Message: "); 
  Serial.println(encrypted_msg); 
  /*
  String e = encodeMessage(key, encrypted_msg); 
  Serial.print("Encrypt Message: ");
  Serial.println(e);
  */
  String decrypted_msg = decodeMessage(key, encrypted_msg, 16);
  Serial.print("Decrypt Message: "); 
  Serial.println(decrypted_msg);

  //String decrypted_msg = decodeMessage(key, encrypted_msg, 16); 
  
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.print("\n");

  int j = 0;
  int k = 0;
  int m = 0;
  int n = 0;

 // if (topic[6] == '1')
  //{
    Serial.println("Entering topic 1");
    /*for (int i = 0; i < RequestLEDon1.length(); i++){
      if((char)payload[i] == RequestLEDon1[i]){
        j++;
      } 
    }
    if(j == RequestLEDon1.length()){
      //digitalWrite(13, HIGH);
    */
    if (decrypted_msg == RequestLEDon1)
    {
      LED1 = true; 
    }
    /*
    for (int i = 0; i < RequestLEDoff1.length(); i++){
      if((char)payload[i] == RequestLEDoff1[i]){
        k++;
      } 
    }
    if(k == RequestLEDoff1.length()){
    */
    if (decrypted_msg == RequestLEDoff1)
    {
      //digitalWrite(13, LOW);
      LED1 = false;
     // loadStatus.transitionTo(load1off);loadStatus.update(); 
    }
  /*
    for (int i = 0; i < RequestLEDoff1.length(); i++){
      if((char)payload[i] == RequestLEDoff1[i]){
        k++;
      } 
    }
    if(k == RequestLEDoff1.length()){
      //digitalWrite(13, LOW);
      LED1 = false;
    //  loadStatus.transitionTo(load1off);loadStatus.update();
    }
  
 // }
*/
  //if (topic[6] == '2')
  //{
  /*
    Serial.println("Entering topic 2");
    j = 0;
    k = 0;
    m = 0;
    n = 0;
    for (int i = 0; i < RequestLEDon2.length(); i++){
      if((char)payload[i] == RequestLEDon2[i]){
        j++;
      } 
    }
    if(j == RequestLEDon2.length()){
      //digitalWrite(13, HIGH);
     */
     if (decrypted_msg == RequestLEDon2)
     {
        LED2 = true;
     // loadStatus.transitionTo(load2ON);loadStatus.update();
     }
    /*
    for (int i = 0; i < RequestLEDoff2.length(); i++){
      if((char)payload[i] == RequestLEDoff2[i]){
        k++;
      } 
    }
    if(k == RequestLEDoff2.length()){

    */
    if (decrypted_msg == RequestLEDoff2)
    {
      //digitalWrite(13, LOW);
      LED2 = false;
    }
  /*
    for (int i = 0; i < RequestLEDoff2.length(); i++){
      if((char)payload[i] == RequestLEDoff2[i]){
        k++;
      } 
    }
    if(k == RequestLEDoff2.length()){
    
    if ((char*)paylod == RequestLEDoff2
      //digitalWrite(13, LOW);
      LED2 = false;
    }
  //}
  */




if(!LED1&!LED2){
  
    loadStatus.transitionTo(loadsOFF);loadStatus.update(); 
    buttonCount=0;
    meterRead = false; 
  }
  else if (LED1&LED2){
    loadStatus.transitionTo(bothLoads); loadStatus.update();
    buttonCount=3;
    meterRead = true; 

  }
  else if (LED1&!LED2){
    loadStatus.transitionTo(load1ON);loadStatus.update(); 
    buttonCount=1;
    meterRead = true; 
    } 
  else if (LED2&!LED1){
  //  loadStatus.transitionTo(loadsOFF);loadStatus.update();
    loadStatus.transitionTo(load2ON);loadStatus.update();
    buttonCount=2;
    meterRead = true; 
    }
  
 Serial.print("LED 1 ");
 Serial.println(LED1);
  Serial.print("LED 2 ");
 Serial.println(LED2);
}

/***********************************************************************************/


/***********************************************************************************/
void setup() {
 // pinMode(13, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  //resetWifi();
  //client.setServer(mqtt_server, 13526);
  client.setServer(mqtt_server, 16565);

  client.setCallback(callback);


  pinMode(12, OUTPUT); 
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
 // pinMode(15, OUTPUT);

  digitalWrite(12,LOW);
  digitalWrite(13,LOW);
  digitalWrite(14,LOW);
 // digitalWrite(15,LOW);

 pinMode(A0, INPUT);
  
  
}
/***********************************************************************************/
void loop() {



   if (!client.connected()) {
      wifiUpdate(3);
      reconnect();
    }
    client.loop();
     //client.subscribe("topic/2");//subscribe to data from topic/2
   wifiUpdate(2);
    long now = millis();
    if (now - lastMsg > 2000) {
      lastMsg = now;
      
    }

  
if (button.uniquePress()){
  buttonCount++;
  change=true;
  //delay(500);
}

wifiUpdate(2);
buttonCount=buttonCount%4;
switch(buttonCount){
  case 0:loadStatus.transitionTo(loadsOFF);loadStatus.update(); break;
  case 1:loadStatus.transitionTo(load1ON);loadStatus.update(); break;
  case 2:loadStatus.transitionTo(load2ON);loadStatus.update(); break;
  case 3:loadStatus.transitionTo(bothLoads); loadStatus.update(); break;
  
}

if (change==true){
  mqtt_pub();
  change=false;
}

  //This part of the code reads from port ADC0 and publishes to topic 3
 // if (meterRead)
 // {
    meterCount++; 
    int portRead = analogRead(A0);
   // float current = currentLinearCalibration(1,portRead,0); 
  //  float current = currentCalibration(0.0346,portRead,-24.221);
    float current;
    if (LED1 & LED2)
    {
      if (portRead < 0.332)
      {
        current = currentQuadCalibration(-0.0177,25.134,portRead,-8902.5);
      }
      else
      {
        current = currentLinearCalibration(0.0346,portRead,-24.221);
      }
    }
    else if (LED1 & !LED2)
    {
       current = currentLinearCalibration(0.0344,portRead,-23.613);
    }
    else if (LED2 & !LED1)
    {
      current = currentLinearCalibration(0.0346,portRead,-23.736);
    }
    else
    {
      current = 0.00;
    }

    totalCurrent = totalCurrent + current; 
    int sampleRate = 50;
    if (meterCount == sampleRate)
    {

        //float current = currentCalibration(0.0312,portRead,-21.496); 
      //  float current = currentCalibration(0.0264,portRead,-18.058); 

      //These m & b values take all the combinations of the 75,100,150,75+100,100+150,75+150
       // float current = currentCalibration(0.0269,portRead,-18.43); 

        float avgCurrent = totalCurrent/sampleRate;
     //  float current = currentCalibration(1,portRead,0); 
        Serial.print("Analog Read: ");
        Serial.println(avgCurrent);
        
        Serial.print("E.D. Published: ");
        Serial.print("Analog Read");
        Serial.print("\n");
       // client.publish("topic/3", msg);
        String pubString = "Current Value: " + String(avgCurrent) + "\n";
        pubString.toCharArray(msg, pubString.length()+1);
        Serial.println(pubString);
        client.publish("topic/3", msg);
        
      totalCurrent = 0; 
      meterCount = 0; 
    }
 // }
  
}



