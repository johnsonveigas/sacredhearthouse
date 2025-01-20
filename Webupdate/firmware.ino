//V2.5
//*Added And Logic for lastbuzzertime variable of less than 30 per logic
//Added overflow acknowledge logic to turn off the buzzer untill level2 reach <95
//V2.6 Added error logging to server which indicates which function triggered the buzzer
//V2.7 Added MQTT code 
//2.8 Updated MQTT code
//2.9 Added Topic Recieve code to recieve the ackno value
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>

//#include <ESPAsyncTCP.h>
//#include <ESPAsyncWebServer.h>
//#include <AsyncElegantOTA.h>
#include<ESP8266mDNS.h>
#include <WiFiClientSecureBearSSL.h>
#include<PubSubClient.h>




const char *ssid = "veigas 2.4g";
const char *password = "kelwin123";
//const char *host = "http://orange-rosita-95.tiiny.io";
 

const char *fingerprint="";
const float current_version = 2.9;


// 20% Full
byte tank20[8] = {
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111,
  B11111
};

// 40% Full
byte tank40[8] = {
  B10001,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111,
  B11111,
  B11111
};

// 60% Full
byte tank60[8] = {
  B10001,
  B10001,
  B10001,
  B10001,
  B11111,
  B11111,
  B11111,
  B11111
};

