This is an example MQTT application that uses Processing 3.0 to control an Arduino based IOT edge device.  To execute/program the code you must have Processing and Arduino IDE installed.  Specifically, a ESP8266 was used as the chipset for this example, using the Arduino IDE environment.  
This example does not include encryption (other than the inherent WiFi encryption).  It is a good example to learn about how MQTT operates.

The MQTT broker for this example is the free cloud based MQTT broker service cloudmqtt--free account  please make a new account to test this code.  Do not subscribe or publish to the test account that is included in the code!  This example requires the following library: https://github.com/knolleary/pubsubclient in addition to any device specific libraries.


This code was developed by UCI EECS 2016 Senior Design Team 22:
Katherine Khanh Dan Tran <katherkt@uci.edu> (Final Team Leader)
Sonum Rajan Hingorani <hingoran@uci.edu>
Nina Tamashiro <tamashin@uci.edu>
Aditi Bhatia <aditib@uci.edu>
in collaboration with Nicholas Farabee <nfarabee@uci.edu> (Processing component development and project integration), Michael Klopfer, PhD (team mentor), Smartenit, Inc. (team sponsor) , and Calit2 @ UC Irvine (team sponsor).

