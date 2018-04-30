import mqtt.*;
MQTTClient client;

PImage LEDoff;
PImage LEDon;
PImage buttonOFF;
PImage buttonON;
Boolean button1, button2;
Boolean LED1, LED2; 
String subscribed_topic1, subscribed_topic2, subscribed_topic3;
String published_topic1, published_topic2, published_topic3;
String RequestLEDoff1, RequestLEDoff2;
String RequestLEDon1, RequestLEDon2;
String StatusLEDoff1, StatusLEDoff2;
String StatusLEDon1, StatusLEDon2;
String device_MAC_ID = "5C:CF:7F:12:14:B3";
String currentValue = "Current Value: 0.000\0"; 
String mqtt_user = "dkpljrty"; 
String mqtt_pwd = "ZJDsxMVKRjoR";

PFont f;

float x = 10, y = 20, w = 100, h = 50;
float x1 = 135, y1= 20, w1 = 100, h1 = 50;
float x2 = 260, y2= 20, w2 = 100, h2 = 50;

/*************Encryption************************/

import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import javax.xml.bind.DatatypeConverter;

String key = "2222222222222222";

//Based on the following example:  <a href="http://stackoverflow.com/questions/15554296/simple-java-aes-encrypt-decrypt-example" target="_blank" rel="nofollow">http://stackoverflow.com/questions/15554296/simple-java-aes-encrypt-decrypt-example</a>

String encodeMessage(String key, String message)
{
  try {
    SecretKeySpec skeySpec_encode = new SecretKeySpec(key.getBytes("UTF-8"), "AES");

    Cipher cipher_encode = Cipher.getInstance("AES/ECB/NOPADDING"); //AES-CBC with IV encoding, ECB is used without the IV, example shown on <a href="http://aesencryption.net/" target="_blank" rel="nofollow">http://aesencryption.net/</a> 
    cipher_encode.init(Cipher.ENCRYPT_MODE, skeySpec_encode);

    byte[] encrypted = cipher_encode.doFinal(message.getBytes());
    System.out.println("Encrypted String (base 64): "
      + DatatypeConverter.printBase64Binary(encrypted));
    //encode without padding: Base64.getEncoder().withoutPadding().encodeToString(encrypted));
    //encode with padding:  Base64.getEncoder().encodeToString(encrypted));
    String base64_encrypted = DatatypeConverter.printBase64Binary(encrypted);
    return base64_encrypted;
  }
  catch (Exception ex) {
    ex.printStackTrace();
  }
  return "0";
}

String decodeMessage(String key, String message)
{
   try
   {
    SecretKeySpec skeySpec_decode = new SecretKeySpec(key.getBytes("UTF-8"), "AES");

    Cipher cipher_decode = Cipher.getInstance("AES/ECB/NOPADDING");
    cipher_decode.init(Cipher.DECRYPT_MODE, skeySpec_decode);

    byte[] decrypted_original = cipher_decode.doFinal(DatatypeConverter.parseBase64Binary(message));
    String decrypt_originalString = new String(decrypted_original);
    return decrypt_originalString;
    
   } catch (Exception ex) {
    ex.printStackTrace();
  }
  return "0";
}
/**********************************************************/

void setup() {
  size(500,500);
  background(0);
  button1 = false;
  button2 = false; 
  LED1 = false;
  LED2 = false; 
  LEDoff = loadImage("LB_OFF_Graphic.jpg");
  LEDon = loadImage("LB_ON_Graphic.jpg");
  buttonOFF = loadImage("ButtonOFF.png");
  buttonON = loadImage("ButtonON.png");
  
  RequestLEDoff1 =  "RqstOFF1!!!!!!!!";
  RequestLEDon1 =   "RqstON1!!!!!!!!!";
  StatusLEDoff1 =   "StatusOFF1!!!!!!";
  StatusLEDon1 =    "StatusON1!!!!!!!";
  
  RequestLEDoff2 =  "RqstOFF2!!!!!!!!";
  RequestLEDon2 =   "RqstON2!!!!!!!!!";
  StatusLEDoff2 =   "StatusOFF2!!!!!!";
  StatusLEDon2 =    "StatusON2!!!!!!!";
  
  client = new MQTTClient(this);
  subscribed_topic1 = "out/devices/" + device_MAC_ID +"/1";
  published_topic1 = "in/devices/" + device_MAC_ID +"/1";
  subscribed_topic2 = "out/devices/" + device_MAC_ID +"/2";
  published_topic2 = "in/devices/" + device_MAC_ID +"/2";
  subscribed_topic3 = "out/devices/" + device_MAC_ID +"/3";
  published_topic3 = "in/devices/" + device_MAC_ID +"/3";
 
  client.connect("mqtt://"+mqtt_user+":"+mqtt_pwd+"@m10.cloudmqtt.com:17934", "processing");
  
  client.subscribe(subscribed_topic1);
  client.publish(published_topic1);
  client.subscribe(subscribed_topic2);
  client.publish(published_topic2);
  client.subscribe(subscribed_topic3);
  client.publish(published_topic3);
  
  f = createFont("Arial",16,true); 
}

