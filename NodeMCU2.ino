#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <time.h>

#define WIFI_AP "preva_AP1"
#define WIFI_PASSWORD "bsnl@247"

#define TOKEN "owtWkjz5n2F6p1FQkBEU"

int timezone = 5 * 3600;
int dst = 0;

char DemoServer[] = "wise3.prevasystems.net";

WiFiClient wifiClient;

PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;

// We assume that all GPIOs are LOW
boolean gpioState[] = {false};

// ir sensor
#define ir_sensor1 D0
#define ir_sensor2 D1
#define ir_sensor3 D2
#define ir_sensor4 D4

#define Slot_LED1 D5
#define Slot_LED2 D6
#define Slot_LED3 D7
#define Slot_LED4 D8

int sl1, sl2, sl3, sl4; // variables indicating led status

//start time and stop time of each users
int st1, st2, st3, st4; //start time in seconds
int sp1, sp2, sp3, sp4; //stop time in seconds
float parking_fee1 = 0, parking_fee2 = 0, parking_fee3 = 0, parking_fee4 = 0;

//user who is booking
int ub1 = 0, ub2 = 0, ub3 = 0;
String user_selection = "\"No user selected. Kindly select a user\"";

String slot_availability = "\" ";
int no_of_available_slots = 0;
String slot_status_a1 = "\"Initializing...\"", slot_status_a2 = "\"Initializing...\"";
String slot_status_b1 = "\"Initializing...\"", slot_status_b2 = "\"Initializing...\"";

#define paking_fee_per_minute 3

//flags for online booking and keeping the LEDs ON
int bkirps1 = 0, bkirps2 = 0, bkirps3 = 0, bkirps4 = 0;

//servo motor for toll gate on sd3 or gpio10
#include <Servo.h>

Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

int pos = 0;    // variable to store the servo position

String user = "\"Welcome users\"", user_status = "\"Initial set-up\"";
String Message_on_booking = "\"No message\"";
String Improper_parking = "\"No Problem\"";
float amount_to_be_paid, time_spent;
boolean flag_user1 = 0, flag_user2 = 0, flag_user3 = 0, flag_user4 = 0; //flag indicators for user entry and exit....0 indicates out of parking space
int PS1u1 = 0, PS2u1 = 0, PS3u1 = 0, PS4u1 = 0, PS1u2 = 0, PS2u2 = 0, PS3u2 = 0, PS4u2 = 0, PS1u3 = 0, PS2u3 = 0, PS3u3 = 0, PS4u3 = 0; //to know which user is booking which slot

String booking_status_s = "\"This is your booking Status\"", parking_status_s = "\"not entered\"";
float amount_s = 0, time_in_min_s = 0;

String booking_status_l = "\"This is your booking Status\"", parking_status_l = "\"not entered\"";
float amount_l = 0, time_in_min_l = 0;

String booking_status_t = "\"This is your booking Status\"", parking_status_t = "\"not entered\"";
float amount_t = 0, time_in_min_t = 0;

//flags to check if user who booked has entered and exited without parking in the slot; if yes, value =1
int ps1ub1 = 0, ps1ub2 = 0, ps1ub3 = 0, ps2ub1 = 0, ps2ub2 = 0, ps2ub3 = 0, ps3ub1 = 0, ps3ub2 = 0, ps3ub3 = 0, ps4ub1 = 0, ps4ub2 = 0, ps4ub3 = 0;

int count = 0;                                          // count = 0
char input[12];                                         // character array of size 12

unsigned long lastSend;

void setup()
{
  Serial.begin(9600);                       // Initializing the Serial communication

  pinMode (ir_sensor1 , INPUT);
  pinMode (ir_sensor2 , INPUT);
  pinMode (ir_sensor3 , INPUT);
  pinMode (ir_sensor4, INPUT);

  pinMode (Slot_LED1, OUTPUT);
  pinMode (Slot_LED2, OUTPUT);
  pinMode (Slot_LED3, OUTPUT);
  pinMode (Slot_LED4, OUTPUT);

  myservo.attach(10);  // attaches the servo on pin 10 or sd3 to the servo object

  delay(10);
  InitWiFi();
  client.setServer( DemoServer, 1883 );
  client.setCallback(on_message);

  configTime(timezone, dst, "asia.pool.ntp.org", "time.nist.gov");
  Serial.println("\nWaiting for Internet time");

  while (!time(nullptr)) {
    Serial.print("*");
    delay(1000);
  }
  Serial.println("\nTime response....OK");
}

