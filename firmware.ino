//V2.5
//*Added And Logic for lastbuzzertime variable of less than 30 per logic
//Added overflow acknowledge logic to turn off the buzzer untill level2 reach <95
//V2.6 Added error logging to server which indicates which function triggered the buzzer
//V2.7 Added MQTT code 
//2.8 Updated MQTT code
//2.9 Added Topic Recieve code to recieve the ackno value
//3.1 Added Ackno fetching from Mqtt and Added Sending Serial data to Mqtt
//3.2 Added top_tank buzzer full ackno option from online also. Added sending both buzzer and top tank buzzer state to online 
//3.8 Added buzzer_state varible in send data to mqtt 
//3.9 Bottom Tank Dead Zone issue fix
//4.0 New Mqtt Broker Test
//4.8 Both Tank level confirmation for 10sec added for true buzzer triggering. Tank ackno condition is sent to server for further debugging
//4.9 Fixed tank ackno string variable
//5.8 Added Timeout feature for checkupdate function. Removed the top tank level check condition from Both tank full if condition.  Added Top tank delay set bit triggring level 
//7.3 MQTT Timeout feature for block proof loop running
//7.4 Implemented boot date time publish to mqtt
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
#define MQTT_SOCKET_TIMEOUT 15  
#include<PubSubClient.h>
#include <ArduinoJson.h>
#include<time.h>




const char *ssid = "veigas 2.4g";
const char *password = "kelwin123";
//const char *host = "http://orange-rosita-95.tiiny.io";
 

const char *fingerprint="";
const float current_version = 7.6;


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
bool previousLed=0;
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
unsigned long top_tank_standby_millis=0;
byte top_tank_disable_bit=0;

byte top_tank_buzzer_state=1;
byte overflow_ackno_bit=1;
String buzzer_reason="";
byte mqtt_ackno=0;
int Bottom_tank_previous_val;
String restartReason="";
byte rstReceive=0;
byte buzzer_on_to_mqtt=0;
byte buzzer_mqtt_receive=0;
unsigned long both_HH_current_milli=0;
unsigned long both_HH_delay_milli=0;
byte both_HH_set_bit=0;
String top_tank_ackno_temp="";
String both_tank_ackno_temp="";

String boot_datetime="";




const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* mqtt_clientId = "Deivce_sacredheartesp";
const char* topic_publish = "sacredheart_waterlevel";
const char* topic_subscribe = "sacredheart_fetchdata";
const char* topic_publish_serial="sacredheart_serialmon";

const char* ntpServer="time.nist.gov";
const long gmtOffset_sec=19800;
const int daylightOffset_sec=0;



int tank1Max= 20;
int tank1Min= 95;
int tank2Max= 20;
int tank2Min=95;
int tempvar=0;

int displayCount;
//Pinouts
const int trigPin = D8; //g
const int trigPin2 = D3;//b
const int echoPin = D6; // down tank
const int echoPin2 = D0;
const int overflow = D5; //WB Actual D5
const int buzzer_out1=D4; //Actual D4
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
    serialPrint("Presses");
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
  ledON=1;
  previousLed=0;
  lcd.backlight();
  
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
 pinMode(overflow,INPUT_PULLUP);
    
    
 
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
    restartReason=ESP.getResetInfo();
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
    
    
  delay(2000); // Give NTP time to sync
    boot_datetime=getTimeFromAPI();
    
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


if(level2_distance==0&&Bottom_tank_previous_val>90)
 level2_distance=Bottom_tank_previous_val;
Bottom_tank_previous_val=level2_distance;
//LCD Backlight
elapsedtime=millis();
if(ledON==1){
        
if(previousLed!=1){
starttime =millis();
previousLed=1;
}
if(elapsedtime-
starttime>=1800000){
lcd.noBacklight();
ledON=0;
previousLed=0;

}
}

//Top Tank  Water HH Delay
if(level1_distance>98&&top_HH_condition_met!=1){
top_tank_HH_start= millis();
top_HH_condition_met=1;
buzzer_reason="HH Set Bit"+String(level1_distance);
}

