/*
* Measure light level with an IR-transistor 
* and send the value via Mqtt.
* Also receives Mqtt messages and sends IR codes to the tv and more
* PHermansson 20161018
* 
* mosquitto_pub -h 192.168.1.79 -u 'emonpi' -P 'emonpimqtt2016' -t 'irsender' -m 'mess'
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
IRsend irsend(3); //an IR led is connected to GPIO pin 3
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
  //  Serial.begin(115200);
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);

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
  char message[10] ="";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message[i] = payload[i];
  }
  Serial.println(message);
  Serial.println();
     irsend.sendSAMSUNG(0xE0E040BF, 32);

    delay(40);

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
