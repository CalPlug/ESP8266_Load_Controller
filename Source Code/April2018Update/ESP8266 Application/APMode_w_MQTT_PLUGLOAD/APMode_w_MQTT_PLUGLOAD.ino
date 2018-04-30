/*
 *    MLC Code Modified by Paolo Caraos
 *    4/26/2018
 * 
 */

#include <EEPROM.h>
#include <WiFiManager.h>
#include <Button.h>
#include <FiniteStateMachine.h>
#include <LED.h>
#include <EMem.h>
#include <PubSubClient.h>
#include <Base64_MLC.h>  //the arduinobase64 library had to be modified to remove the need for the avr/progmem library
#include <AES.h>
#include <AES_config.h>

#define clip(x, min, max)      ((x) > (max))? (max) : ((x) < (min))? (min) : (x)

void loadoff();
void load1();
void load1off();
void load2();
void load2off();
void powerBoth();
void wifiConnected();
void wifiDisconnected();
void wifiSearching();
void upgradefirmware();
void wifi_AP();

//STATUS LED FSM States
State loadsOFF = State(loadoff, loadoff, load1);
State load1ON = State(load1, load1, load1off);
State load2ON = State(load2, load2, load2off);
State bothLoads = State(powerBoth, powerBoth, loadoff);

//WIFI LED STATES
State wifiDis = State(wifiDisconnected, wifiDisconnected, wifiConnected);
State wifiAP = State(wifi_AP, wifi_AP, wifiDisconnected);
State wifiConnect = State(wifiConnected, wifiConnected, wifiDisconnected);
State firmware = State(upgradefirmware);

//WIFI LED FSM
FSM wifiStatus = FSM(wifiDis);

//STATUS LED FSM
FSM loadStatus = FSM(loadsOFF);

LED redStatusLed = LED(5);
LED greenStatusLed = LED(0);
LED wifiLED = LED(4);

String RequestLEDoff1 =  "RqstOFF1!!!!!!!!";
String RequestLEDon1 =   "RqstON1!!!!!!!!!";
String StatusLEDoff1 =   "StatusOFF1!!!!!!";
String StatusLEDon1 =    "StatusON1!!!!!!!";
volatile bool LED1 = false;

String RequestLEDoff2 =  "RqstOFF2!!!!!!!!";
String RequestLEDon2 =   "RqstON2!!!!!!!!!";
String StatusLEDoff2 =   "StatusOFF2!!!!!!";
String StatusLEDon2 =    "StatusON2!!!!!!!";
volatile bool LED2 = false;

AES aes ;

String MAC_ID;
const char* mqtt_server = "m10.cloudmqtt.com";
const char* mqtt_port = "17934";
const char* mqtt_user = "dkpljrty";
const char* mqtt_pwd = "ZJDsxMVKRjoR";
byte AES_key[] = "2222222222222222";
const char* AES_IV = "1111111111111111";
const char* wifiMode = "tkip";

char configured[] = {'0', 0};

unsigned long mqtt_time;
unsigned long led_time;

const unsigned long SECONDS = 1000;
const unsigned long PERIOD = 10 * SECONDS;

WiFiManager wifiManager;
WiFiManagerParameter mqtt_server_param("mqtt_server", "mqtt_server", mqtt_server, 40);
WiFiManagerParameter mqtt_port_param("mqtt_port", "mqtt_port", mqtt_port, 40);
WiFiManagerParameter mqtt_user_param("mqtt_user", "mqtt_user", mqtt_user, 40);
WiFiManagerParameter mqtt_pwd_param("mqtt_pwd", "mqtt_pwd", mqtt_pwd, 50);
WiFiClient espClient;
PubSubClient client(espClient);
String MQTT_TOPIC_PUB = "out/devices/"; //append MAC_ID
String MQTT_TOPIC_SUB = "in/devices/"; //append MAC_ID

