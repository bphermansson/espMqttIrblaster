// EspWifi
#include <ESP8266WiFi.h>
// Mqtt
#include <PubSubClient.h>
// Ir remote
#include <IRremoteESP8266.h>
IRsend irsend(3); //an IR led is connected to GPIO pin 3
int blueLed=15;

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
  digitalWrite(2, HIGH); 

  pinMode (blueLed, OUTPUT);
  digitalWrite(blueLed,HIGH);
  
  // Use Gpio3 (== RX on a Esp-01) for Ir transmitter
  //Serial.begin(115200);
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
  int repeats = longpress.toInt();
  Serial.print("Longpress: ");
  Serial.println(repeats);
  
  unsigned long decCode = hexToDec(irCode);
  Serial.print("decCode: ");
  Serial.println(decCode);

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
  
  digitalWrite(blueLed,LOW);

  for (int i = 0; i < repeats; i++) {
    //irsend.sendSAMSUNG(0xE0E040BF, 32);
    if (manucode=="1") {
      Serial.println("Send Samsung code");
      irsend.sendSAMSUNG(decCode, imessLen);
      // It works to send the dec equivalent of 0xE0E040BF (=3772793023)
      //irsend.sendSAMSUNG(3772793023, 32);
      delay(40);
    }
    else if (manucode=="2") {
      Serial.println("Send LG(NEC) code");
      irsend.sendNEC(decCode, imessLen);
      delay(40);
    }
    else if (manucode=="3") {
      Serial.println("Send Yamaha(NEC) code");
      irsend.sendNEC(decCode, imessLen);
      delay(40);
    }
  }
  digitalWrite(blueLed,HIGH);

  
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