// 80% Full
byte tank80[8] = {
  B10001,
  B10001,
  B10001,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

// 100% Full
byte tankFull[8] = {
  B10001,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte dash[8] = {
  B00000,
  B00000,
  B00000,
  B01110,
  B01110,
  B00000,
  B00000,
  B00000
};

//AsyncWebServer server(8080); 


//Variable Declaration
int level1,level2,sen_status,distance;
byte command,comd_resp,buzzer;
long duration;
int previous_read1,previous_read2,previous_read_history,previous_read_history2;
String error_code,error_description;


unsigned long starttime=0;
unsigned long elapsedtime=0;
unsigned long settime=60000;
int tolerance = 5;
int ledON=0;
int cyclecheck=0;
byte ackno=1;
byte top_HH_ackno=1;
unsigned long startingmilli=0;
unsigned long presentmilli=0;
const unsigned long buzzerInterval=120000;

unsigned long lastBuzzerTime1 = 0;  // Timestamp for the last time buzzer was triggered
const unsigned long buzzerInterval1 = 600000;
byte top_HH_set_bit=0;
byte top_HH_condition_met=0;
unsigned long top_tank_HH_start=0;
byte buzzer_state=0;
unsigned long buzzer_on_time=0;
unsigned long buzzer_current_millis=0;

byte top_tank_buzzer_state=1;
byte overflow_ackno_bit=1;
String buzzer_reason="";




const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_clientId = "Deivce_sacredheartesp";
const char* topic_publish = "sacredheart_waterlevel";
const char* topic_subscribe = "sacredheart_fetchdata";



int tank1Max= 23;
int tank1Min= 95;
int tank2Max= 23;
int tank2Min=95;
int tempvar=0;

//Pinouts
const int trigPin = D3; 
const int trigPin2 = D8;
const int echoPin = D0; // down tank
const int echoPin2 = D6;
const int overflow = D5;
const int buzzer_out1=D4;
const int push_Button = D7;


volatile bool buttonPressed=false;



const int api_key=1234;
const int id=1;

HTTPClient httpCli;
WiFiClientSecure client;
WiFiClient mqtt_cli;


void callback(char* topic, byte* payload, unsigned int length);
PubSubClient mqtt_client(mqtt_server, mqtt_port, callback, mqtt_cli);




//interrupt isr
void IRAM_ATTR handleButtonPress() {
  if(buzzer==1){
 // digitalWrite(buzzer_out1, HIGH); // Turn off the buzzer immediately
        buzzer_on("OFF");
  buttonPressed = true;
    ackno  = 1;       // Set flag to acknowledge button press
    buzzer=0;
        overflow_ackno_bit=1;

  }
    if(top_tank_buzzer_state==1){
    buzzer_on("OFF");
        
}
  
  lcd.backlight();
  ledON=1;
}
/*
void IRAM_ATTR limitS(){
  
    delay(500);
    if(buzzer!=1&&digitalRead(overflow)==0&&ackno==0){
   //digitalWrite(buzzer_out1,LOW);
   Serial.println("o1");
        buzzer_on("ON");
        buzzer=1;
        ackno=0;
        
    }
}
*/
void setup() {
    previous_read1=0;

    digitalWrite(buzzer_out1,HIGH);
    digitalWrite(trigPin,HIGH);
 pinMode(trigPin,OUTPUT);
 pinMode(trigPin2,OUTPUT);
 pinMode(echoPin,INPUT);
 pinMode(echoPin2,INPUT);
    //pinMode(buzzer_out,OUTPUT);
 pinMode(buzzer_out1,OUTPUT);
 /*   
 digitalWrite(buzzer_out1,LOW);
 delay(250);
    digitalWrite(buzzer_out1,HIGH);
    delay(50);
    digitalWrite(buzzer_out1,LOW);
    delay(250);
    
    digitalWrite(buzzer_out1,HIGH);*/
   digitalWrite(buzzer_out1,HIGH);
 pinMode(push_Button,INPUT_PULLUP);
 pinMode(overflow,INPUT);
    
    
 
    //attachInterrupt(digitalPinToInterrupt(overrflow), limitS, HIGH);

    attachInterrupt(digitalPinToInterrupt(push_Button), handleButtonPress, FALLING);


  lcd.init();      
  lcd.backlight();
   ledON=1;
   
  
  lcd.clear();     
    lcd.createChar(0,tank20);
    lcd.createChar(1,tank40);
    lcd.createChar(2,tank60);
    lcd.createChar(3,tank80);
    lcd.createChar(4,tankFull);
    lcd.createChar(5,dash);
  Serial.begin(115200);
   String Displaystring="By Johnson V";
    Displaystring.concat(current_version);
 Display(0,0,"TankTrack System");
 Display(1,1,Displaystring);
 buzzer_sound(1);  
 digitalWrite(buzzer_out1,HIGH);
 delay(3000);
 
 lcd.clear();
 Display(2,0,"Connecting to");
 Display(4,1,"WiFi....");
 WifiConnect();
 lcd.clear();
 if(WiFi.status()==WL_CONNECTED){
 Display(1,0,"WiFi Connected");
 IPAddress localip=WiFi.localIP();
 
 Display(1,1,localip.toString());
 

 }
    else{
    
       
}
 
 
 //lcd.write(byte(1));
 
    
   if (MDNS.begin("myesp")) {  // "myesp" is the desired hostname
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error starting mDNS");
  }
    Serial.println(current_version);
    
  /*  
server.on("/",HTTP_GET,[](AsyncWebServerRequest *request){
    request->send(200,"text/plain","Hi Your are connected and onlinehhgg");
});
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Restarting ESP...");
    delay(3000);  // Allow time for the response to be sent before restarting
    ESP.restart();  // Restart the ESP
  });
    
    AsyncElegantOTA.begin(&server); // Start ElegantOTA 
    server.begin();
    */
    checkForupdates();
    reconnect();
    
    
}
void loop() {
   // AsyncElegantOTA.loop();
  HTTPClient httpCli;
    WiFiClientSecure client;
    
    


 if(WiFi.status()!=WL_CONNECTED){
  WifiConnect();
 }
   
if (!mqtt_client.loop())
reconnect();

  
    buzzer_current_millis=millis();
    if(buzzer_state==1&&buzzer_current_millis-buzzer_on_time>=300000){
        digitalWrite(buzzer_out1,HIGH);
        buzzer_state=0;
        buzzer_on_time=0;
    
}
  
//sen1();
//sen2();
int level1_distance=sensor1();

int level2_distance=sensor2();


//LCD Backlight
if(ledON==1){
starttime =millis();

if(starttime-
elapsedtime>=1800000){
lcd.noBacklight();
ledON=0;
elapsedtime=starttime;
}
}

//Top Tank  Water HH Delay
if(level1_distance>98&&top_HH_condition_met!=1){
top_tank_HH_start= millis();
top_HH_condition_met=1;
}

if(top_HH_condition_met && millis() - top_tank_HH_start >= 30000){
  top_HH_set_bit=1;
  
}

if(level1_distance<95){
  top_HH_set_bit=0;
  top_HH_condition_met=0;
}
 //Top Tank Water Level Full 
   
    if(level1_distance>98&&top_HH_set_bit==1&&level2_distance<98&&top_tank_buzzer_state==0){
  /*digitalWrite(buzzer_out1, LOW);  
  delay(800);
  digitalWrite(buzzer_out1, HIGH); 
  delay(300);
  digitalWrite(buzzer_out1, LOW); 
  delay(800);
  digitalWrite(buzzer_out1, HIGH);*/
        buzzer_on("ON");
        buzzer_reason="Up Tank Full";
        error_code="0xA01";
        Serial.println("o2");
        top_tank_buzzer_state=1;
    }   
    
    if(level1_distance<95){
    top_tank_buzzer_state=0;
}
// Water Full Condition
//Serial.println(ackno);
if(level1_distance>98&&level2_distance>98&&top_HH_set_bit==1){
        if(ackno!=1){
            
   // digitalWrite(buzzer_out1,LOW);
    buzzer_on("ON");
    error_code="0xA01";
    buzzer_reason="Both Tank Full";
    buzzer=1;
    ackno=0;
    top_HH_condition_met = 0; 
    top_HH_set_bit = 0; 
    
}}
    //Display(10,1,String(fetchAcknoStatus()));
    if(buzzer==1){
       if(fetchAcknoStatus()==1){
        //digitalWrite(buzzer_out1,HIGH);
            buzzer_on("OFF");
            buzzer=0;
            ackno=1;
            overflow_ackno_bit=1;
    }
        
    
}
unsigned long  currentMillis1 = millis();

// Check if either level1 or level2 is below 30% and 15 minutes have passed since last buzzer
if ((level1_distance < 30 || level2_distance < 30) && (currentMillis1 - lastBuzzerTime1 >= buzzerInterval1)) {
  
  // Trigger buzzer
  digitalWrite(buzzer_out1, LOW);  // Turn buzzer ON
  delay(250);
  digitalWrite(buzzer_out1, HIGH); // Turn buzzer OFF
  delay(250);
  digitalWrite(buzzer_out1, LOW);  // Turn buzzer ON
  delay(250);
  digitalWrite(buzzer_out1, HIGH); // Turn buzzer OFF
  
  // Update the last buzzer time
  lastBuzzerTime1 = currentMillis1;
}
else{
  
}


if(level1_distance>35&&level2_distance>35){
  lastBuzzerTime1=0;
}
 /*   
if(digitalRead(D6)==1){
    if(starttime==0){
        starttime= millis();
    }
        elapsedtime=millis()-starttime;
        lcd.clear();
    Display(9,0,String(elapsedtime));
    delay(1000);
        
       
}
    else{
    starttime=0;
        elapsedtime=0;
}
    if(elapsedtime>=settime){
    ackno=0;
        elapsedtime=0;
        starttime=0;
}
   */ 
if(digitalRead(overflow)==1&&ackno==0&&buzzer==0&&overflow_ackno_bit==0){
        error_code="0xA01";
    buzzer_reason="Tank Overflow";
    buzzer_on("ON");
    buzzer=1;
    
     
}

//Overflow acknobit reset
if(level2_distance<95&&overflow_ackno_bit==1){
        overflow_ackno_bit=0;
    }
    
    
if(ackno==1&&level1_distance<95||ackno==1&&level2_distance<95){
    ackno=0;
}

    
    
lcd.clear();
if(cyclecheck==0){
lcd.setCursor(11,0);
lcd.write(byte(5));
lcd.setCursor(11,1);
lcd.write(byte(5));
}
cyclecheck=!cyclecheck;
Display(0,0,"Tank1");
Display(0,1,"Tank2");
if(level1_distance>100){
        Display(12,0,"100%");
    }else if(level1_distance<0){
        Display(12,0,"0%");
    } else {
Display(12,0,String(level1_distance)+"%");
        }
     WriteByte(level1_distance,0);

//Sensor 2
    
  if(level2_distance>100){
        Display(12,1,"100%");
    }else if(level2_distance<0){
        Display(12,1,"0%");
    } else {
Display(12,1,String(level2_distance)+"%");
        }
    
    WriteByte(level2_distance,1);
    
  
   
    
    
    sen_status=0;
    if(buzzer==0){
    
    buzzer_reason="";  
}

  
   
  Error();
   //SendData(buzzer,ackno,level1_distance,level2_distance,sen_status,error_code,error_description);
    SendDataToMQTT(buzzer,ackno,level1_distance,level2_distance,sen_status,error_code,error_description);


  
 //Water Level Buzzer 
  
  delay(500);
    error_code="";
    previous_read_history=0;
    previous_read_history2=0;
   // lcd.clear();
}


//Buzzer Output Function
void buzzer_on(String buzz_state)
{
  if(buzz_state=="ON"){
    digitalWrite(buzzer_out1,LOW);
        buzzer_state=1;
        buzzer_on_time=millis();
}
    if(buzz_state=="OFF"){
    digitalWrite(buzzer_out1,HIGH);
        buzzer_state=0;
        buzzer_on_time=0;
}

}

void checkForupdates(){
  
  client.setInsecure();
  Serial.println("ok");
float web_version_int=0.0;
String update_url_version="https://raw.githubusercontent.com/johnsonveigas/sacredhearthouse/refs/heads/webupdate/Webupdate/update.version?raw=true";
    const char* update_url_bin="https://raw.githubusercontent.com/johnsonveigas/sacredhearthouse/refs/heads/webupdate/Webupdate/firmware.cpp.bin?raw=true";
    //Check Version
     httpCli.begin(client, update_url_version);
    int httpCode=httpCli.GET();
   Serial.println(httpCode);
   Serial.println("ok1");
   Serial.print(httpCli.getString());
    if(httpCode==200){
    String web_version=httpCli.getString();
web_version_int=web_version.toFloat();
//Serial.println(web_version_int);
//Version Check If statemtn
if(web_version_int>current_version){
//Serial.println("Update Available");
lcd.clear();
Display(5,0,"Updating");
Serial.println("44");
t_httpUpdate_return ret = ESPhttpUpdate.update(client, update_url_bin);
switch(ret)
{ 
case HTTP_UPDATE_FAILED: Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
break;
case HTTP_UPDATE_NO_UPDATES: Serial.println("HTTP_UPDATE_NO_UPDATES");
break; 
            }
         }
        }
    else{
    //Serial.println("Http error"+httpCode);
}
    httpCli.end();
}
    
//MQTT Reconnect

void reconnect() {
    int count_retry=0;
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt_client.connect(mqtt_clientId)) {  // Use a unique client ID
      Serial.println("connected");
      mqtt_subscribe(topic_subscribe);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      delay(200);
      count_retry=count_retry+1;
      if(count_retry==5){
            break;
        }
    }
  }
}

