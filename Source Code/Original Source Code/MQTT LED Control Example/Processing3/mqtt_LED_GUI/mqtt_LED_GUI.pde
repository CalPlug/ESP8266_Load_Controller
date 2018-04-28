// Modified by Nicholas Farabee on 2/25/2016
// --------------------------------------------------------------------------------
// Developed by Aditi Bhatia on 2/16/16-2/21/16
// Reference: https://github.com/256dpi/processing-mqtt

import mqtt.*;
MQTTClient client;

PImage LEDoff;
PImage LEDon;
PImage buttonOFF;
PImage buttonON;
Boolean button;
Boolean LED;
String subscribed_topic;
String published_topic;
String RequestLEDoff;
String RequestLEDon;
String StatusLEDoff;
String StatusLEDon;
PFont f;

float x = 10;
float y = 20;
float w = 100;
float h = 50;
float x1 = 135;
float y1= 20;
float w1 = 100;
float h1 = 50;
float x2 = 260;
float y2= 20;
float w2 = 100;
float h2 = 50;
float x4 = 80;
float y4 = 250;
float x5 = 150;
float y5 = 120;

void setup() {
  size(500,500);
  background(0);
  button = false;
  LED = false;
  LEDoff = loadImage("LED_OFF_Graphic.jpg");
  LEDon = loadImage("LED_ON_Graphic.jpg");
  buttonOFF = loadImage("ButtonOFF.png");
  buttonON = loadImage("ButtonON.png");
  
  RequestLEDoff = "RqstOFF";
  RequestLEDon = "RqstON";
  StatusLEDoff = "StatusOFF";
  StatusLEDon = "StatusON";
  
  client = new MQTTClient(this);
  subscribed_topic = "topic/1";
  published_topic = "topic/1";
  //client.connect("mqtt://mgdhulpk:1h44okAq546U@m10.cloudmqtt.com:15710", "processing");
  client.connect("mqtt://yhpjclsx:RwSAmqe-htvY@m10.cloudmqtt.com:13526", "processing");
  //client.connect("mqtt://client-app:sd2016@m10.cloudmqtt.com:15710", "processing");
  client.subscribe(subscribed_topic);
  client.publish(published_topic);
  
  f = createFont("Arial",16,true); 
}

void drawButton(){
  if(button)
    image(buttonON, x4, y4);
    //In here send message to MQTT requesting LED to be turned ON.
  else
    image(buttonOFF, x4, y4);
}

void drawLED(){
  if(LED)
    image(LEDon, x5-13, y5-20); 
  else
    image(LEDoff, x5, y5);
}

void draw() {
  background(0);
  drawLED();
  drawButton();
  fill(250);
  rect(x,y,w,h);
  rect(x1,y1,w1,h1);
  rect(x2,y2,w2,h2);
  
  textFont(f,16);                  
  fill(0);                         
  text("Publish",33,50);  
    
  textFont(f,16);                  
  fill(0);                         
  text("Read",165,50);  
    
  textFont(f,16);                  
  fill(0);                         
  text("Refresh",286,50);  
  // fill(200);
 }

void mousePressed(){
  if(mouseX>x && mouseX <x+w && mouseY>y && mouseY <y+h){
    println("Publishing...");
  }
  if(mouseX>x1 && mouseX <x1+w1 && mouseY>y1 && mouseY <y1+h1){
   println("Reading...",300,200);
  }
  if(mouseX>x4 && mouseX <x4+121 && mouseY>y4 && mouseY <y4+118){
   button = !button;
   publish2topic();
  }
}

void publish2topic() {
  if(button){
    client.publish(subscribed_topic, RequestLEDon);
    //client.publish(subscribed_topic, StatusLEDoff);
    println("Request LED ON");
  }
  else{
    client.publish(subscribed_topic, RequestLEDoff);
    //client.publish(subscribed_topic, StatusLEDon);
    println("Request LED OFF");
  }
}

void unsubscribeFromBroker() {
  client.unsubscribe(subscribed_topic);
  subscribed_topic = "NULL";
}

void messageReceived(String topic, byte[] payload){
  String s = new String(payload);
  if (topic.equals("topic/1") && StatusLEDon.equals(s)){
    LED = true;
  }
  else if (topic.equals("topic/1") && StatusLEDoff.equals(s)){
    LED = false;
  }
  println("new message: " + topic + " - " + new String(payload));
}