float x4 = 90, y4 = 300, a4 = 60, b4 = 60; 
float x6 = 325, y6 = 300, a6 = 60, b6 = 60;

void drawButton(){
  if(button1)
    image(buttonON, x4, y4, a4, b4);
    //In here send message to MQTT requesting LED to be turned ON.
  else
    image(buttonOFF, x4, y4, a4, b4);
 
  if(button2)
    image(buttonON, x6, y6, a6, b6);
    //In here send message to MQTT requesting LED to be turned ON.
  else
    image(buttonOFF, x6, y6, a6, b6);
}

float x5 = 25, y5 = 100, a5 = 200, b5 = 200;
float x7 = 260, y7 = 100, a7 = 200, b7 = 200;

void drawLED(){
  if(LED1)
    image(LEDon, x5, y5, a5, b5); 
  else
    image(LEDoff, x5, y5, a5, b5);
  if(LED2)
    image(LEDon, x7, y7, a7, b7); 
  else
    image(LEDoff, x7, y7, a7, b7);
}

float x8 = 130, y8 = 450, a8 = 200, b8 = 200;

void drawCurrentValue(){
  PFont myFont = createFont("Georgia", 32); 
   fill(#FFFFFF);
    textFont(myFont); 
   text(currentValue, x8, y8);

 /* if (LED1 || LED2)
  {
    fill(#FFFFFF);
    textFont(myFont); 
    text(currentValue, x8, y8);
  }
  else 
  {
    fill(#FFFFFF);
    textFont(myFont);
    text(currentValue, x8, y8);
    //text("No Loads Connected", x8-20, y8);
  }
  */
  
}

void draw() {
  background(0);
  drawLED();
  drawButton();
  drawCurrentValue();
  
  PFont myFont = createFont("Georgia", 32); 
  textFont(myFont); 
  fill(#5FFFA6);
  text("Load 1", x5+50, 75);
  fill(#F0272A);
  text("Load 2", x7+50, 75); 
  
  
  
  /*
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
  */
 }

void mousePressed(){
  /*
  if(mouseX>x && mouseX <x+w && mouseY>y && mouseY <y+h){
    println("Publishing...");
  }
  if(mouseX>x1 && mouseX <x1+w1 && mouseY>y1 && mouseY <y1+h1){
   println("Reading...",300,200);
  }
  */
  if(mouseX>x4 && mouseX <x4+121 && mouseY>y4 && mouseY <y4+118){
   button1 = !button1;
   LED1 = !LED1; 
   publish2topic(button1,published_topic1,RequestLEDon1, RequestLEDoff1, 1);
  }
  if(mouseX>x6 && mouseX <x6+121 && mouseY>y6 && mouseY <y6+118){
   button2 = !button2;
   LED2 = !LED2; 
   publish2topic(button2,published_topic2,RequestLEDon2, RequestLEDoff2, 2);
  }
}

void publish2topic(boolean button, String subscribed_topic, String RequestLEDon, String RequestLEDoff, int LED) {
  if(button){
    client.publish(subscribed_topic, encodeMessage(key,RequestLEDon));
    //client.publish(subscribed_topic, RequestLEDon);
    println("Request LED", LED, " On");
  }
  else{    
    client.publish(subscribed_topic, encodeMessage(key,RequestLEDoff));
    //client.publish(subscribed_topic, RequestLEDoff);
    println("Request LED", LED, " Off");
  }
}

void unsubscribeFromBroker() {
  client.unsubscribe(subscribed_topic1);
  subscribed_topic1 = "NULL";
  client.unsubscribe(subscribed_topic2);
  subscribed_topic2 = "NULL";
}

void messageReceived(String topic, byte[] payload){
  String s = new String(payload);
  
  println("Payload: ", s);
  s = decodeMessage(key, s); 

  if (topic.equals(subscribed_topic1) && StatusLEDon1.equals(s)){
    button1 = true;
    LED1 = true;
  }
  else if (topic.equals(subscribed_topic1) && StatusLEDoff1.equals(s)){
    button1 = false; 
    LED1 = false;
  }
  if (topic.equals(subscribed_topic2) && StatusLEDon2.equals(s)){
    button2 = true; 
    LED2 = true;
  }
  else if (topic.equals(subscribed_topic2) && StatusLEDoff2.equals(s)){
    button2 = false; 
    LED2 = false;
  }
  if (topic.equals(subscribed_topic3))
  {
    currentValue = "Current = " + s.substring(0,5);  
  }
  
  println("new message: " + topic + " - " + currentValue);
}