//Send Data to Mqtt Server
void SendDataToMQTT(byte buzzer1, byte ackno, int level1_1, int level2_1, int sen_status1, String err_cd, String err_des) {
  
    reconnect();
  
  String payload = String("ackno=") + String(ackno) + "&" +
                   "buzzer=" + String(buzzer1) + "&" +
                   "level1=" + String(level1_1) + "&" + 
                   "level2=" + String(level2_1) + "&" +
                   "sen_status=" + String(sen_status1) + "&" +
                   "error_code=" + String(err_cd) + "&" +
                   "error_description=" + String(err_des);

  mqtt_client.publish(topic_publish, payload.c_str());  // Publish the data to the encoded topic

  Serial.println("Published data: " + payload);
}

//mqtt message Recieve

void callback(char* topic, byte* payload, unsigned int length) {
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  

  
  if ((char)payload[0] == '1') {
     lcd.backlight();
  } else {
     lcd.noBacklight();
  }


}

//Mqtt Topic Subscribe

void mqtt_subscribe(const char * topic) {
  if (mqtt_client.subscribe(topic))
    Serial.println("Subscribe \"" + String(topic) + "\" ok");
  else
    Serial.println("Subscribe \"" + String(topic) + "\" failed");
}


void Display(int x,int y, String text){
    
    lcd.setCursor(x, y);
    lcd.print(text);
}

