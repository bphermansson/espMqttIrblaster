/*
* Measure light level with an IR-transistor 
* and send the value via Mqtt.
* Also receives Mqtt messages and sends IR codes to the tv and more
* PHermansson 20161018
* 
* Send Mqtt messages like this "Manufacturer code, ir code, code length"
* Manufacturer code are 1 for Samsung, 2 for LG
* 
* ir code 
* Find your remote at http://lirc.sourceforge.net/remotes/
* Note bits, pre_data_bits, pre_data and a code
* Example, Samsung BN59-00538A. 
* bits = 16, pre_data_bits = 16, pre_data = 0xE0E0, power on/off code = 0x40BF
* 
* Then the message to send is Manu code, pre_data+code, pre_data_bits+bits, longpress (0 or 1) =
* "1,E0E040BF,32,1"
* 
* Longpress gives a longer transmission, sometimes needed to turn of equipment.
* 
* mosquitto_pub -h 192.168.1.79 -u 'emonpi' -P 'emonpimqtt2016' -t 'irsender' -m '1,E0E040BF,32,1'
*/

/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

// EspWifi
#include <ESP8266WiFi.h>
// Mqtt
#include <PubSubClient.h>
// Ir remote
#include <IRremoteESP8266.h>
IRsend irsend(13); //an IR led is connected to GPIO pin 3
unsigned int Samsung_power_toggle[71] = {38000,1,1,170,170,20,63,20,63,20,63,20,20,20,20,20,20,20,20,20,20,20,63,20,63,20,63,20,20,20,20,20,20,20,20,20,20,20,20,20,63,20,20,20,20,20,20,20,20,20,20,20,20,20,63,20,20,20,63,20,63,20,63,20,63,20,63,20,63,20,1798};


// Update these with values suitable for your network.
const char* ssid = "NETGEAR83";
const char* password = "........";
const char* mqtt_server = "192.168.1.79";

// Mqtt topic to listen to
const char* mqtt_sub_topic = "irsender";
const char* mqtt_pub_topic = "lightlevel1";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup() {
  // Setup Gpio 2
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW); 
  
  // Use Gpio3 (== RX) for Ir transmitter
  Serial.begin(115200);
  //Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);

  setup_wifi();

  // Mqtt
  client.setServer(mqtt_server, 1883);
  // Mqtt callback for incoming messages
  client.setCallback(callback);

  // Initialize irsender
  irsend.begin();

}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
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

void callback(char* topic, byte* payload, unsigned int length) {
  // This is called when a message with the correct topic arrives
  char message[14] ="";
  Serial.print("Message arrived topic=");
  Serial.println(topic);
  // Convert to correct format
  for (int i = 0; i < length; i++) {
    message[i] = payload[i];
  }
  String sMess(message);
  Serial.print("Message: ");
  Serial.println( sMess );

  // Manufacturers are listed here:
  // https://github.com/markszabo/IRremoteESP8266/blob/master/IRremoteESP8266.h
  String manucode = sMess.substring(0,1);
  Serial.print("Manu code: ");
  Serial.println( manucode );
  
  String irCode = sMess.substring(2,10);
  Serial.print("irCode: ");
  Serial.println( irCode );

  String messLen = sMess.substring(11,13);
  int imessLen = messLen.toInt();
  Serial.print("Code length: ");
  Serial.println(imessLen);

  String longpress = sMess.substring(14,15);
  int ilongpress = longpress.toInt();
  Serial.print("Longpress: ");
  Serial.println(ilongpress);
  
  unsigned long decCode = hexToDec(irCode);
  Serial.print("decCode: ");
  Serial.println(decCode);
  int repeats;
  if (ilongpress==1) {
    repeats = 5;
  }
  else {
    repeats = 3;
  }
  for (int i = 0; i < repeats; i++) {
      //irsend.sendSAMSUNG(0xE0E040BF, 32);
      if (manucode=="1") {
        
        irsend.sendSAMSUNG(decCode, imessLen);
        // It works to send the dec equivalent of 0xE0E040BF (=3772793023)
        //irsend.sendSAMSUNG(3772793023, 32);
        delay(100);
      }
      else if (manucode=="2") {
        irsend.sendLG(decCode, imessLen);
      }
    else {
      Serial.println("Incorrect manufacturer");
    }  
  
  }
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", "emonpi", "emonpimqtt2016")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_pub_topic, "hello from lightmeter");
      // ... and resubscribe
      client.subscribe(mqtt_sub_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;

    // Get light level
    int lightvalue = measureLight();
    Serial.print ("Light level from function: ");
    Serial.println(lightvalue);

    // Convert and publish to Mqtt broker
    sprintf(msg,"%d", lightvalue);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(mqtt_pub_topic, msg);
  }
}

int measureLight() {
   /* Measure light by testing how long time it takes for
    *  a capacitor to discharge through a ir transistor. 
    *  Makes it possible to measure this analog value without an Adc. 
    */
   // Set output high
    Serial.println("Gpio 2 high");
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    // Wait for capacitor to charge
    delay(500);          
    // Set Gpio2 to input
    digitalWrite(2, LOW);
    pinMode(2, INPUT);
  
    // Wait while capacitor discharges and count how long it takes
    unsigned long startTime = micros();
    // Measure discharge time and use a timeout. If its really dark, we get stuck here otherwise. 
    while (digitalRead(2) == HIGH && micros()-startTime < 1000000) {
    }
    unsigned int level = micros() - startTime;
    //Serial.print ("Light level: ");
    //Serial.println(level);
    return level;
}
unsigned long hexToDec(String hexString) {
  https://github.com/benrugg/Arduino-Hex-Decimal-Conversion/blob/master/hex_dec.ino
  unsigned long decValue = 0;
  int nextInt;
  
  for (int i = 0; i < hexString.length(); i++) {
    
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    
    decValue = (decValue * 16) + nextInt;
  }
  
  return decValue;
}