int buttonCount = 0;
boolean change = false;
int rst = 0;
int meterCount = 0;
boolean meterRead = false;
float totalCurrent = 0;
const int sampleRate = 50;

EMem emem;

Button button = Button(D4, PULLUP); //Bring to ground

void configModeCallback (WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
}


//STATUS LED FSM transition functions
void loadoff() {
  redStatusLed.off();
  greenStatusLed.off();
  LED1 = false;
  LED2 = false;
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  digitalWrite(14, LOW);
}

void load1() {
  greenStatusLed.on();
  redStatusLed.off();
  LED1 = true;
  LED2 = false;
  digitalWrite(12, HIGH);
  digitalWrite(13, LOW);
  digitalWrite(14, HIGH);
  //mqtt_pub();
}

void load1off() {
  greenStatusLed.off();
  LED1 = false;
  LED2 = false;
  digitalWrite(12, LOW);
  //  digitalWrite(13,LOW);
  digitalWrite(14, LOW);
}

void load2() {
  greenStatusLed.off();
  redStatusLed.on();
  LED2 = true;
  LED1 = false;
  digitalWrite(12, LOW);
  digitalWrite(13, HIGH);
  digitalWrite(14, HIGH);
}
void load2off() {
  redStatusLed.off();
  LED2 = false;
  LED1 = false;
  //  digitalWrite(12,LOW);
  digitalWrite(13, LOW);
  digitalWrite(14, LOW);
}

void powerBoth() {
  greenStatusLed.on();
  delay(50);
  greenStatusLed.off();
  delay(50);
  redStatusLed.on();
  delay(50);
  redStatusLed.off();
  delay(50);
  LED1 = true;
  LED2 = true;
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  digitalWrite(14, HIGH);
}

//WIFI LED TRANSITION FUNCTIONS
void wifiConnected() {
  wifiLED.on();
  //Serial.println("Connected");
}