/*
void checkLowWaterLevel(int lvl1, int lvl2) {
  presentmilli = millis();

  if ((lvl1 < 30) || (lvl2 < 30)) { 
    if (presentmilli - startingmilli >= buzzerInterval) {
      startingmilli = presentmilli; 
      buzzer_sound(2);
    } 
  } else {
    noTone(buzzer_out1); 
  }
}
*/


void SendData(byte buzzer1,byte ackno,int level1_1,int level2_1,int sen_status1,String err_cd,String err_des){
    
    client.setInsecure();
    //client.setFingerprint(fingerprint);
    String postData =
  "api_key=" + String(api_key) + "&" +
  "id=" + String(id)+ "&" +"ackno="+String(ackno)  +"&"+
  "buzzer=" + String(buzzer1) + "&" +
  "level1=" + String(level1_1) + "&" +
  "level2=" + String(level2_1) + "&" +
  "sen_status=" + String(sen_status1)+"&"+"error_code="+String(err_cd)+"&"+"error_description="+String(err_des);
  
  httpCli.begin(client, "https://bronze-alyse-61.tiiny.io/post-data.php");
  httpCli.addHeader("Content-Type", "application/x-www-form-urlencoded");
    httpCli.setTimeout(5000);
    

int httpCode = httpCli.POST(postData);  
  // Serial.println(httpCode);  
 String payload = httpCli.getString();    
 // Serial.println("Error Reason: " + httpCli.errorToString(httpCode));
   /* lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(payload);
    delay(3000);*/
httpCli.end();  
}