void loop()
{
  if ( !client.connected() ) {
    reconnect();
  }
  //IR SENSOR
  if (digitalRead(ir_sensor1) == LOW)
  {
    Serial.println("the parking slot 1 is occupied");
    Serial.println("the number of slots has been reduced by 1");
    digitalWrite(Slot_LED1, HIGH);
    sl1 = 1;
    if (PS1u1)
    {
      parking_status_s = "\"parked properly\"";
      ps1ub1 = 0;
      Improper_parking = "\"No Problem\"";
    }
    if (PS1u2)
    {
      parking_status_l = "\"parked properly\"";
      ps1ub2 = 0;
      Improper_parking = "\"No Problem\"";
    }
    if (PS1u3)
    {
      parking_status_t = "\"parked properly\"";
      ps1ub3 = 0;
      Improper_parking = "\"No Problem\"";
    }
    bkirps1 = 0;//booked car has parked properly
  }
  else
  {
    if (bkirps1)
    {
      //slot is booked but not occupied

      if (PS1u1 && flag_user1)
      {
        parking_status_s = "\"entered but not parked properly\"";
        ps1ub1 = 1;
        Improper_parking = "\"user entered,vehicle not parked in A1\"";
      }
      if (PS1u2 && flag_user2)
      {
        parking_status_l = "\"entered but not parked properly\"";
        ps1ub2 = 1;
        Improper_parking = "\"user entered,vehicle not parked in A1\"";
      }
      if (PS1u3 && flag_user3)
      {
        parking_status_t = "\"entered but not parked properly\"";
        ps1ub3 = 1;
        Improper_parking = "\"user entered,vehicle not parked in A1\"";
      }
    }
    if (!bkirps1)
    {
      //check whether the slot is booked or not
      //if it's not booked, then proceed
      Serial.println("the parking slot 1 is  free to occupy");
      digitalWrite(Slot_LED1, LOW);
      sl1 = 0;
      Improper_parking = "\"No Problem\"";
      if (PS1u1)
      {
        parking_status_s = "\"removed vehicle,still in parking space\"";
      }
      if (PS1u2)
      {
        parking_status_l = "\"removed vehicle,still in parking space\"";
      }
      if (PS1u3)
      {
        parking_status_t = "\"removed vehicle,still in parking space\"";
      }
    }
  }

  if (digitalRead(ir_sensor2) == LOW)
  {
    Serial.println("the parking slot 2 is occupied");
    Serial.println("the number of slots has been reduced by 1");
    digitalWrite(Slot_LED2, HIGH);
    sl2 = 1;
    if (PS2u1)
    {
      parking_status_s = "\"parked properly\"";
      ps2ub1 = 0;
      Improper_parking = "\"No Problem\"";
    }
    if (PS2u2)
    {
      parking_status_l = "\"parked properly\"";
      ps2ub2 = 0;
      Improper_parking = "\"No Problem\"";
    }
    if (PS2u3)
    {
      parking_status_t = "\"parked properly\"";
      ps2ub3 = 0;
      Improper_parking = "\"No Problem\"";
    }
    bkirps2 = 0;
  }
  else
  {
    if (bkirps2)
    {

      if (PS2u1 && flag_user1)
      {
        parking_status_s = "\"entered but not parked properly\"";
        ps2ub1 = 1;
        Improper_parking = "\"user entered,vehicle not parked in A2\"";
      }
      if (PS2u2 && flag_user2)
      {
        parking_status_l = "\"entered but not parked properly\"";
        ps2ub2 = 1;
        Improper_parking = "\"user entered,vehicle not parked in A2\"";
      }
      if (PS2u3 && flag_user3)
      {
        parking_status_t = "\"entered but not parked properly\"";
        ps2ub3 = 1;
        Improper_parking = "\"user entered,vehicle not parked in A2\"";
      }
    }
    if (!bkirps2)
    {
      Serial.println("the parking slot 2 is  free to occupy");
      digitalWrite(Slot_LED2, LOW);
      sl2 = 0;
      Improper_parking = "\"No Problem\"";
      if (PS2u1)
      {
        parking_status_s = "\"removed vehicle,still in parking space\"";
      }
      if (PS2u2)
      {
        parking_status_l = "\"removed vehicle,still in parking space\"";
      }
      if (PS2u3)
      {
        parking_status_t = "\"removed vehicle,still in parking space\"";
      }
    }
  }

  if (digitalRead(ir_sensor3) == LOW)
  {
    Serial.println("the parking slot 3 is occupied");
    Serial.println("the number of slots has been reduced by 1");
    digitalWrite(Slot_LED3, HIGH);
    sl3 = 1;
    if (PS3u1)
    {
      parking_status_s = "\"parked properly\"";
      ps3ub1 = 0;
      Improper_parking = "\"No Problem\"";
    }
    if (PS3u2)
    {
      parking_status_l = "\"parked properly\"";
      ps3ub2 = 0;
      Improper_parking = "\"No Problem\"";
    }
    if (PS3u3)
    {
      parking_status_t = "\"parked properly\"";
      ps3ub3 = 0;
      Improper_parking = "\"No Problem\"";
    }
    bkirps3 = 0;
  }
  else
  {
    if (bkirps3)
    {
      //slot is booked but not occupied

      if (PS3u1 && flag_user1)
      {
        parking_status_s = "\"entered but not parked properly\"";
        ps3ub1 = 1;
        Improper_parking = "\"user entered,vehicle not parked in B2\"";
      }
      if (PS3u2 && flag_user2)
      {
        parking_status_l = "\"entered but not parked properly\"";
        ps3ub2 = 1;
        Improper_parking = "\"user entered,vehicle not parked in B2\"";
      }
      if (PS3u3 && flag_user3)
      {
        parking_status_t = "\"entered but not parked properly\"";
        ps3ub3 = 1;
        Improper_parking = "\"user entered,vehicle not parked in B2\"";
      }
    }
    if (!bkirps3)
    {
      Serial.println("the parking slot 3 is  free to occupy");
      digitalWrite(Slot_LED3, LOW);
      sl3 = 0;
      Improper_parking = "\"No Problem\"";
      if (PS3u1)
      {
        parking_status_s = "\"removed vehicle,still in parking space\"";
      }
      if (PS3u2)
      {
        parking_status_l = "\"removed vehicle,still in parking space\"";
      }
      if (PS3u3)
      {
        parking_status_t = "\"removed vehicle,still in parking space\"";
      }
    }
  }

  if (digitalRead(ir_sensor4) == LOW)
  {
    Serial.println("the parking slot 4 is occupied");
    Serial.println("the number of slots has been reduced by 1");
    digitalWrite(Slot_LED4, HIGH);
    sl4 = 1;
    if (PS4u1)
    {
      parking_status_s = "\"parked properly\"";
      ps4ub1 = 0;
      Improper_parking = "\"No Problem\"";
    }
    if (PS4u2)
    {
      parking_status_l = "\"parked properly\"";
      ps4ub2 = 0;
      Improper_parking = "\"No Problem\"";
    }
    if (PS4u3)
    {
      parking_status_t = "\"parked properly\"";
      ps4ub3 = 0;
      Improper_parking = "\"No Problem\"";
    }
    bkirps4 = 0;
  }
  else
  {
    if (bkirps4)
    {
      //slot is booked but not occupied

      if (PS4u1 && flag_user1)
      {
        parking_status_s = "\"entered but not parked properly\"";
        ps4ub1 = 1;
        Improper_parking = "\"user entered,vehicle not parked in B1\"";
      }
      if (PS4u2 && flag_user2)
      {
        parking_status_l = "\"entered but not parked properly\"";
        ps4ub2 = 1;
        Improper_parking = "\"user entered,vehicle not parked in B1\"";
      }
      if (PS4u3 && flag_user3)
      {
        parking_status_t = "\"entered but not parked properly\"";
        ps4ub3 = 1;
        Improper_parking = "\"user entered,vehicle not parked in B1\"";
      }
    }
    if (!bkirps4)
    {
      Serial.println("the parking slot 4 is  free to occupy");
      digitalWrite(Slot_LED4, LOW );
      sl4 = 0;
      Improper_parking = "\"No Problem\"";
      if (PS4u1)
      {
        parking_status_s = "\"removed vehicle,still in parking space\"";
      }
      if (PS4u2)
      {
        parking_status_l = "\"removed vehicle,still in parking space\"";
      }
      if (PS4u3)
      {
        parking_status_t = "\"removed vehicle,still in parking space\"";
      }
    }
  }
  slot_availability = "\" ";
  if (sl1==0)
  {
    slot_availability += "A1 ";
  }
  if (sl2==0)
  {
    slot_availability += "A2 ";
  }
  
  if (sl4==0)
  {
    slot_availability += "B1 ";
  }
  if (sl3==0)
  {
    slot_availability += "B2 ";
  }
  slot_availability += " \"";
  no_of_available_slots = (sl1 ? 0 : 1) + (sl2 ? 0 : 1) + (sl3 ? 0 : 1) + (sl4 ? 0 : 1);
  if (!(no_of_available_slots))
  {
    slot_availability = "\"Sorry...none of the slots are available\"";
  }
  if (sl1)
  {
    slot_status_a1 = "\"not available\"";
  }
  else
  {
    slot_status_a1 = "\"available\"";
  }
  if (sl2)
  {
    slot_status_a2 = "\"not available\"";
  }
  else
  {
    slot_status_a2 = "\"available\"";
  }
  if (sl3)
  {
    slot_status_b2 = "\"not available\"";
  }
  else
  {
    slot_status_b2 = "\"available\"";
  }
  if (sl4)
  {
    slot_status_b1 = "\"not available\"";
  }
  else
  {
    slot_status_b1 = "\"available\"";
  }
  //RFID user identification
  if (Serial.available() > 0)
  {
    count = 0;
    while (Serial.available() && count < 12)         // Read 12 characters and store them in input array
    {
      input[count] = Serial.read();
      delay(5);
      if (count == 0)
      {
        if (input[count] == '0')
        {
          user = "\"unauthorized user\"";
          user_status = "\"unauthorized\"";
          amount_to_be_paid = 0;
          time_spent = 0;
          Serial.println("u r an invalid user");
        }
      }
      if (count == 5)
      {

        if (input[count] == '8')
        {
          user = "\"Sahana\"";
          Serial.println("Hello Sahana");
          toll_gate();
          if (flag_user1 == 0)
          {
            parking_status_s = "\"You have entered the parking space\"";
            user_status = "\"Entry\"";
            //note the entry time if user has not booked any slot
            if (!(PS1u1 || PS2u1 || PS3u1 || PS4u1))
            {
              time_t now = time(nullptr);
              struct tm* p_tm = localtime(&now);
              st1 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// coverting into seconds
              amount_s = 0;
              time_in_min_s = 0;
            }
            amount_to_be_paid = 0;
            time_spent = 0;
            flag_user1 = 1;
          }
          else
          {
            parking_status_s = "\"You have exited from the parking space\"";
            booking_status_s = "\"Your booking is now open\"";
            //check if user has booked in a slot and leaving without parking in that slot
            if (ps1ub1 == 1)
            {
              sl1 = 0;
              digitalWrite(Slot_LED1, LOW);
            }
            if (ps2ub1 == 1)
            {
              sl2 = 0;
              digitalWrite(Slot_LED2, LOW);
            }
            if (ps3ub1 == 1)
            {
              sl3 = 0;
              digitalWrite(Slot_LED3, LOW);
            }
            if (ps4ub1 == 1)
            {
              sl4 = 0;
              digitalWrite(Slot_LED4, LOW);
            }
            PS1u1 = 0; //for booking next time
            PS2u1 = 0;
            PS3u1 = 0;
            PS4u1 = 0;
            //note the exit time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp1 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_s = (sp1 - st1) / 60 * paking_fee_per_minute;
            time_in_min_s = (sp1 - st1) / 60;
            parking_fee1 = (sp1 - st1) / 60 * paking_fee_per_minute;
            user_status = "\"Exit\"";
            amount_to_be_paid = parking_fee1;
            time_spent = (sp1 - st1) / 60;
            flag_user1 = 0;
          }
        }
        else if (input[count] == '6') {
          user = "\"Lokesh\"";
          Serial.println("hello lokesh");
          toll_gate();
          if (flag_user2 == 0)
          {
            parking_status_l = "\"You have entered the parking space\"";
            user_status = "\"Entry\"";
            //note the entry time if user has not booked any slot
            if (!(PS1u2 || PS2u2 || PS3u2 || PS4u2))
            {
              time_t now = time(nullptr);
              struct tm* p_tm = localtime(&now);
              st2 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
              amount_l = 0;
              time_in_min_l = 0;
            }
            amount_to_be_paid = 0;
            time_spent = 0;
            flag_user2 = 1;
          }
          else
          {
            parking_status_l = "\"You have exited from the parking space\"";
            booking_status_l = "\"Your booking is now open\"";
            //check if user has booked in a slot and leaving without parking in that slot
            if (ps1ub2 == 1)
            {
              sl1 = 0;
              digitalWrite(Slot_LED1, LOW);
            }
            if (ps2ub2 == 1)
            {
              sl2 = 0;
              digitalWrite(Slot_LED2, LOW);
            }
            if (ps3ub2 == 1)
            {
              sl3 = 0;
              digitalWrite(Slot_LED3, LOW);
            }
            if (ps4ub2 == 1)
            {
              sl4 = 0;
              digitalWrite(Slot_LED4, LOW);
            }
            PS1u2 = 0; //for booking next time
            PS2u2 = 0;
            PS3u2 = 0;
            PS4u2 = 0;
            //note the exit time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp2 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_l = (sp2 - st2) / 60 * paking_fee_per_minute;
            time_in_min_l = (sp2 - st2) / 60;
            parking_fee2 = (sp2 - st2) / 60 * paking_fee_per_minute;
            user_status = "\"Exit\"";
            amount_to_be_paid = parking_fee2;
            time_spent = (sp2 - st2) / 60;
            flag_user2 = 0;
          }
        }
        else if (input[count] == '7') {
          user = "\"Trishank\"";
          Serial.println("hello trishank");
          toll_gate();
          if (flag_user3 == 0)
          {
            parking_status_t = "\"You have entered the parking space\"";
            user_status = "\"Entry\"";
            //note the entry time if user has not booked any slot
            if (!(PS1u3 || PS2u3 || PS3u3 || PS4u3))
            {
              time_t now = time(nullptr);
              struct tm* p_tm = localtime(&now);
              st3 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
              amount_t = 0;
              time_in_min_t = 0;
            }
            amount_to_be_paid = 0;
            time_spent = 0;
            flag_user3 = 1;
          }
          else
          {
            parking_status_t = "\"You have exited from the parking space\"";
            booking_status_t = "\"Your booking is now open\"";
            //check if user has booked in a slot and leaving without parking in that slot
            if (ps1ub3 == 1)
            {
              sl1 = 0;
              digitalWrite(Slot_LED1, LOW);
            }
            if (ps2ub3 == 1)
            {
              sl2 = 0;
              digitalWrite(Slot_LED2, LOW);
            }
            if (ps3ub3 == 1)
            {
              sl3 = 0;
              digitalWrite(Slot_LED3, LOW);
            }
            if (ps4ub3 == 1)
            {
              sl4 = 0;
              digitalWrite(Slot_LED4, LOW);
            }
            PS1u3 = 0; //for booking next time
            PS2u3 = 0;
            PS3u3 = 0;
            PS4u3 = 0;
            //note the exit time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp3 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_l = (sp2 - st2) / 60 * paking_fee_per_minute;
            time_in_min_l = (sp2 - st2) / 60;
            parking_fee3 = (sp3 - st3) / 60 * paking_fee_per_minute;
            user_status = "\"Exit\"";
            amount_to_be_paid = parking_fee3;
            time_spent = (sp3 - st3) / 60;
            flag_user3 = 0;
          }
        }
        else
        {
          user = "\"unauthorized user\"";
          Serial.println("hello you're not a recognized user");
        }
      }
      count++;
    }
    Serial.println(input);
  }
  payload1();
  payload2();
  payload3();
  payloads();
  payloadl();
  payloadt();
  client.setCallback(on_message);
  client.loop();
}