void wifi_AP() {
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

void wifiSearching() {
  wifiLED.blink(100);
  // Serial.println("Searching");
}

void upgradefirmware() {
  wifiLED.blink(70);
  // Serial.println("Firmware");
}

void wifiDisconnected() {
  wifiLED.off();
}

void wifiUpdate(int Mode) {
  //Serial.println("Wifi LED UPDATE");
  //Serial.println(Mode);
  switch (Mode) {
    case 0: wifiStatus.transitionTo(wifiAP); wifiStatus.update(); break;
    //case 1:wifiStatus.transitionTo(wifiSearch);wifiStatus.update();break;
    case 2: wifiStatus.transitionTo(wifiConnect); wifiStatus.update(); break;
    case 3: wifiStatus.transitionTo(wifiDis); wifiStatus.update(); break;
  }
  wifiStatus.update();

}

/***********************Current Calibration*****************************/
float currentLinearCalibration(float gain, float value, float offset)
{
  float current;
  current = gain * value + offset;
  return current;
}

float currentQuadCalibration(float gain2, float gain1, float value, float offset)
{
  float current;
  current = gain2 * value * value + gain1 * value + offset;
  return current;
}

String encodeMessage(byte key[], String msg)
{
  byte plain[msg.length() + 1];
  msg.getBytes(plain, msg.length() + 1);
  byte cipher [N_BLOCK] ;
  aes.set_key (key, 128) ;
  aes.encrypt (plain, cipher) ;

  aes.copy_n_bytes (plain, cipher, sizeof(plain)) ;

  char data [sizeof(cipher) + 1] = {0}; //make null character array
  for (int i = 0; i < sizeof(cipher); i++) //trasfer cipher into null character array
  {
    data[i] = char (cipher[i]); //should there be a char typecast?
  }
  int inputLen = sizeof(cipher);
  int encodedLen = base64_enc_len(inputLen);
  char encoded[encodedLen];
  base64_encode(encoded, data, inputLen);
  return String(encoded);
}

String decodeMessage(byte key[], String msg, int msgLength)
{
  char encoded[msg.length() + 1];
  msg.toCharArray(encoded, msg.length() + 1);

  int input2Len = sizeof(encoded);
  int decodedLen = base64_dec_len(encoded, input2Len);
  char decoded[decodedLen];

  base64_decode(decoded, encoded, input2Len);

  String decodedString = String(decoded);

  byte cipher[decodedString.length() + 1];
  decodedString.getBytes(cipher, decodedString.length() + 1);

  byte check[msgLength];

  aes.set_key (key, 128) ;
  aes.decrypt(cipher, check) ;

  return String((char*) check).substring(0, msgLength);
}

/***********************************************************************************/
void callback(char* topic, byte* payload, unsigned int length1) {

  byte key[] = "2222222222222222";
  String encrypted_msg = (char *) payload;

  Serial.print("Payload Message: ");
  Serial.println(encrypted_msg);
  String decrypted_msg = decodeMessage(AES_key, encrypted_msg, 16);
  Serial.print("Decrypt Message: ");
  Serial.println(decrypted_msg);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.print("\n");

  int j = 0;
  int k = 0;
  int m = 0;
  int n = 0;

  Serial.println("Entering topic 1");

  if (decrypted_msg == RequestLEDon1)
  {
    LED1 = true;
  }

  if (decrypted_msg == RequestLEDoff1)
  {
    LED1 = false;
  }

  if (decrypted_msg == RequestLEDon2)
  {
    LED2 = true;
  }
  if (decrypted_msg == RequestLEDoff2)
  {
    LED2 = false;
  }
  if (!LED1 & !LED2) {

    loadStatus.transitionTo(loadsOFF); loadStatus.update();
    buttonCount = 0;
    meterRead = false;
  }
  else if (LED1 & LED2) {
    loadStatus.transitionTo(bothLoads); loadStatus.update();
    buttonCount = 3;
    meterRead = true;
  }
  else if (LED1 & !LED2) {
    loadStatus.transitionTo(load1ON); loadStatus.update();
    buttonCount = 1;
    meterRead = true;
  }
  else if (LED2 & !LED1) {
    //  loadStatus.transitionTo(loadsOFF);loadStatus.update();
    loadStatus.transitionTo(load2ON); loadStatus.update();
    buttonCount = 2;
    meterRead = true;
  }
  Serial.print("LED 1 ");
  Serial.println(LED1);
  Serial.print("LED 2 ");
  Serial.println(LED2);
}

unsigned long reconnect() //records how long each reconnect takes
{
  unsigned long timeElapsed = millis();
  if (!client.connected())
  {
    Serial.println("Attempting to connecting to MQTT...");
    if (client.connect(MAC_ID.c_str(), emem.getMqttUser().c_str(), emem.getMqttPwd().c_str()))
    {
      Serial.println("MQTT connected");

      //publish and subscribe to topics
      client.subscribe(String(MQTT_TOPIC_SUB + "1").c_str());
      client.subscribe(String(MQTT_TOPIC_SUB + "2").c_str());
      client.subscribe(String(MQTT_TOPIC_SUB + "3").c_str());
      client.publish(MQTT_TOPIC_PUB.c_str(), encodeMessage(AES_key, "Started").c_str());

      Serial.println("Published");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" will try connecting again in a few secs");
    }
  }
  return millis() - timeElapsed;
}