void WifiConnect()
{
    WiFi.mode(WIFI_OFF); 
    delay(1000);
    WiFi.mode(WIFI_STA); // WiFi Station Mode

    // Try connecting to 'dev' Wi-Fi first
    WiFi.begin("JAISON 5G", "");  // Attempt to connect to 'dev' Wi-Fi
    Serial.println("");
    Serial.print("Connecting to dev Wi-Fi");

    int attempt = 10;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        attempt = attempt - 1;
        if (attempt == 0) {
            break;
        }
    }
delay(2000);
    // If connection to 'dev' Wi-Fi fails, connect to 'veigas 2.4'
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("");
        Serial.println("Failed to connect to dev Wi-Fi. Trying 'veigas 2.4' Wi-Fi...");

        WiFi.begin("veigas 2.4g", "kelwin123");  // Attempt to connect to 'veigas 2.4'
        attempt = 10;
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
            attempt = attempt - 1;
            if (attempt == 0) {
                break;
            }
        }
    }

    // Check if Wi-Fi connection was successful
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(WiFi.SSID());
        Serial.println("IP Address: " + WiFi.localIP().toString());

        // Show IP address on the LCD (optional)
        lcd.clear();
        Display(1, 0, "WiFi Connected");
        Display(1, 1, WiFi.localIP().toString());
    } else {
        Serial.println("");
        Serial.println("Failed to connect to both Wi-Fi networks.");
    }
}


void sen1(){
digitalWrite(trigPin,LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin,HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin,LOW);
    
    duration = pulseIn(echoPin,HIGH);
    Serial.println("Value1:-");
    Serial.print(duration*0.034/2);
    
}
void sen2(){
digitalWrite(trigPin2,LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin2,HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin2,LOW);
    
    duration = pulseIn(echoPin2,HIGH);
    Serial.println("Value2:-");
    Serial.print(duration*0.034/2);
    
}


int sensor1() {
  float percentage;
  int readings[5];  // Array to store 5 readings

  // Take 5 readings
  for (int i = 0; i < 5; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
        
    readings[i] = duration * 0.034 / 2; // Calculate distance in cm
        
    delay(100);  // Small delay between readings
  }

  // Sort readings to remove highest and lowest
  for (int i = 0; i < 5; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (readings[i] > readings[j]) {
        int temp = readings[i];
        readings[i] = readings[j];
        readings[j] = temp;
      }
    }
  }

  // Average the middle 3 readings (discard highest and lowest)
  int total = readings[1] + readings[2] + readings[3];
  distance = total / 3;

  // Calculate percentage based on the average distance
  if (distance >= 23 && distance < 400) {
    percentage = ((float)(distance - tank1Min) / (float)(tank1Max - tank1Min)) * 100;
  } else {
    percentage = 100;
  }

  // Water Sensor Check
  if (previous_read1 == 0) {
    previous_read1 = distance;
  } else if (distance - previous_read1 != 0) {
    if ((distance - previous_read1) > 5) {
      error_code = "0xS02";
    } else if ((distance - previous_read1) < -5) {
      error_code = "0xS01";
    }
  }

  if (distance > 400 || distance < 0) {
    error_code = "0xS03";
  }

  previous_read_history = previous_read1;
  previous_read1 = distance;
    previous_read1 = duration * 0.034 / 2;
  int roundedPercentage = round(percentage);

  delay(500);
  return roundedPercentage;
}

