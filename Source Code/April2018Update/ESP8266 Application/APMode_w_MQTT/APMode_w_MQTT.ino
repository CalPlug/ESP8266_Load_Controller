#include <EEPROM.h>
#include <WiFiManager.h>
#include <Button.h>
#include <EMem.h>
#include <PubSubClient.h>

#define clip(x, min, max)      ((x) > (max))? (max) : ((x) < (min))? (min) : (x)

String MAC_ID;

const char* mqtt_server = "m10.cloudmqtt.com";
const char* mqtt_port = "17934";
const char* mqtt_user = "dkpljrty";
const char* mqtt_pwd = "ZJDsxMVKRjoR";
const char* AES_key ="2222222222222222";
const char* AES_IV ="1111111111111111";
const char* wifiMode = "tkip";

char configured[] = {'0', 0};

unsigned long mqtt_time;
const unsigned long SECONDS = 1000;
const unsigned long PERIOD = 10 * SECONDS;

WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient client(espClient);
String MQTT_TOPIC_PUB = "out/devices/"; //append MAC_ID
String MQTT_TOPIC_SUB = "in/devices/"; //append MAC_ID

int buttonCount = 0;
boolean change = false;
int rst = 0;

EMem emem;

Button button = Button(D4, PULLUP); //Bring to ground, reset button

void configModeCallback (WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
}

void callback(char* topic, byte* payload, unsigned int length)
{
  char message_buff[100] = {};
  int i = 0;
  Serial.print("Message arrived:  topic: " + String(topic));
  for (i = 0; i < length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  Serial.println(" >>>> MESSAGE: " + String(message_buff));
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
      client.publish(MQTT_TOPIC_PUB.c_str(), "encrypt this");

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
      ESP.reset();  //Board will reset before leaving loop
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
  ESP.reset(); //alternative approach is using [ESP.restart();] //Button not held for 4 seconds, loop terminates early - this provides dual function for this reset, soft and hard reset depending on hold time
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
  strcat(data, mqtt_server);
  strcat(data, sep);
  strcat(data, mqtt_port);
  strcat(data, sep);
  strcat(data, mqtt_user);
  strcat(data, sep);
  strcat(data, mqtt_pwd);
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
    ESP.reset();
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
  //wifiUpdate(0);
  Serial.println("Setting up Wifi");
  String accessPointName = "ESP - " + MAC_ID;
  if (!wifiManager.autoConnect(accessPointName.c_str(), "")) { //credentials for SSID in AP mode

    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    char data[100] = {};
    configured[0] = emem.getConfigStatus().charAt(0) -  1;
    data_setup(data);
    emem.saveData(data);
    ESP.reset();
    delay(1000);
  }
  else {
    char data[100] = {};
    Serial.println("Connected. Saving to EEPROM and resetting.");

    configured[0] = '9';
    data_setup(data);
    emem.saveData(data);

    ESP.reset();
    delay(1000);
  }
  delay(1000);
}

void isr()
{
  Serial.println("interrupted");
}

void APModeSetup()
{
  emem.loadData();
  Serial.println();
  Serial.println("First read");
  Serial.print("EEPROM recorded Configured state: "); Serial.println( emem.getConfigStatus());
  Serial.print("EEPROM recorded SSID: "); Serial.println(emem.getWifiSsid());
  Serial.print("EEPROM recorded PWD: "); Serial.println(emem.getWifiPwd());/*
  Serial.println(mqtt_server);
  Serial.println(mqtt_port);
  Serial.println(mqtt_user);
  Serial.println(mqtt_pwd);*/

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

        ESP.reset();
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

  APModeSetup();

  //pinMode(D4, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(D4), isr, CHANGE);

  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);

  mqtt_time = millis();
}

void loop() {
  unsigned long currTime = millis();
  // put your main code here, to run repeatedly:
  if (WiFi.status() == WL_CONNECTED) {
    //code in this block will run only if connected
    //do MQTT operations here periodically
    if (currTime >= mqtt_time)
    {
      unsigned long reconnectTime = clip(3 * reconnect(), 4 * SECONDS, 120 * SECONDS);
      client.loop();
      if (client.connected())
      {
        //publish
        Serial.println(String(currTime) + " : Publishing!");
        //client.publish(String(MQTT_TOPIC_PUB + "1").c_str(), "encypt this");
      }
      mqtt_time = currTime + reconnectTime;
    }
  }

  if (button.uniquePress()) {
    EEPROMReset();
  }
}