if(top_HH_condition_met && millis() - top_tank_HH_start >= 80000){
  top_HH_set_bit=1;
  
}

if(level1_distance<80){
  top_HH_set_bit=0;
  top_HH_condition_met=0;
  
}
    
if(top_tank_disable_bit==1&&millis()-top_tank_standby_millis>=900000){
    top_tank_disable_bit=0;
    top_tank_standby_millis=0;
}
    
      
 //Top Tank Water Level Full 
   
    if(level1_distance>90&&top_HH_set_bit==1&&top_tank_buzzer_state==0&&top_tank_disable_bit==0){
  /*digitalWrite(buzzer_out1, LOW);  
  delay(800);
  digitalWrite(buzzer_out1, HIGH); 
  delay(300);
  digitalWrite(buzzer_out1, LOW); 
  delay(800);
  digitalWrite(buzzer_out1, HIGH);*/
        buzzer_on("ON");
        top_tank_standby_millis=millis();
        top_tank_disable_bit=1;
        buzzer_reason="Up Tank Full";
        error_code="0xA01";
        serialPrint("O2");
        top_tank_buzzer_state=1;
    }   
    
    if(level1_distance<80){
    top_tank_buzzer_state=0;
  //  top_tank_ackno_temp += " Top Buzzer state"+String(level1_distance);
}
//Water Full Delay 
if(level2_distance>98&&both_HH_set_bit!=1)  
    {
    both_HH_delay_milli=millis();
    both_HH_set_bit=1;
} else{
    both_HH_set_bit=0;
    both_HH_delay_milli=0;
    both_HH_current_milli=0;
}
if(both_HH_set_bit==1){
    both_HH_current_milli=millis();
}
    
// Water Full Condition
//Serial.println(ackno);
if(level2_distance>98&&both_HH_set_bit==1&&both_HH_current_milli-both_HH_delay_milli>=10000){
        
        if(ackno!=1){
            
   // digitalWrite(buzzer_out1,LOW);
     serialPrint("O1");
    buzzer_on("ON");
    error_code="0xA01";
    buzzer_reason=" Both Tank Full"+String(level1_distance)+String(level2_distance);
    buzzer=1;
    ackno=0;
    top_HH_condition_met = 0; 
    top_HH_set_bit = 0; 
    both_HH_set_bit=0;
    both_HH_delay_milli=0;
    both_HH_current_milli=0;        
    
}}
    
    if(ackno==1)
    mqtt_ackno=0;
    
    //Display(10,1,String(fetchAcknoStatus()));
    if(buzzer==1||top_tank_buzzer_state==1){
       if(mqtt_ackno==1){
        //digitalWrite(buzzer_out1,HIGH);
            buzzer_on("OFF");
            serialPrint("Buzzer Off Command From Online");
            buzzer=0;
            ackno=1;
            overflow_ackno_bit=1;
            mqtt_ackno=0;
    }
        
    
}
unsigned long  currentMillis1 = millis();