int sensor2() {
  float percentage=0;
  int readings[5];  // Array to store 5 readings

  // Take 5 readings
  for (int i = 0; i < 5; i++) {
    digitalWrite(trigPin2, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin2, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin2, LOW);

    duration = pulseIn(echoPin2, HIGH);
    readings[i] = duration * 0.034 / 2; // Calculate distance in cm
    delay(100);  // Small delay between readings
  }

  // Sort readings to remove highest and lowest
  for (int i = 0; i < 5; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (readings[i] > readings[j]) {
        int temp = readings[i];
        readings[i] = readings[j];
        readings[j] = temp;
      }
    }
  }

  // Average the middle 3 readings (discard highest and lowest)
  int total = readings[1] + readings[2] + readings[3];
  distance = total / 3;

  // Calculate percentage based on the average distance
  if (distance >=20 && distance < 400) {
    percentage = ((float)(distance - tank2Min) / (float)(tank2Max - tank2Min)) * 100;
  }

  // Water Sensor Check
  if (previous_read2 == 0) {
    previous_read2 = distance;
  } else if (distance - previous_read2 != 0) {
    if ((distance - previous_read2) > 5) {
      error_code = "0xS04";
    } else if ((distance - previous_read2) < -5) {
      error_code = "0xS05";
    }
  }

  if (distance > 400 || distance < 0) {
    error_code = "0xS06";
  }

  previous_read_history2 = previous_read2;
  previous_read2 = distance;
    previous_read2 = duration * 0.034 / 2;
  int roundedPercentage = round(percentage);

  delay(500);
  return roundedPercentage;
}

int fetchAcknoStatus() {
  if (httpCli.begin(client, "https://bronze-alyse-61.tiiny.io/fetch_ackno.php")) {
    int httpCode = httpCli.GET();
    if (httpCode > 0) {
      String payload = httpCli.getString();
      Serial.println("Fetch Ackno Response: " + payload);
      
      // Parse JSON response
      if (payload.indexOf("\"Ackno\":\"1\"") > 0) {
        ackno = 1;
        
      } else {
        ackno = 0;
      }
    }
    httpCli.end();
  } else {
    
  }
    return ackno;
}


void WriteByte(int percent,int row){
   
int levelo = 0;       

switch (percent / 20) {
    case 0:
        levelo = 1;  // For 0-20%
        break;
    case 1:
        levelo = 2;  // For 21-40%
        break;
    case 2:
        levelo = 3;  // For 41-60%
        break;
    case 3:
        levelo = 4;  // For 61-80%
        break;
    case 4:
        levelo = 5;  // For 81-100%
        break;
    
}
    if(percent>95)
    levelo=5;
    
    if(levelo>=1){
    lcd.setCursor(6,row);
    lcd.write(byte(0));
}
    if(levelo>=2){
    lcd.setCursor(7,row);
    lcd.write(byte(1));
}
    if(levelo>=3){
    lcd.setCursor(8,row);
    lcd.write(byte(2));
}
    if(levelo>=4){
    lcd.setCursor(9,row);
    lcd.write(byte(3));
}
    if(levelo>=5){
    lcd.setCursor(10,row);
    lcd.write(byte(4));
}


}


    



void buzzer_sound(int type){
    //Error Sound
    if(type==1){
       unsigned long startTime = millis();
  while (millis() - startTime < 1000) {  // Play sound for 2 seconds
    tone(buzzer_out1, 200, 100);          // 200 Hz tone for 100 ms
    delay(125);                           // 25 ms pause for effect 
  }
  
  
    
}
    if(type==2){
       unsigned long startTime = millis();
  while (millis() - startTime < 3000) {  // Play sound for 2 seconds
    tone(buzzer_out1, 300, 250);          // 200 Hz tone for 100 ms
    delay(125);                           // 25 ms pause for effect 
  }
  
    
}
    

    }

void Error() {
    // Sensor 1 error conditions
    if (error_code == "0xS01") {
        error_description = "Sensor1 Level Decremented Rapidly With Previous Value as " + String(previous_read_history) + " and Current Value: " + String(distance);
    } else if (error_code == "0xS02") {
        error_description = "Sensor1 Level Increased Rapidly With Previous Value as " + String(previous_read_history) + " and Current Value: " + String(distance);
    } else if (error_code == "0xS03") {
        error_description = "Inappropriate Values Returned By Sensor 1: " + String(distance);
    }
    
    // Sensor 2 error conditions
    else if (error_code == "0xS04") {
        error_description = "Sensor2 Level Decremented Rapidly With Previous Value as " + String(previous_read_history) + " and Current Value: " + String(distance);
    } else if (error_code == "0xS05") {
        error_description = "Sensor2 Level Increased Rapidly With Previous Value as " + String(previous_read_history) + " and Current Value: " + String(distance);
    } else if (error_code == "0xS06") {
        error_description = "Inappropriate Values Returned By Sensor 2: " + String(distance);
    }
    else if (error_code == "0xA01"){
        error_description = "Buzzer is turned on by: " +buzzer_reason;
    }

    // Reset error if no matching code
    else {
        error_code = "";
        error_description = "";
    }
}