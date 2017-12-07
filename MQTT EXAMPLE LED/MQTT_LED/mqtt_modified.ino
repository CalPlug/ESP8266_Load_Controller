/*subscription now functioning, tested with two computers
one running arduino, the other running processing*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "";
const char* password = "";
//const char* mqtt_server = "mqtt://192.168.1.115";
const char* mqtt_server = "m10.cloudmqtt.com";

WiFiClient espClient;
//PubSubClient client(mqtt_server, 15710,callback, espClient);
//PubSubClient client(WiFi.localIP(),15710,espClient);
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

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
  for (int i = 0; i < length1; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   
    } else {
    digitalWrite(BUILTIN_LED, HIGH);  
  }

}

/***********************************************************************************/
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting a conenection to MQTT...");

    if (client.connect("ESP8266Client", "client-app", "sd2016")) {
      Serial.println("connected");
      client.publish("topic/2", "publishing-yes");
      client.subscribe("topic/1");//subscribe to data from topic/1
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
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 15710);
  client.setCallback(*callback); //changed to pointer 
}
/***********************************************************************************/
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
   //   client.subscribe("topic/2");//subscribe to data from topic/2

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "publshing-yes #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("topic/2", msg);
  }
}
