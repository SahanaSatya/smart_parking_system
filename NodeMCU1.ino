#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

/////////*    Network Configuration     */////////////

#define WIFI_SSID        "preva_AP1"             //WiFi SSID name
#define WIFI_PASSWORD    "bsnl@247"         //WiFi Pasword

///////*     Access  Token From WISE3   *////////////

#define TOKEN "owtWkjz5n2F6p1FQkBEU"        //access Token ID from the wise3 Server
char wise3Server[] = "wise3.prevasystems.net";      //Wise3 Server address

///////*     DHT11 setup                *////////////

#include "DHT.h"
#define DHTPIN D0                           //Pin D0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//light sensor setup
#define light_sensor D1
#define led_for_light1 D2
#define led_for_light2 D8
#define led_for_light3 10   //SD3

//smoke sensor setup
#define smoke_analog_sensor A0
#define buzzer D4
#define smoke_water_motor D5

// water(raindrop) sensor
#define water_sensor D6
#define waterlog_led D7

// PARAMETERS TO BE UPLOADED
float h, t; //  to read humidity and temperature as Celsius
int LDR = 0;
String light_message;
int smoke_value;
String   smoke_message;
int waterlog;
String   waterlog_message;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

unsigned long lastSend;

void setup()
{
  Serial.begin(9600);                       // Initializing the Serial communication
  dht.begin();

  pinMode (light_sensor , INPUT);
  pinMode (led_for_light1 , OUTPUT);
  pinMode (led_for_light2 , OUTPUT);
  pinMode (led_for_light3 , OUTPUT);

  pinMode (smoke_analog_sensor , INPUT);
  pinMode (buzzer , OUTPUT);
  pinMode (smoke_water_motor , OUTPUT);

  pinMode (water_sensor , INPUT);
  pinMode (waterlog_led , OUTPUT);

  delay(10);
  InitWiFi();                                 // Setting up the network
  client.setServer( wise3Server, 1883 );      // Connecting with Wise3 Server
  lastSend = 0;

}

void loop()
{
  if ( !client.connected() )
  {
    reconnect();
  }

  //reading temperature and humidity
  Serial.println("Collecting temperature data.");
  h = dht.readHumidity();                       // Reading humidity

  t = dht.readTemperature();                    // Read temperature as Celsius

  if (isnan(h) || isnan(t))                           // Check if any reads failed and exit early (to try again).
  {
    Serial.println("Failed to read from DHT sensor!");

  }

  Serial.println("temperature: ");
  Serial.print(t);
  Serial.println("humidity");
  Serial.print(h);
  delay(1000);//Function call

  //LIGHT SENSOR
  LDR = digitalRead(light_sensor);
  if (LDR == 1)
  {
    Serial.println("lighting is required and the leds are on");
    digitalWrite(led_for_light1, HIGH);
    digitalWrite(led_for_light2, HIGH);
    digitalWrite(led_for_light3, HIGH);
    light_message = "\"Dark\"";
  }
  else
  {
    Serial.println("lighting is required and the leds are off");
    digitalWrite(led_for_light1, LOW);
    digitalWrite(led_for_light2, LOW);
    digitalWrite(led_for_light3, LOW);
    light_message = "\"Bright\"";
  }


  //SMOKE SENSOR
  smoke_value = analogRead(smoke_analog_sensor);

  if ( smoke_value > 450) //100-150 is normal air range
  {
    Serial.println("smoke has been detected");
    smoke_message = "\"smoke detected\"";
    buzzer_fn();
    digitalWrite(smoke_water_motor, HIGH);
    delay(1000);
    digitalWrite(smoke_water_motor, LOW);
  }
  else
  {
    Serial.println("NO SMOKE DETECTED");
    smoke_message = "\"no smoke\"";
  }

  //WATER(RAINDROP) SENSOR
  waterlog = digitalRead(water_sensor);
  if ( waterlog == 0)
  {
    Serial.println("water is detected in the parking slot");
    digitalWrite(waterlog_led, HIGH);
    waterlog_message = "\"waterlog present\"";
  }
  else
  {
    Serial.println("there is no water in the parking slot");
    digitalWrite(waterlog_led, LOW);
    waterlog_message = "\"no waterlog\"";
  }
payload_upload();
  client.loop();
}
void buzzer_fn()
{
  digitalWrite(buzzer, HIGH);
  delay(500);
  digitalWrite(buzzer, LOW);
  delay(500);
}
void payload_upload()                           //Function to collect the Temperature and Humidity
{

  String payload = "{";
  payload += "\"temperature\":";
  payload += t;
  payload += ",";
  payload += "\"humidity\":";
  payload += h;
  payload += ",";
  payload += "\"LDR\":";
  payload += LDR;
  payload += ",";
  payload += "\"Messsage_for_Lighting\":";
  payload += light_message;
  payload += ",";
  payload += "\"smoke_senor_value\":";
  payload += smoke_value;
  payload += ",";
  payload += "\"Messsage_for_Smoke\":";
  payload += smoke_message;
  payload += ",";
  payload += "\"waterlog\":";
  payload += waterlog;
  payload += ",";
  payload += "\"waterlog_message\":";
  payload += waterlog_message;
  payload += "}";

  //     Send payload to the WISE3 telemetry

  char att[300];
  payload.toCharArray( att, 300 );
  client.publish( "v1/devices/me/telemetry", att );
  Serial.println( att );

}

void InitWiFi()                                       //Setup to connect through WiFi network
{
  Serial.println("Connecting to AP ...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    buzzer_fn();
  }
  Serial.println("Connected to AP");
}


void reconnect()                                      //Reconnecting with MQTT
{
  while (!client.connected())
  {

    if (  WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED)
      {
        Serial.print(".");
        buzzer_fn();
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to WISE3 ...");
    // Attempt to connect (clientId, username, password)
    if ( client.connect("ESP8266 Device", TOKEN, NULL) ) {
      Serial.println( "[DONE]" );
      buzzer_fn();
      buzzer_fn();
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print(client.state());
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}