// Check if either level1 or level2 is below 30% and 15 minutes have passed since last buzzer
if ((level1_distance < 30 || level2_distance < 10) && (currentMillis1 - lastBuzzerTime1 >= buzzerInterval1)) {
  
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
    
if(digitalRead(overflow)==0&&ackno==0&&buzzer==0&&overflow_ackno_bit==0){
        delay(1000);
        if(digitalRead(overflow)==0){
            serialPrint("O3");
        error_code="0xA01";
    buzzer_reason="Tank Overflow";
    buzzer_on("ON");
    buzzer=1;
    
     }
}

//Overflow acknobit reset
if(level2_distance<95&&overflow_ackno_bit==1){
        overflow_ackno_bit=0;
    }
    
    
if(ackno==1&&level1_distance<95||ackno==1&&level2_distance<95){
    ackno=0;
   // both_tank_ackno_temp+="Both Tank Ackno"+String(level1_distance)+String(level2_distance);
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
    
  if(displayCount==6){
        serialPrint("yes");
        
        displayCount=0;
        delay(1500);
        lcd.clear();
    Display(0,0,"Top:");
    Display(0,1,"Bot:");
        
   
    if(level1_distance<=100){
           int TankLvl1=(1000*level1_distance)/100;
            String TankLvl1s=String(TankLvl1)+"L/"+"1000L";
            int TankLvl2=(1000*level2_distance)/100;
         String TankLvl2s=String(TankLvl2)+"L/"+"1000L";   Display(4,0,TankLvl1s);
            Display(4,1,TankLvl2s);
            delay(2000);
        }
}
   
 displayCount=displayCount+1;   
    
    sen_status=0;
//Reset buzzerMqtt 
if(buzzer_mqtt_receive==1){
    buzzer_on_to_mqtt=0;
    buzzer_reason="";
    top_tank_ackno_temp="";
    both_tank_ackno_temp="";
    buzzer_mqtt_receive=0;
}
    

//Reset Reason Sending To MQTT
if(restartReason!=""&&rstReceive==1)
    {
        serialPrint("Reset");
    restartReason="";
        rstReceive=0;
}
  
byte temp_buzzer;   
  Error();
   //SendData(buzzer,ackno,level1_distance,level2_distance,sen_status,error_code,error_description);
if(buzzer==1||top_tank_buzzer_state==1){
temp_buzzer=1;  
        }
    else{
    temp_buzzer=0;
}
    
     SendDataToMQTT(buzzer_state,ackno,level1_distance,level2_distance,sen_status,error_code,error_description,restartReason,buzzer_on_to_mqtt,buzzer_reason,boot_datetime);


  /*  
if(buzzer==0){
    
    buzzer_reason="";  
} */
  
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
        buzzer_on_to_mqtt=1;
        buzzer_on_time=millis();
}
    if(buzz_state=="OFF"){
    digitalWrite(buzzer_out1,HIGH);
        buzzer_state=0;
        buzzer_on_time=0;
}

}

void serialPrint(String serialdata){
    Serial.println(serialdata);
    if(mqtt_client.connected()){
    mqtt_client.publish(topic_publish_serial,serialdata.c_str());  
}
    
}