void toll_gate()
{
  //open toll gate
  for (pos = 0; pos <= 90; pos += 1)
  { // goes from 0 degrees to 90 degrees  in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  delay(1000);
  //close gate
  for (pos = 90; pos >= 0; pos -= 1)
  { // goes from 90 degrees to 0 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}


// The callback for when a PUBLISH message is received from the server.
void on_message(const char* topic, byte* payload, unsigned int length) {

  Serial.println("On message");

  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(json);

  // Decode JSON request
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject((char*)json);

  if (!data.success())
  {
    Serial.println("parseObject() failed");
    return;
  }

  // Check request method
  String methodName = String((const char*)data["method"]);

  if (methodName.equals("setValuePS1u1"))
  {
    int x = data["params"];
    if (sl1 == 1)
    {
      //slot is not free to occupy
      if (x)
      {
        //user is trying to book
        Message_on_booking = "\"A1 cannot be booked\"";
        booking_status_s = "\"A1 cannot be booked\""; ////////booking a slot not available
      }
      if (!x)
      {
        //user is trying to cancel the booking
        if (PS1u1 == 1)
        {
          //Check if he's the user who booked, then check if he has not entered the parking lot
          if (flag_user1 == 0)
          {
            booking_status_s = "\"A1 booking is cancelled\"";
            parking_status_s = "\"You haven't entered the parking space\"";
            //note the cancellation time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp1 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_s = (sp1 - st1) / 60 * paking_fee_per_minute;
            time_in_min_s = (sp1 - st1) / 60;
            set_gpio_statusPS1(x);
            Message_on_booking = "\"A1 slot booking is cancelled\"";
            PS1u1 = 0;
          }
          else
          {
            //if the user is inside the parking space then he cannot cancel it
            booking_status_s = "\"Error!!!canceling not allowed\"";
          }
        }
        else if (PS1u2 || PS1u3)
        {
          //check if anyone has booked and this user is trying to cancel it
          booking_status_s = "\"Invalid action\"";
        }
      }
    }
    else
    {
      //slot is available for booking
      if (!x)
      {
        //user is trying to cancel the booking but slot is available
        booking_status_s = "\"Invalid action\"";
      }

      if (x)
      {
        //user is trying to book the slot
        if (PS2u1 || PS3u1 || PS4u1)
        {
          //check if he hasn’t booked any other slot
          booking_status_s = "\"cannot book more than one slot\""; ////////booking multiple slots
        }
        if (!(PS2u1 || PS3u1 || PS4u1))
        {
          //user has not booked any other slot
          if (flag_user1 == 0)
          {
            booking_status_s = "\"You have booked A1 slot\"";
            PS1u1 = 1; //indicating user 1 has booked slot ps1
            parking_status_s = "\"You haven't entered the parking space\"";
            //note the start time for booking
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            st1 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// converting into seconds
            amount_s = 0;
            time_in_min_s = 0;
            set_gpio_statusPS1(x);
            Message_on_booking = "\"A1 slot is booked\"";
          }
          else
          {
            booking_status_s = "\"Invalid booking\"";
            parking_status_s = "\"You are in the parking space\"";
          }
        }
      }
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
  if (methodName.equals("setValuePS2u1"))
  {
    int x = data["params"];
    if (sl2 == 1)
    {
      //slot is not free to occupy
      if (x)
      {
        //user is trying to book
        Message_on_booking = "\"A2 cannot be booked\"";
        booking_status_s = "\"A2 cannot be booked\""; ////////booking a slot not available
      }
      if (!x)
      {
        //user is trying to cancel the booking
        if (PS2u1 == 1)
        {
          //Check if he's the user who booked, then check if he has not entered the parking lot
          if (flag_user1 == 0)
          {
            booking_status_s = "\"A2 booking is cancelled\"";
            parking_status_s = "\"You haven't entered the parking space\"";
            //note the cancellation time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp1 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_s = (sp1 - st1) / 60 * paking_fee_per_minute;
            time_in_min_s = (sp1 - st1) / 60;
            set_gpio_statusPS2(x);
            Message_on_booking = "\"A2 slot booking is cancelled\"";
            PS2u1 = 0;
          }
          else
          {
            //if the user is inside the parking space then he cannot cancel it
            booking_status_s = "\"Error!!!canceling not allowed\"";
          }
        }
        else if (PS2u2 || PS2u3)
        {
          //check if anyone has booked and this user is trying to cancel it
          booking_status_s = "\"Invalid action\"";
        }
      }
    }
    else
    {
      //slot is available for booking
      if (!x)
      {
        //user is trying to cancel the booking
        booking_status_s = "\"Invalid action\"";
      }

      if (x)
      {
        //user is trying to book the slot
        if (PS1u1 || PS3u1 || PS4u1)
        {
          //check if he hasn’t booked any other slot
          booking_status_s = "\"cannot book more than one slot\""; ////////booking multiple slots
        }
        if (!(PS1u1 || PS3u1 || PS4u1))
        {
          //user has not booked any other slot
          if (flag_user1 == 0)
          {
            booking_status_s = "\"You have booked A2 slot\"";
            PS2u1 = 1; //indicating user 1 has booked slot ps2
            parking_status_s = "\"You haven't entered the parking space\"";
            //note the start time for booking
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            st1 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// converting into seconds
            amount_s = 0;
            time_in_min_s = 0;
            set_gpio_statusPS2(x);
            Message_on_booking = "\"A2 slot is booked\"";
          }
          else
          {
            booking_status_s = "\"Invalid booking\"";
            parking_status_s = "\"You are in the parking space\"";
          }
        }
      }
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
  if (methodName.equals("setValuePS3u1"))
  {
    int x = data["params"];
    if (sl3 == 1)
    {
      //slot is not free to occupy
      if (x)
      {
        //user is trying to book
        Message_on_booking = "\"B2 cannot be booked\"";
        booking_status_s = "\"B2 cannot be booked\""; ////////booking a slot not available
      }
      if (!x)
      {
        //user is trying to cancel the booking
        if (PS3u1 == 1)
        {
          //Check if he's the user who booked, then check if he has not entered the parking lot
          if (flag_user1 == 0)
          {
            booking_status_s = "\"B2 booking is cancelled\"";
            parking_status_s = "\"You haven't entered the parking space\"";
            //note the cancellation time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp1 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_s = (sp1 - st1) / 60 * paking_fee_per_minute;
            time_in_min_s = (sp1 - st1) / 60;
            set_gpio_statusPS3(x);
            Message_on_booking = "\"B2 slot booking is cancelled\"";
            PS3u1 = 0;
          }
          else
          {
            //if the user is inside the parking space then he cannot cancel it
            booking_status_s = "\"Error!!!canceling not allowed\"";
          }
        }
        else if (PS3u2 || PS3u3)
        {
          booking_status_s = "\"Invalid action\"";
        }
      }
    }
    else
    {
      //slot is available for booking
      if (!x)
      {
        //user is trying to cancel the booking
        booking_status_s = "\"Invalid action\"";
      }

      if (x)
      {
        //user is trying to book the slot
        if (PS1u1 || PS2u1 || PS4u1)
        {
          //check if he hasn’t booked any other slot
          booking_status_s = "\"cannot book more than one slot\""; ////////booking multiple slots
        }
        if (!(PS1u1 || PS2u1 || PS4u1))
        {
          //user has not booked any other slot
          if (flag_user1 == 0)
          {
            booking_status_s = "\"You have booked B2 slot\"";
            PS3u1 = 1; //indicating user 1 has booked slot ps3
            parking_status_s = "\"You haven't entered the parking space\"";
            //note the start time for booking
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            st1 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// converting into seconds
            amount_s = 0;
            time_in_min_s = 0;
            set_gpio_statusPS3(x);
            Message_on_booking = "\"B2 slot is booked\"";
          }
          else
          {
            booking_status_s = "\"Invalid booking\"";
            parking_status_s = "\"You are in the parking space\"";
          }
        }
      }
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
  if (methodName.equals("setValuePS4u1"))
  {
    int x = data["params"];
    if (sl4 == 1)
    {
      //slot is not free to occupy
      if (x)
      {
        //user is trying to book
        Message_on_booking = "\"B1 cannot be booked\"";
        booking_status_s = "\"B1 cannot be booked\""; ////////booking a slot not available
      }
      if (!x)
      {
        //user is trying to cancel the booking
        if (PS4u1 == 1)
        {
          //Check if he's the user who booked, then check if he has not entered the parking lot
          if (flag_user1 == 0)
          {
            booking_status_s = "\"B1 booking is cancelled\"";
            parking_status_s = "\"You haven't entered the parking space\"";
            //note the cancellation time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp1 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_s = (sp1 - st1) / 60 * paking_fee_per_minute;
            time_in_min_s = (sp1 - st1) / 60;
            set_gpio_statusPS4(x);
            Message_on_booking = "\"B1 slot booking is cancelled\"";
            PS4u1 = 0;
          }
          else
          {
            //if the user is inside the parking space then he cannot cancel it
            booking_status_s = "\"Error!!!canceling not allowed\"";
          }
        }
        else if (PS4u2 || PS4u3)
        {
          booking_status_s = "\"Invalid action\"";
        }
      }
    }
    else
    {
      //slot is available for booking
      if (!x)
      {
        //user is trying to cancel the booking
        booking_status_s = "\"Invalid action\"";
      }

      if (x)
      {
        //user is trying to book the slot
        if (PS1u1 || PS2u1 || PS3u1)
        {
          //check if he hasn’t booked any other slot
          booking_status_s = "\"cannot book more than one slot\""; ////////booking multiple slots
        }
        if (!(PS1u1 || PS2u1 || PS3u1))
        {
          //user has not booked any other slot
          if (flag_user1 == 0)
          {
            booking_status_s = "\"You have booked B1 slot\"";
            PS4u1 = 1; //indicating user 1 has booked slot ps4
            parking_status_s = "\"You haven't entered the parking space\"";
            //note the start time for booking
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            st1 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// converting into seconds
            amount_s = 0;
            time_in_min_s = 0;
            set_gpio_statusPS4(x);
            Message_on_booking = "\"B1 slot is booked\"";
          }
          else
          {
            booking_status_s = "\"Invalid booking\"";
            parking_status_s = "\"You are in the parking space\"";
          }
        }
      }
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }

  if (methodName.equals("setValuePS1u2"))
  {
    int x = data["params"];
    if (sl1 == 1)
    {
      //slot is not free to occupy
      if (x)
      {
        //user is trying to book
        Message_on_booking = "\"A1 cannot be booked\"";
        booking_status_l = "\"A1 cannot be booked\""; ////////booking a slot not available
      }
      if (!x)
      {
        //user is trying to cancel the booking
        if (PS1u2 == 1)
        {
          //Check if he's the user who booked, then check if he has not entered the parking lot
          if (flag_user2 == 0)
          {
            booking_status_l = "\"A1 booking is cancelled\"";
            parking_status_l = "\"You haven't entered the parking space\"";
            //note the cancellation time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp2 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_l = (sp2 - st2) / 60 * paking_fee_per_minute;
            time_in_min_l = (sp2 - st2) / 60;
            set_gpio_statusPS1(x);
            Message_on_booking = "\"A1 slot booking is cancelled\"";
            PS1u2 = 0;
          }
          else
          {
            //if the user is inside the parking space then he cannot cancel it
            booking_status_l = "\"Error!!!canceling not allowed\"";
          }
        }
        else if (PS1u1 || PS1u3)
        {
          booking_status_l = "\"Invalid action\"";
        }
      }
    }
    else
    {
      //slot is available for booking
      if (!x)
      {
        //user is trying to cancel the booking
        booking_status_l = "\"Invalid action\"";
      }

      if (x)
      {
        //user is trying to book the slot
        if (PS2u2 || PS3u2 || PS4u2)
        {
          //check if he hasn’t booked any other slot
          booking_status_l = "\"cannot book more than one slot\""; ////////booking multiple slots
        }
        if (!(PS2u2 || PS3u2 || PS4u2))
        {
          //user has not booked any other slot
          if (flag_user2 == 0)
          {
            booking_status_l = "\"You have booked A1 slot\"";
            PS1u2 = 1; //indicating user 2 has booked slot ps1
            parking_status_l = "\"You haven't entered the parking space\"";
            //note the start time for booking
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            st2 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// converting into seconds
            amount_l = 0;
            time_in_min_l = 0;
            set_gpio_statusPS1(x);
            Message_on_booking = "\"A1 slot is booked\"";
          }
          else
          {
            booking_status_l = "\"Invalid booking\"";
            parking_status_l = "\"You are in the parking space\"";
          }
        }
      }
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
  if (methodName.equals("setValuePS2u2"))
  {
    int x = data["params"];
    if (sl2 == 1)
    {
      //slot is not free to occupy
      if (x)
      {
        //user is trying to book
        Message_on_booking = "\"A2 cannot be booked\"";
        booking_status_l = "\"A2 cannot be booked\""; ////////booking a slot not available
      }
      if (!x)
      {
        //user is trying to cancel the booking
        if (PS2u2 == 1)
        {
          //Check if he's the user who booked, then check if he has not entered the parking lot
          if (flag_user2 == 0)
          {
            booking_status_l = "\"A2 booking is cancelled\"";
            parking_status_l = "\"You haven't entered the parking space\"";
            //note the cancellation time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp2 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_l = (sp2 - st2) / 60 * paking_fee_per_minute;
            time_in_min_l = (sp2 - st2) / 60;
            set_gpio_statusPS2(x);
            Message_on_booking = "\"A2 slot booking is cancelled\"";
            PS2u2 = 0;
          }
          else
          {
            //if the user is inside the parking space then he cannot cancel it
            booking_status_l = "\"Error!!!canceling not allowed\"";
          }
        }
        else if (PS2u1 || PS2u3)
        {
          booking_status_l = "\"Invalid action\"";
        }
      }
    }
    else
    {
      //slot is available for booking
      if (!x)
      {
        //user is trying to cancel the booking
        booking_status_l = "\"Invalid action\"";
      }

      if (x)
      {
        //user is trying to book the slot
        if (PS1u2 || PS3u2 || PS4u2)
        {
          //check if he hasn’t booked any other slot
          booking_status_l = "\"cannot book more than one slot\""; ////////booking multiple slots
        }
        if (!(PS1u2 || PS3u2 || PS4u2))
        {
          //user has not booked any other slot
          if (flag_user2 == 0)
          {
            booking_status_l = "\"You have booked A2 slot\"";
            PS2u2 = 1; //indicating user 2 has booked slot ps2
            parking_status_l = "\"You haven't entered the parking space\"";
            //note the start time for booking
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            st2 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// converting into seconds
            amount_l = 0;
            time_in_min_l = 0;
            set_gpio_statusPS2(x);
            Message_on_booking = "\"A2 slot is booked\"";
          }
          else
          {
            booking_status_l = "\"Invalid booking\"";
            parking_status_l = "\"You are in the parking space\"";
          }
        }
      }
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
  if (methodName.equals("setValuePS3u2"))
  {
    int x = data["params"];
    if (sl3 == 1)
    {
      //slot is not free to occupy
      if (x)
      {
        //user is trying to book
        Message_on_booking = "\"B2 cannot be booked\"";
        booking_status_l = "\"B2 cannot be booked\""; ////////booking a slot not available
      }
      if (!x)
      {
        //user is trying to cancel the booking
        if (PS3u2 == 1)
        {
          //Check if he's the user who booked, then check if he has not entered the parking lot
          if (flag_user2 == 0)
          {
            booking_status_l = "\"B2 booking is cancelled\"";
            parking_status_l = "\"You haven't entered the parking space\"";
            //note the cancellation time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp2 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_l = (sp2 - st2) / 60 * paking_fee_per_minute;
            time_in_min_l = (sp2 - st2) / 60;
            set_gpio_statusPS3(x);
            Message_on_booking = "\"B2 slot booking is cancelled\"";
            PS3u2 = 0;
          }
          else
          {
            //if the user is inside the parking space then he cannot cancel it
            booking_status_l = "\"Error!!!canceling not allowed\"";
          }
        }
        else if (PS3u1 || PS3u3)
        {
          booking_status_l = "\"Invalid action\"";
        }
      }
    }
    else
    {
      //slot is available for booking
      if (!x)
      {
        //user is trying to cancel the booking
        booking_status_l = "\"Invalid action\"";
      }

      if (x)
      {
        //user is trying to book the slot
        if (PS1u2 || PS2u2 || PS4u2)
        {
          //check if he hasn’t booked any other slot
          booking_status_l = "\"cannot book more than one slot\""; ////////booking multiple slots
        }
        if (!(PS1u2 || PS2u2 || PS4u2))
        {
          //user has not booked any other slot
          if (flag_user2 == 0)
          {
            booking_status_l = "\"You have booked B2 slot\"";
            PS3u2 = 1; //indicating user 2 has booked slot ps3
            parking_status_l = "\"You haven't entered the parking space\"";
            //note the start time for booking
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            st2 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// converting into seconds
            amount_l = 0;
            time_in_min_l = 0;
            set_gpio_statusPS3(x);
            Message_on_booking = "\"B2 slot is booked\"";
          }
          else
          {
            booking_status_l = "\"Invalid booking\"";
            parking_status_l = "\"You are in the parking space\"";
          }
        }
      }
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
  if (methodName.equals("setValuePS4u2"))
  {
    int x = data["params"];
    if (sl4 == 1)
    {
      //slot is not free to occupy
      if (x)
      {
        //user is trying to book
        Message_on_booking = "\"B1 cannot be booked\"";
        booking_status_l = "\"B1 cannot be booked\""; ////////booking a slot not available
      }
      if (!x)
      {
        //user is trying to cancel the booking
        if (PS4u2 == 1)
        {
          //Check if he's the user who booked, then check if he has not entered the parking lot
          if (flag_user2 == 0)
          {
            booking_status_l = "\"B1 booking is cancelled\"";
            parking_status_l = "\"You haven't entered the parking space\"";
            //note the cancellation time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp2 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_l = (sp2 - st2) / 60 * paking_fee_per_minute;
            time_in_min_l = (sp2 - st2) / 60;
            set_gpio_statusPS4(x);
            Message_on_booking = "\"B1 slot booking is cancelled\"";
            PS4u2 = 0;
          }
          else
          {
            //if the user is inside the parking space then he cannot cancel it
            booking_status_l = "\"Error!!!canceling not allowed\"";
          }
        }
        else if (PS4u1 || PS4u3)
        {
          booking_status_l = "\"Invalid action\"";
        }
      }
    }
    else
    {
      //slot is available for booking
      if (!x)
      {
        //user is trying to cancel the booking
        booking_status_l = "\"Invalid action\"";
      }

      if (x)
      {
        //user is trying to book the slot
        if (PS1u2 || PS2u2 || PS3u2)
        {
          //check if he hasn’t booked any other slot
          booking_status_l = "\"cannot book more than one slot\""; ////////booking multiple slots
        }
        if (!(PS1u2 || PS2u2 || PS3u2))
        {
          //user has not booked any other slot
          if (flag_user2 == 0)
          {
            booking_status_l = "\"You have booked B1 slot\"";
            PS4u2 = 1; //indicating user 2 has booked slot ps4
            parking_status_l = "\"You haven't entered the parking space\"";
            //note the start time for booking
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            st2 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// converting into seconds
            amount_l = 0;
            time_in_min_l = 0;
            set_gpio_statusPS4(x);
            Message_on_booking = "\"B1 slot is booked\"";
          }
          else
          {
            booking_status_l = "\"Invalid booking\"";
            parking_status_l = "\"You are in the parking space\"";
          }
        }
      }
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }

  if (methodName.equals("setValuePS1u3"))
  {
    int x = data["params"];
    if (sl1 == 1)
    {
      //slot is not free to occupy
      if (x)
      {
        //user is trying to book
        Message_on_booking = "\"A1 cannot be booked\"";
        booking_status_t = "\"A1 cannot be booked\""; ////////booking a slot not available
      }
      if (!x)
      {
        //user is trying to cancel the booking
        if (PS1u3 == 1)
        {
          //Check if he's the user who booked, then check if he has not entered the parking lot
          if (flag_user3 == 0)
          {
            booking_status_t = "\"A1 booking is cancelled\"";
            parking_status_t = "\"You haven't entered the parking space\"";
            //note the cancellation time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp3 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_t = (sp3 - st3) / 60 * paking_fee_per_minute;
            time_in_min_t = (sp3 - st3) / 60;
            set_gpio_statusPS1(x);
            Message_on_booking = "\"A1 slot booking is cancelled\"";
            PS1u3 = 0;
          }
          else
          {
            //if the user is inside the parking space then he cannot cancel it
            booking_status_t = "\"Error!!!canceling not allowed\"";
          }
        }
        else if (PS1u1 || PS1u2)
        {
          booking_status_t = "\"Invalid action\"";
        }
      }
    }
    else
    {
      //slot is available for booking
      if (!x)
      {
        //user is trying to cancel the booking
        booking_status_t = "\"Invalid action\"";
      }

      if (x)
      {
        //user is trying to book the slot
        if (PS2u3 || PS3u3 || PS4u3)
        {
          //check if he hasn’t booked any other slot
          booking_status_t = "\"cannot book more than one slot\""; ////////booking multiple slots
        }
        if (!(PS2u3 || PS3u3 || PS4u3))
        {
          //user has not booked any other slot
          if (flag_user3 == 0)
          {
            booking_status_t = "\"You have booked A1 slot\"";
            PS1u3 = 1; //indicating user 3 has booked slot ps1
            parking_status_t = "\"You haven't entered the parking space\"";
            //note the start time for booking
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            st3 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// converting into seconds
            amount_t = 0;
            time_in_min_t = 0;
            set_gpio_statusPS1(x);
            Message_on_booking = "\"A1 slot is booked\"";
          }
          else
          {
            booking_status_t = "\"Invalid booking\"";
            parking_status_t = "\"You are in the parking space\"";
          }
        }
      }
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
  if (methodName.equals("setValuePS2u3"))
  {
    int x = data["params"];
    if (sl2 == 1)
    {
      //slot is not free to occupy
      if (x)
      {
        //user is trying to book
        Message_on_booking = "\"A2 cannot be booked\"";
        booking_status_t = "\"A2 cannot be booked\""; ////////booking a slot not available
      }
      if (!x)
      {
        //user is trying to cancel the booking
        if (PS2u3 == 1)
        {
          //Check if he's the user who booked, then check if he has not entered the parking lot
          if (flag_user3 == 0)
          {
            booking_status_t = "\"A2 booking is cancelled\"";
            parking_status_t = "\"You haven't entered the parking space\"";
            //note the cancellation time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp3 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_t = (sp3 - st3) / 60 * paking_fee_per_minute;
            time_in_min_t = (sp3 - st3) / 60;
            set_gpio_statusPS2(x);
            Message_on_booking = "\"A2 slot booking is cancelled\"";
            PS2u3 = 0;
          }
          else
          {
            //if the user is inside the parking space then he cannot cancel it
            booking_status_t = "\"Error!!!canceling not allowed\"";
          }
        }
        else if (PS2u1 || PS2u2)
        {
          booking_status_t = "\"Invalid action\"";
        }
      }
    }
    else
    {
      //slot is available for booking
      if (!x)
      {
        //user is trying to cancel the booking
        booking_status_t = "\"Invalid action\"";
      }

      if (x)
      {
        //user is trying to book the slot
        if (PS1u3 || PS3u3 || PS4u3)
        {
          //check if he hasn’t booked any other slot
          booking_status_t = "\"cannot book more than one slot\""; ////////booking multiple slots
        }
        if (!(PS1u3 || PS3u3 || PS4u3))
        {
          //user has not booked any other slot
          if (flag_user3 == 0)
          {
            booking_status_t = "\"You have booked A2 slot\"";
            PS2u3 = 1; //indicating user 3 has booked slot ps2
            parking_status_t = "\"You haven't entered the parking space\"";
            //note the start time for booking
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            st3 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// converting into seconds
            amount_t = 0;
            time_in_min_t = 0;
            set_gpio_statusPS2(x);
            Message_on_booking = "\"A2 slot is booked\"";
          }
          else
          {
            booking_status_t = "\"Invalid booking\"";
            parking_status_t = "\"You are in the parking space\"";
          }
        }
      }
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
  if (methodName.equals("setValuePS3u3"))
  {
    int x = data["params"];
    if (sl3 == 1)
    {
      //slot is not free to occupy
      if (x)
      {
        //user is trying to book
        Message_on_booking = "\"B2 cannot be booked\"";
        booking_status_t = "\"B2 cannot be booked\""; ////////booking a slot not available
      }
      if (!x)
      {
        //user is trying to cancel the booking
        if (PS3u3 == 1)
        {
          //Check if he's the user who booked, then check if he has not entered the parking lot
          if (flag_user3 == 0)
          {
            booking_status_t = "\"B2 booking is cancelled\"";
            parking_status_t = "\"You haven't entered the parking space\"";
            //note the cancellation time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp3 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_t = (sp3 - st3) / 60 * paking_fee_per_minute;
            time_in_min_t = (sp3 - st3) / 60;
            set_gpio_statusPS3(x);
            Message_on_booking = "\"B2 slot booking is cancelled\"";
            PS3u3 = 0;
          }
          else
          {
            //if the user is inside the parking space then he cannot cancel it
            booking_status_t = "\"Error!!!canceling not allowed\"";
          }
        }
        else if (PS3u1 || PS3u2)
        {
          booking_status_t = "\"Invalid action\"";
        }
      }
    }
    else
    {
      //slot is available for booking
      if (!x)
      {
        //user is trying to cancel the booking
        booking_status_t = "\"Invalid action\"";
      }

      if (x)
      {
        //user is trying to book the slot
        if (PS1u3 || PS2u3 || PS4u3)
        {
          //check if he hasn’t booked any other slot
          booking_status_t = "\"cannot book more than one slot\""; ////////booking multiple slots
        }
        if (!(PS1u3 || PS2u3 || PS4u3))
        {
          //user has not booked any other slot
          if (flag_user3 == 0)
          {
            booking_status_t = "\"You have booked B2 slot\"";
            PS3u3 = 1; //indicating user 3 has booked slot ps3
            parking_status_t = "\"You haven't entered the parking space\"";
            //note the start time for booking
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            st3 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// converting into seconds
            amount_t = 0;
            time_in_min_t = 0;
            set_gpio_statusPS3(x);
            Message_on_booking = "\"B2 slot is booked\"";
          }
          else
          {
            booking_status_t = "\"Invalid booking\"";
            parking_status_t = "\"You are in the parking space\"";
          }
        }
      }
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
  if (methodName.equals("setValuePS4u3"))
  {
    int x = data["params"];
    if (sl4 == 1)
    {
      //slot is not free to occupy
      if (x)
      {
        //user is trying to book
        Message_on_booking = "\"B1 cannot be booked\"";
        booking_status_t = "\"B1 cannot be booked\""; ////////booking a slot not available
      }
      if (!x)
      {
        //user is trying to cancel the booking
        if (PS4u3 == 1)
        {
          //Check if he's the user who booked, then check if he has not entered the parking lot
          if (flag_user3 == 0)
          {
            booking_status_t = "\"B1 booking is cancelled\"";
            parking_status_t = "\"You haven't entered the parking space\"";
            //note the cancellation time
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            sp3 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);
            amount_t = (sp3 - st3) / 60 * paking_fee_per_minute;
            time_in_min_t = (sp3 - st3) / 60;
            set_gpio_statusPS4(x);
            Message_on_booking = "\"B1 slot booking is cancelled\"";
            PS4u3 = 0;
          }
          else
          {
            //if the user is inside the parking space then he cannot cancel it
            booking_status_t = "\"Error!!!canceling not allowed\"";
          }
        }
        else if (PS4u3 == 0)
        {
          booking_status_t = "\"Invalid action\"";
        }
      }
    }
    else
    {
      //slot is available for booking
      if (!x)
      {
        //user is trying to cancel the booking
        booking_status_t = "\"Invalid action\"";
      }

      if (x)
      {
        //user is trying to book the slot
        if (PS1u3 || PS2u3 || PS3u3)
        {
          //check if he hasn’t booked any other slot
          booking_status_t = "\"cannot book more than one slot\""; ////////booking multiple slots
        }
        if (!(PS1u3 || PS2u3 || PS3u3))
        {
          //user has not booked any other slot
          if (flag_user3 == 0)
          {
            booking_status_t = "\"You have booked B1 slot\"";
            PS4u3 = 1; //indicating user 3 has booked slot ps4
            parking_status_t = "\"You haven't entered the parking space\"";
            //note the start time for booking
            time_t now = time(nullptr);
            struct tm* p_tm = localtime(&now);
            st3 = (p_tm->tm_hour) * (3600) + (p_tm->tm_min) * (60) + (p_tm->tm_sec);// converting into seconds
            amount_t = 0;
            time_in_min_t = 0;
            set_gpio_statusPS4(x);
            Message_on_booking = "\"B1 slot is booked\"";
          }
          else
          {
            booking_status_t = "\"Invalid booking\"";
            parking_status_t = "\"You are in the parking space\"";
          }
        }
      }
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
  if (methodName.equals("setValueub1"))
  {
    ub1 = data["params"];
    if (ub1)
    {
      user_selection = "\"User Sahana Selected\"";
    }
    else
    {
      user_selection = "\"User Sahana Unselected\"";
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
  if (methodName.equals("setValueub2"))
  {
    ub2 = data["params"];
    if (ub2)
    {
      user_selection = "\"User Lokesh Selected\"";
    }
    else
    {
      user_selection = "\"User Lokesh Unselected\"";
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
  if (methodName.equals("setValueub3"))
  {
    ub3 = data["params"];
    if (ub3)
    {
      user_selection = "\"User Trishank Selected\"";
    }
    else
    {
      user_selection = "\"User Trishank Unselected\"";
    }
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
  }
}

String get_gpio_status() {
  // Prepare gpios JSON payload string
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.createObject();
  data[String("params")] = gpioState[5];
  char payload[256];
  data.printTo(payload, sizeof(payload));
  String strPayload = String(payload);
  Serial.print("Get gpio status: ");
  Serial.println(strPayload);
  return strPayload;
}

void  set_gpio_statusPS1(int statu) {
  // Output GPIOs state
  Serial.println(statu);
  digitalWrite(Slot_LED1, statu);
  bkirps1 = statu;
  sl1 = statu;
  // Update GPIOs state
  gpioState[5] = statu;

}
void  set_gpio_statusPS2(int statu) {
  // Output GPIOs state
  Serial.println(statu);
  digitalWrite(Slot_LED2, statu);
  sl2 = statu;
  bkirps2 = statu;
  // Update GPIOs state
  gpioState[5] = statu;

}
void  set_gpio_statusPS3(int statu) {
  // Output GPIOs state
  Serial.println(statu);
  digitalWrite(Slot_LED3, statu);
  sl3 = statu;
  bkirps3 = statu;
  // Update GPIOs state
  gpioState[5] = statu;

}
void  set_gpio_statusPS4(int statu) {
  // Output GPIOs state
  Serial.println(statu);
  digitalWrite(Slot_LED4, statu);
  sl4 = statu;
  bkirps4 = statu;
  // Update GPIOs state
  gpioState[5] = statu;

}

void payload1()
{
  /////*      Prepare a JSON payload string    *////////

  String payload1 = "{";
  payload1 += "\"User\":";
  payload1 += user;
  payload1 += ",";
  payload1 += "\"Status\":";
  payload1 += user_status;
  payload1 += ",";
  payload1 += "\"Amount_to_be_paid_in_Rs\":";
  payload1 += amount_to_be_paid;
  payload1 += ",";
  payload1 += "\"Time_spent_in_minutes\":";
  payload1 += time_spent;
  payload1 += ",";
  payload1 += "\"Message_on_booking\":";
  payload1 += Message_on_booking;
  payload1 += ",";
  payload1 += "\"Message_on_Selecting_users\":";
  payload1 += user_selection;
  payload1 += "}";

  /////*      Send payload1 to the WISE3 telemetry *///////

  char att[300];
  payload1.toCharArray( att, 300 );
  client.publish( "v1/devices/me/telemetry", att );
  Serial.println( att );

}

void payload2()
{
  /////*      Prepare a JSON payload string    *////////

  String payload2 = "{";
  payload2 += "\"A1\":";
  payload2 += slot_status_a1;
  payload2 += ",";
  payload2 += "\"A2\":";
  payload2 += slot_status_a2;
  payload2 += ",";
  payload2 += "\"B1\":";
  payload2 += slot_status_b1;
  payload2 += ",";
  payload2 += "\"B2\":";
  payload2 += slot_status_b2;
  payload2 += ",";
  payload2 += "\"Slot_availability\":";
  payload2 += slot_availability;
  payload2 += ",";
  payload2 += "\"No_of_available_slots\":";
  payload2 += no_of_available_slots;
  payload2 += "}";

  /////*      Send payload2 to the WISE3 telemetry *///////

  char att[300];
  payload2.toCharArray( att, 300 );
  client.publish( "v1/devices/me/telemetry", att );
  Serial.println( att );

}
void payload3()
{
  /////*      Prepare a JSON payload string    *////////

  String payload3 = "{";
  payload3 += "\"Improper_parking\":";
  payload3 += Improper_parking;
  payload3 += "}";

  /////*      Send payload3 to the WISE3 telemetry *///////

  char att[300];
  payload3.toCharArray( att, 300 );
  client.publish( "v1/devices/me/telemetry", att );
  Serial.println( att );

}
void payloads()
{
  /////*      Prepare a JSON payload string    *////////

  String payloads = "{";
  payloads += "\"Booking_status_for_sahana\":";
  payloads += booking_status_s;
  payloads += ",";
  payloads += "\"Parking_status_for_sahana\":";
  payloads += parking_status_s;
  payloads += ",";
  payloads += "\"Amount_to_be_paid_in_rupees_for_sahana\":";
  payloads += amount_s;
  payloads += ",";
  payloads += "\"time_spent_in_min_for_sahana\":";
  payloads += time_in_min_s;
  payloads += "}";

  /////*      Send payloads to the WISE3 telemetry *///////

  char att[300];
  payloads.toCharArray( att, 300 );
  client.publish( "v1/devices/me/telemetry", att );
  Serial.println( att );

}

void payloadl()
{
  /////*      Prepare a JSON payload string    *////////

  String payloadl = "{";
  payloadl += "\"Booking_status_for_lokesh\":";
  payloadl += booking_status_l;
  payloadl += ",";
  payloadl += "\"Parking_status_for_lokesh\":";
  payloadl += parking_status_l;
  payloadl += ",";
  payloadl += "\"Amount_to_be_paid_in_rupees_for_lokesh\":";
  payloadl += amount_l;
  payloadl += ",";
  payloadl += "\"time_spent_in_min_for_lokesh\":";
  payloadl += time_in_min_l;
  payloadl += "}";

  /////*      Send payloadl to the WISE3 telemetry *///////

  char att[300];
  payloadl.toCharArray( att, 300 );
  client.publish( "v1/devices/me/telemetry", att );
  Serial.println( att );

}
void payloadt()
{
  /////*      Prepare a JSON payload string    *////////

  String payloadt = "{";
  payloadt += "\"Booking_status_for_trishank\":";
  payloadt += booking_status_t;
  payloadt += ",";
  payloadt += "\"Parking_status_for_trishank\":";
  payloadt += parking_status_t;
  payloadt += ",";
  payloadt += "\"Amount_to_be_paid_in_rupees_for_trishank\":";
  payloadt += amount_t;
  payloadt += ",";
  payloadt += "\"time_spent_in_min_for_trishank\":";
  payloadt += time_in_min_t;
  payloadt += "}";

  /////*      Send payloadt to the WISE3 telemetry *///////

  char att[300];
  payloadt.toCharArray( att, 300 );
  client.publish( "v1/devices/me/telemetry", att );
  Serial.println( att );

}
void InitWiFi() {
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to demoserver node ...");
    // Attempt to connect (clientId, username, password)
    if ( client.connect("ESP8266 Device", TOKEN, NULL) ) {
      Serial.println( "[DONE]" );
      // Subscribing to receive RPC requests

      client.subscribe("v1/devices/me/rpc/request/+");
      // Sending current GPIO status
      Serial.println("Sending current GPIO status ...");
      client.publish("v1/devices/me/attributes", get_gpio_status().c_str());
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}



