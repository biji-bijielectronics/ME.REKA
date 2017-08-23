/*
  This sketch demonstrates the capabilities of the pubsub library in combination
  with the ESP8266 board/library.

  It connects to an MQTT server then:
  - publishes to the topic "timeElapsed" every one seconds
  - subscribes to the topic "onoff", "color" printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "onoff" is an 1, switch ON the ESP Led,
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
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

/************************* Adafruit.io Setup *********************************/
#define AIO_USERNAME    ""    //insert AIO_Username
#define AIO_KEY         ""   //insert Aio_Key

#define FEED_PATH AIO_USERNAME "/feeds/"
#define IN_FEED FEED_PATH "onoff"
#define IN_FEED_2 FEED_PATH "color"
#define IN_FEED_3 FEED_PATH "rainbow"
#define OUT_FEED FEED_PATH "timeElapsed"

/*************************** NeoPixel Setup **********************************/
#define PIN 1
#define NUM_LEDS 62
#define BRIGHTNESS 30

/************************* WiFi Access Point *********************************/
// Update these with values suitable for your network.
const char* ssid = "";   //insert wifi SSID
const char* password = "";   //insert wifi passwords
const char* mqtt_server = "io.adafruit.com";

/*************************** Global State ************************************/
WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);
long lastMsg = 0;
char msg[50];
int value = 0;
int state = 0;

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
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
  char hexstring[length];
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    hexstring[i] = (char)payload[i];
  }
  Serial.println(hexstring);

  // Switch on the LED if an 1 was received as first character
  if (topic[14] == 'o' && (char)payload[0] == '1') {
    state = 1;
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(255, 255, 255));
      strip.show();
    }
  }

  if (topic[14] == 'o' && (char)payload[0] == '0') {
    state = 0;
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
      strip.show();
    }
  }

  if (state == 1 && topic[14] == 'r' && (char)payload[0] == '1') {
    state = 1;
    rainbowCycle(10);
  }

  else if (state == 1 && topic[14] == 'c' && (char)payload[0] == '#') {
    long number = strtol( &hexstring[1], NULL, 16);
    // Split them up into r, g, b values
    long r = number >> 16;
    long g = number >> 8 & 0xFF;
    long b = number & 0xFF;
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(r, g, b));
      strip.show();
    }
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", AIO_USERNAME, AIO_KEY)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(OUT_FEED, "Time Elapsed");
      // ... and resubscribe
      client.subscribe(IN_FEED);
      client.subscribe(IN_FEED_2);
      client.subscribe(IN_FEED_3);
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
  if (state == 1) {
    long now = millis();
    if (now - lastMsg > 2000) {
      lastMsg = now;
      value +=2;
      snprintf (msg, 75, "%ld", value);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish(OUT_FEED, msg);
    }
  } else {value = 0;}
}
/************************** I'm Feeling Lucky ********************************/
// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256 ; j++) { // # cycles of all colors on wheel
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