void EEPROMReset()
{
  WiFi.disconnect();//Close WiFi Connections
  delay(200);
  //noInterrupts(); //Disable further interrupt calls - Typically you want to turn this off, but this is the end of the line before a reset when an interrupt is called.
  int rst = 0;
  while (button.isPressed())
  {
    ESP.wdtFeed(); //Keep WDT Timer fed to prevent WDT restart
    delay(100); //This is in place to slow down the loop, it will freeze other operations!  Will either exit with a soft or hard reset
    rst++;
    if (rst >= 50) //Check to see if heald for more than 4 seconds, if so, hard reset
    { //reset after 40 loops with button press (4 seconds)

      Serial.println("EEPROM Reset (Hard or Soft) command received");
      //wifiManager.resetSettings();  //No need to use, you are just stacking deck chairs on the Titanic at this point, EEPROM will be erased
      Serial.println("Preparing to default EEPROM (Hard Reset)");

      delay(10);
      while (button.isPressed()) //inside the IF statement to check if button is still being pressed (pin pulled low)
      {
        //Stay in this loop until the button is released.  This prevents bootup issues if the pin is kept low at reset, this is an issue for GPIO2
        ESP.wdtFeed();
      }
      Serial.println("Now Resetting EEPROM to default and restarting (Hard Reset)");

      delay (20);
      char data[100] = "3#ssid#pw123456789#x#x#x#x#x#x"; //blank String with 0 as EEPROM config value
      emem.saveData(data);
      delay (500);

      Serial.println("EEPROM overwrite complete, restarting...");
      ESP.restart();  //Board will reset before leaving loop
      delay (2000);
    }
    else
    {} //Rounding out IF statement
  }
  Serial.print("Soft Reset command received!  Will be issued on Button release"); //should already be released, this is just a warning

  while (button.isPressed()) //inside the IF statement to check if button is still being pressed (pin pulled low)
  {
    //Stay in this loop until the button is released.  This prevents bootup issues if the pin is kept low at reset, this is an issue for GPIO2
    ESP.wdtFeed(); //Keep WDT Timer fed to prevent WDT restart
  }

  Serial.println("Soft Reset command received, restarting...");

  delay (500);
  ESP.restart(); //alternative approach is using [ESP.restart();] //Button not held for 4 seconds, loop terminates early - this provides dual function for this reset, soft and hard reset depending on hold time
  delay (2000);
}

void data_setup(char* data)
{
  char* sep = "#";
  strcat(data, configured);
  strcat(data, sep);
  strcat(data, wifiManager.getSSID().c_str());
  strcat(data, sep);
  strcat(data, wifiManager.getPassword().c_str());
  strcat(data, sep);
  strcat(data, mqtt_server_param.getValue());
  strcat(data, sep);
  strcat(data, mqtt_port_param.getValue());
  strcat(data, sep);
  strcat(data, mqtt_user_param.getValue());
  strcat(data, sep);
  strcat(data, mqtt_pwd_param.getValue());
  strcat(data, sep);
  Serial.println("Current values ready to be upated to EEPROM:");
  Serial.println(data);
  Serial.println();
}

