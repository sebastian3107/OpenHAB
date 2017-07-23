#include <FadeLed.h> // Makes fading leds on an Arduino easy. By Timo Engelgeer (Septillion)
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
#define wifi_ssid     "Internet SSID"
#define wifi_password "password123"
#define mqtt_server   "192.168.0.100"

// Debug on=1 / off=0
#define TRACE 0

#define PWM_VALUE 63
int gamma_table[PWM_VALUE+1] =
{
  0,    1,    1,    2,    4,    6,    9,    13,  
  16,   21,   26,   31,   37,   44,   51,   58,  
  66,   74,   84,   93,   103,  114,  125,  136,  
  148,  161,  174,  188,  202,  217,  232,  248,  
  264,  281,  298,  316,  334,  353,  372,  392,  
  412,  433,  455,  477,  499,  522,  545,  569,  
  594,  619,  644,  670,  697,  724,  752,  780,  
  808,  837,  867,  897,  928,  959,  991,  1023  
};

// RGB FET
FadeLed redLed(13);    // Pin D7 (Arduino Pin 13)
FadeLed greenLed(12);  // Pin D6 (Arduino Pin 12)
FadeLed blueLed(14);   // Pin D5 (Arduino Pin 14)
int fadeTime = 1000;   // fade time in milliseconds

//adding this to bypass to problem of the arduino builder issue 50
void callback(char*topic, byte* payload,unsigned int length);
WiFiClient espClient;

// client parameters
PubSubClient client(mqtt_server, 1883, callback, espClient);

//MQTT last attemps reconnection number
long lastReconnectAttempt = 0;

// Flag to turn all LEDs on once after startup
boolean flag = true;

// Callback function, when the gateway receive an MQTT value on the topics subscribed this function is called
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.
  trc("Hey I got a callback ");
  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  
  // Conversion to a printable string
  p[length] = '\0';
  String callbackstring = String((char *) p);
  String topicNameRec = String((char*) topic);
  
  //launch the function to treat received data
  receivingMQTT(topicNameRec,callbackstring);

  // Free the memory
  free(p);
}

void changeColor(int redValue, int greenValue, int blueValue, int fadeTimeInSec){ // parameter expected in percent
  int red = map(redValue,0,100,0,PWM_VALUE);
  red = constrain(red,0,PWM_VALUE);
  int green = map(greenValue,0,100,0,PWM_VALUE);
  green = constrain(green,0,PWM_VALUE);
  int blue = map(blueValue,0,100,0,PWM_VALUE);
  blue = constrain(blue,0,PWM_VALUE);
 
  red   = gamma_table[red];
  green = gamma_table[green];
  blue  = gamma_table[blue];

  fadeTime = fadeTimeInSec * 1000;

  redLed.setTime(fadeTime,   true);
  greenLed.setTime(fadeTime, true);
  blueLed.setTime(fadeTime,  true);
 
  redLed.set(red);
  greenLed.set(green);
  blueLed.set(blue);
  
}

void setup()
{
  //Launch serial for debugging purposes
  Serial.begin(9600);

  // set fade time (true = constant fade time, without true = constant speed)
  redLed.setTime(fadeTime,   true);
  greenLed.setTime(fadeTime, true);
  blueLed.setTime(fadeTime,  true);
  
  redLed.begin(1023);
  delay(1500);
  
  //Begining wifi connection
  setup_wifi();
  delay(1500);
  lastReconnectAttempt = 0;
  
}

void setup_wifi() {
  redLed.begin(0);
  WiFi.mode(WIFI_STA);
  delay(20);
  // We start by connecting to a WiFi network
  trc("Connecting to ");
  trc(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    blueLed.begin(1023);
    delay(500);
    trc(".");
  }
  trc("WiFi connected");
}

boolean reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    trc("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("WiFi_RGB")) {
    // Once connected, publish an announcement...
      client.publish("outTopic","hello world");
      trc("connected");
      redLed.begin(0);
      greenLed.begin(1023);
      blueLed.begin(0);
      delay(5000);
      redLed.begin(0);
      greenLed.begin(0);
      blueLed.begin(0);
    //Topic subscribed so as to get data
    String topicNameRec = String("home/RGB/Color/");
    //Subscribing to topic(s)
    subscribing(topicNameRec);
    } else {
      trc("failed, rc=");
      trc(String(client.state()));
      trc(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      redLed.begin(1023);
      greenLed.begin(0);
      blueLed.begin(0);
      delay(5000);
    }
  }
  return client.connected();
}

void loop()
{
  FadeLed::update();
  
  //MQTT client connection management
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      trc("client mqtt not connected, trying to connect");
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // MQTT loop
    if (flag == true){
      flag = false;
      delay(3000);
      redLed.begin(0);
      greenLed.begin(0);
      blueLed.begin(0);
      delay(500);
      changeColor(100, 100, 100, 5);
    }
    client.loop();
  }
}

void subscribing(String topicNameRec){ // MQTT subscribing to topic
  char topicStrRec[26];
  topicNameRec.toCharArray(topicStrRec,26);
  // subscription to topic for receiving data
  boolean pubresult = client.subscribe(topicStrRec);
  if (pubresult) {
    trc("subscription OK to");
    trc(topicNameRec);
  }
}

void receivingMQTT(String topicNameRec, String callbackstring) {
  trc("Receiving data by MQTT");
  trc(topicNameRec);

  // Extract new RGB Values from String (in percent, %)
  int c1 = callbackstring.indexOf(';');
  int c2 = callbackstring.indexOf(';',c1+1);
  int c3 = callbackstring.indexOf(';',c2+1);
  int newRedValue   = callbackstring.toInt();
  int newGreenValue = callbackstring.substring(c1+1,c2).toInt();
  int newBlueValue  = callbackstring.substring(c2+1,c3).toInt();
  int fadeTimeInSec = callbackstring.substring(c3+1).toInt();

  changeColor(newRedValue, newGreenValue, newBlueValue, fadeTimeInSec);
}

//send MQTT data dataStr to topic topicNameSend
//void sendMQTT(String topicNameSend, String dataStr){
//
//    char topicStrSend[26];
//    topicNameSend.toCharArray(topicStrSend,26);
//    char dataStrSend[200];
//    dataStr.toCharArray(dataStrSend,200);
//    boolean pubresult = client.publish(topicStrSend,dataStrSend);
//    trc("sending ");
//    trc(dataStr);
//    trc("to ");
//    trc(topicNameSend);
//
//}

//trace
void trc(String msg){
  if (TRACE) {
  Serial.println(msg);
  }
}