void checkForupdates(){
  
  client.setInsecure();
  
float web_version_int=0.0;
String update_url_version="https://raw.githubusercontent.com/johnsonveigas/sacredhearthouse/refs/heads/webupdate/Webupdate/update.version?raw=true";
    const char* update_url_bin="https://raw.githubusercontent.com/johnsonveigas/sacredhearthouse/refs/heads/webupdate/Webupdate/firmware.cpp.bin?raw=true";
    //Check Version
    httpCli.setTimeout(6000);  // Timeout in milliseconds (e.g., 5 seconds)
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
Display(4,0,"Updating");
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
void SendDataToMQTT(byte buzzer1, byte ackno, int level1_1, int level2_1, int sen_status1, String err_cd, String err_des,String rstreason,byte buzzeron,String buzzres,String bootdatetime) {
  
    reconnect();
  
  String payload = String("ackno=") + String(ackno) + "&" +
                   "buzzer=" + String(buzzer1) + "&" +
                   "level1=" + String(level1_1) + "&" + 
                   "level2=" + String(level2_1) + "&" +
                   "sen_status=" + String(sen_status1) + "&" +
                   "error_code=" + String(err_cd) + "&" +
                   "error_description=" + String(err_des) + "&" + "rstreason=" + String(rstreason) + "&" + "buzzerOn=" + String(buzzeron) + "&" + "buzzerReason=" + String(buzzres)+"&"+"Booted At="+String(bootdatetime);

 // mqtt_client.publish(topic_publish, payload.c_str()); 
     // Publish the data to the encoded topic
    
   // Publish only if MQTT client is connected
  if (mqtt_client.connected()) {
    mqtt_client.publish(topic_publish, payload.c_str());  // Non-blocking publish
    //Serial.println("Published data: " + payload);
  } else {
    Serial.println("MQTT still not connected. Skipping publish.");
  }

 // Serial.println("Published data: " + payload);
}



String getTimeFromAPI() {
  WiFiClientSecure client;
  client.setInsecure();  // Bypass SSL check (safe for public APIs)

  
  String url = "https://timeapi.io/api/Time/current/zone?timeZone=Asia/Kolkata";

  String finalStr = "########";

  if (httpCli.begin(client, url)) {
    int httpCode = httpCli.GET();

    if (httpCode == 200) {
      String payload = httpCli.getString();

      // Extract fields using simple string parsing (no ArduinoJson needed)
      int y = payload.indexOf("\"year\":");
      int m = payload.indexOf("\"month\":");
      int d = payload.indexOf("\"day\":");
      int h = payload.indexOf("\"hour\":");
      int min = payload.indexOf("\"minute\":");
      int s = payload.indexOf("\"seconds\":");

      if (y >= 0 && m >= 0 && d >= 0 && h >= 0 && min >= 0 && s >= 0) {
        int year = payload.substring(y + 7, payload.indexOf(",", y)).toInt();
        int month = payload.substring(m + 8, payload.indexOf(",", m)).toInt();
        int day = payload.substring(d + 6, payload.indexOf(",", d)).toInt();
        int hour = payload.substring(h + 7, payload.indexOf(",", h)).toInt();
        int minute = payload.substring(min + 9, payload.indexOf(",", min)).toInt();
        int sec = payload.substring(s + 10, payload.indexOf(",", s)).toInt();

        // Format: DDMMYYYY HHMMSS (with leading zeros)
        char buffer[20];
        sprintf(buffer, "%02d%02d%04d %02d%02d%02d", day, month, year, hour, minute, sec);
        finalStr = String(buffer);
      }
    }

    httpCli.end();
  }

  return finalStr;
}



//mqtt message Recieve





void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';  // Null-terminate the string
  Serial.println((char*)payload);  // Print full payload
  
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);
  
  if (!error) {
    if (doc.containsKey("ackno")) {
      mqtt_ackno = doc["ackno"];
    }
    if (doc.containsKey("rstReceive")) {
            
      rstReceive = doc["rstReceive"];
    }
    if(doc.containsKey("buzzerReceive")){
            buzzer_mqtt_receive=doc["buzzerReceive"];
            serialPrint(String(buzzer_mqtt_receive));
        }
        if(doc.containsKey("rstCmd")){
        if(doc["rstCmd"]==1){
            delay(2000);
            ESP.restart();
        }
    }
    else {
    Serial.println("JSON Parsing Failed!");
  }
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


String getDateTime() {
  if (WiFi.status() != WL_CONNECTED) {
    return "####### NoWifi"; // WiFi not connected
  }

  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate verification

  const char* host = "timeapi.io";
  const int httpsPort = 443;
  const char* url = "/api/Time/current/zone?timeZone=Asia/Kolkata";

  if (!client.connect(host, httpsPort)) {
    return "####### Confailed"; // Connection failed
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break; // headers ended
  }

  String payload = client.readString();
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    return "####### JSONERR"; // JSON parse error
  }

  int d = doc["day"].as<int>();
int m = doc["month"].as<int>();
int fullYear = doc["year"].as<int>();
int y = fullYear % 100;
int h = doc["hour"].as<int>();
int min = doc["minute"].as<int>();
int s = doc["seconds"].as<int>();

  char buffer[20];
  sprintf(buffer, "%02d%02d%02d %02d%02d%02d", d, m, y, h, min, s);

  return String(buffer);
}


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

    serialPrint(String(distance));
  // Calculate percentage based on the average distance
  if (distance >= 20 && distance < 400) {
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

    serialPrint(String(distance));
  // Calculate percentage based on the average distance
  if (distance >=20 && distance < 400) {
    percentage = ((float)(distance - tank2Min) / (float)(tank2Max - tank2Min)) * 100;
  }
    else{
    percentage=100;
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
    tone(buzzer_out1, 200, 50);          // 200 Hz tone for 100 ms
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