void setup_wifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
  {
    Serial.println("no networks found");
    char data[100] = {};
    configured[0] = '0';
    data_setup(data);
    emem.saveData(data);
    ESP.restart();
    delay(1000);
  }
  else
  {
    Serial.print(n);
    Serial.println("Networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setTimeout(45); // 45 sec to timeout
  wifiUpdate(0);
  Serial.println("Setting up Wifi");
  String accessPointName = "ESP - " + MAC_ID;
  if (!wifiManager.autoConnect(accessPointName.c_str(), "")) { //credentials for SSID in AP mode

    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    char data[100] = {};
    configured[0] = emem.getConfigStatus().charAt(0) -  1;
    data_setup(data);
    emem.saveData(data);
    ESP.restart();
    delay(1000);
  }
  else {
    char data[100] = {};
    Serial.println("Connected. Saving to EEPROM and resetting.");

    configured[0] = '9';
    data_setup(data);
    emem.saveData(data);

    ESP.restart();
    delay(1000);
  }
  delay(1000);
}

void isr()
{
  Serial.println("interrupted");
}


void mqtt_pub() {
  char msg[50] = {};
  int value = 0;
  if (LED1) {

    String eM = encodeMessage(AES_key, StatusLEDon1);
    char state[eM.length() + 1];
    eM.toCharArray(state, eM.length() + 1);

    snprintf (msg, 75, state, value);
    // snprintf (msg, 75, eM, value);
    Serial.print("E.D. Published: ");
    Serial.print(StatusLEDon1);
    Serial.print("\n");
    client.publish(String(MQTT_TOPIC_PUB + "1").c_str(), eM.c_str());
  }
  else {

    String eM = encodeMessage(AES_key, StatusLEDoff1);
    char state[eM.length() + 1];
    eM.toCharArray(state, eM.length() + 1);

    snprintf (msg, 75, state, value);
    Serial.print("E.D. Published: ");
    Serial.print(StatusLEDoff1);
    Serial.print("\n");
    client.publish(String(MQTT_TOPIC_PUB + "1").c_str(), eM.c_str());
  }

  if (LED2) {

    String eM = encodeMessage(AES_key, StatusLEDon2);
    char state[eM.length() + 1];
    eM.toCharArray(state, eM.length() + 1);

    snprintf (msg, 75, state, value);
    Serial.print("E.D. Published: ");
    Serial.print(StatusLEDon2);
    Serial.print("\n");
    client.publish(String(MQTT_TOPIC_PUB + "2").c_str(), eM.c_str());
  }
  else {
    String eM = encodeMessage(AES_key, StatusLEDoff2);
    char state[eM.length() + 1];
    eM.toCharArray(state, eM.length() + 1);
    snprintf (msg, 75, state, value);
    Serial.print("E.D. Published: ");
    Serial.print(StatusLEDoff2);
    Serial.print("\n");
    client.publish(String(MQTT_TOPIC_PUB + "2").c_str(), eM.c_str());
  }
}

void APModeSetup()
{
  emem.loadData();
  Serial.println();
  Serial.println("First read");
  Serial.println("EEPROM recorded Configured state: " + emem.getConfigStatus());
  Serial.println("EEPROM recorded SSID: "+ emem.getWifiSsid());
  Serial.println("EEPROM recorded PWD: "+emem.getWifiPwd());
  Serial.println ("EEPROM recorded MQTT Server: "+emem.getMqttServer());
  Serial.println("EEPROM recorded MQTT Port: "+emem.getMqttPort());
  Serial.println("EEPROM recorded MQTT User: "+emem.getMqttUser());
  Serial.println("EEPROM recorded MQTT Pwd: "+emem.getMqttPwd());

  MAC_ID = WiFi.macAddress();
  MQTT_TOPIC_PUB += MAC_ID + "/";
  MQTT_TOPIC_SUB += MAC_ID + "/";

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(emem.getWifiSsid());

  if (emem.getConfigStatus().charAt(0) <= 48) //**ASCII value of '0' is 48;
  {
    Serial.println("Too many timeouts. Resuming without Wifi.");
  }
  else if (emem.getConfigStatus() != "9") //check first character in EEPROM array which is the configured status
  {
    Serial.println();
    Serial.println("Connection Time Out...");
    Serial.println("Enter AP Mode...");
    setup_wifi();

    Serial.println("Credentials set from user response in AP mode.");
    char data[100] = {};
    Serial.print("SSID: "); Serial.println(wifiManager.getSSID().c_str());
    Serial.print("Password: "); Serial.println(wifiManager.getPassword().c_str());
    WiFi.begin(wifiManager.getSSID().c_str(), wifiManager.getPassword().c_str());
    Serial.println("Configuration entered. Testing connection.");

    for (int j = 0; WiFi.status() != WL_CONNECTED; j++)
    {
      Serial.print(".");
      delay(1000);

      if (j >= 100) {
        Serial.println("Timeout initial connection to AP");
        configured[0] = emem.getConfigStatus().charAt(0) -  1;
        data_setup(data);
        emem.saveData(data);

        ESP.restart();
        delay(1000);
      }
    }
  }
  WiFi.begin(emem.getWifiSsid().c_str(), emem.getWifiPwd().c_str());
  Serial.println("Connected");
  delay(5000);
}

void setup()
{
  Serial.begin(115200);
  
  wifiManager.addParameter(&mqtt_server_param);
  wifiManager.addParameter(&mqtt_port_param);
  wifiManager.addParameter(&mqtt_user_param);
  wifiManager.addParameter(&mqtt_pwd_param);

  APModeSetup();

  //pinMode(D4, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(D4), isr, CHANGE);

  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);

  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  // pinMode(15, OUTPUT);

  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  digitalWrite(14, LOW);
  // digitalWrite(15,LOW);

  pinMode(A0, INPUT);

  mqtt_time = millis();
  led_time = millis();
}

