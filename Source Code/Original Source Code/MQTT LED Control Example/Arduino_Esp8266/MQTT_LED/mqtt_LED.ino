/*
 * will print out any messages from subscription to topic/2
 * topic/1 is where it's sending out messages
 * client is client-app
 * Developed by Aditi Bhatia 2/16/16 - 2/22/16

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

String RequestLEDoff;
String RequestLEDon;
String StatusLEDoff;
String StatusLEDon;
const char* ssid = "UCInet Mobile Access";
const char* password = "";
//const char* mqtt_server = "mqtt://192.168.1.115";  //Example Mosquitto MQTT server running local on a PC
const char* mqtt_server = "m10.cloudmqtt.com";  //Using the cloud-based server at cloudmqtt.com

WiFiClient espClient;
//PubSubClient client(mqtt_server, 15710,callback, espClient);
//PubSubClient client(WiFi.localIP(),15710,espClient);
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
boolean LED;

void setup_wifi() { //because it needs wifi to connect to

  delay(10);
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
  Serial.println(WiFi.localIP());
}
/***********************************************************************************/
void callback(char* topic, byte* payload, unsigned int length1) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  int j = 0;
  int k = 0;
  int m = 0;
  int n = 0;
  for (int i = 0; i < RequestLEDon.length(); i++){
    if((char)payload[i] == RequestLEDon[i]){
      j++;
    } 
  }
  if(j == RequestLEDon.length()){
    digitalWrite(13, HIGH);
    LED = true;
  }
  
  for (int i = 0; i < RequestLEDoff.length(); i++){
    if((char)payload[i] == RequestLEDoff[i]){
      k++;
    } 
  }
  if(k == RequestLEDoff.length()){
    digitalWrite(13, LOW);
    LED = false;
  }

  for (int i = 0; i < RequestLEDoff.length(); i++){
    if((char)payload[i] == RequestLEDoff[i]){
      k++;
    } 
  }
  if(k == RequestLEDoff.length()){
    digitalWrite(13, LOW);
    LED = false;
  }
    
  /* 
  //arr[length1] = '\0';
  String s(arr);
  Serial.println();
  if(s.equals(RequestLEDon)){
    digitalWrite(13, HIGH);
    LED = true;
  }
  else if(s.equals(RequestLEDoff)){
    digitalWrite(13, LOW);
    LED = false;
  }*/
  /*if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   
    } else {
    digitalWrite(BUILTIN_LED, HIGH);  
  }*/

}

/***********************************************************************************/
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting a conenection to MQTT...");

    if (client.connect("ESP8266Client", "yhpjclsx", "RwSAmqe-htvY")) {
      Serial.println("connected");
      client.publish("topic/1", "publishing-yes");
      client.subscribe("topic/1");//subscribe to data from topic/2
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" will try connecting again in 5 secs");
      delay(5000);
    }
  }
}

/***********************************************************************************/
void setup() {
  pinMode(13, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 13526);
  client.setCallback(callback);
  RequestLEDoff = "RqstOFF";
  RequestLEDon = "RqstON";
  StatusLEDoff = "StatusOFF";
  StatusLEDon = "StatusON";
  LED = false;
}
/***********************************************************************************/
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
   //client.subscribe("topic/2");//subscribe to data from topic/2

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    
    if(LED){
      snprintf (msg, 75, "StatusON", value);
      Serial.print("E.D. Published: ");
      Serial.print(StatusLEDon);
      client.publish("topic/1", msg);
    }
    else{
      snprintf (msg, 75, "StatusOFF", value);
      Serial.print("E.D. Published: ");
      Serial.print(StatusLEDoff);
      client.publish("topic/1", msg);
    }
  }
}