void loop()
{
  unsigned long currTime = millis();
  // put your main code here, to run repeatedly:
  if (WiFi.status() == WL_CONNECTED)
  {
    wifiUpdate(2);
    //code in this block will run only if connected
    //do MQTT operations here periodically
    if (currTime >= mqtt_time)
    {
      unsigned long reconnectTime = clip(3 * reconnect(), 4 * SECONDS, 120 * SECONDS);
      client.loop();
      if (client.connected() && change)
      {
        //publish
        Serial.println(String(currTime) + " : Publishing!");
        if(LED1)
          client.publish(String(MQTT_TOPIC_PUB + "1").c_str(), encodeMessage(AES_key, "LED Configured").c_str());        
        if(LED2)
          client.publish(String(MQTT_TOPIC_PUB + "2").c_str(), encodeMessage(AES_key, "LED Configured").c_str());        
        change = false;
      }
      mqtt_time = currTime + reconnectTime;
    }
  }
  else
  {
    wifiUpdate(3); 
  }

  buttonCount = buttonCount % 4;
  switch (buttonCount) {
    case 0: loadStatus.transitionTo(loadsOFF); loadStatus.update(); break;
    case 1: loadStatus.transitionTo(load1ON); loadStatus.update(); break;
    case 2: loadStatus.transitionTo(load2ON); loadStatus.update(); break;
    case 3: loadStatus.transitionTo(bothLoads); loadStatus.update(); break;
  }

  if (button.uniquePress()) {
    wifiUpdate(3); 
    EEPROMReset();
  }

  //This part of the code reads from port ADC0 and publishes to topic 3
  
  meterCount++;
  int portRead = analogRead(A0);
  // float current = currentLinearCalibration(1,portRead,0);
  //  float current = currentCalibration(0.0346,portRead,-24.221);
  float current;
  if (LED1 & LED2)
  {
    if (portRead < 0.332)
    {
      current = currentQuadCalibration(-0.0177, 25.134, portRead, -8902.5);
    }
    else
    {
      current = currentLinearCalibration(0.0346, portRead, -24.221);
    }
  }
  else if (LED1 & !LED2)
  {
    current = currentLinearCalibration(0.0344, portRead, -23.613);
  }
  else if (LED2 & !LED1)
  {
    current = currentLinearCalibration(0.0346, portRead, -23.736);
  }
  else
  {
    current = 0.00;
  }

  totalCurrent = totalCurrent + current;
  if (meterCount == sampleRate)
  {
    //float current = currentCalibration(0.0312,portRead,-21.496);
    //  float current = currentCalibration(0.0264,portRead,-18.058);

    //These m & b values take all the combinations of the 75,100,150,75+100,100+150,75+150
    // float current = currentCalibration(0.0269,portRead,-18.43);

    float avgCurrent = totalCurrent / sampleRate;
    //  float current = currentCalibration(1,portRead,0);
    Serial.print("Analog Read: ");
    Serial.println(avgCurrent);

    Serial.print("E.D. Published: ");
    Serial.print("Analog Read");
    Serial.print("\n");
    String pubString = "Current Value: " + String(avgCurrent);
    Serial.println(pubString);
    if(WiFi.status() == WL_CONNECTED && client.connected())
      client.publish(String(MQTT_TOPIC_PUB + "3").c_str(), encodeMessage(AES_key, String(avgCurrent)).c_str());

    totalCurrent = 0;
    meterCount = 0;
  }
}
