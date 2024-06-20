#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "esp_sleep.h"

#define DHTTYPE DHT11

#define PH_PIN 35
#define SCOUNT  30            // sum of sample point
#define TDS_PIN 34
#define DHT_PIN 21
#define VREF 3.3

//sensor status
int ph_status = 1;
int dht_status = 1;
int tds_status = 1;

//
float humidity, temp;
//
int analogBuffer[SCOUNT];     // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;
long int last_reading = 0;
float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25;       // current temperature for compensation
float ph = 0;
float voltage = 0;
// WiFi
const char *ssid = "Rico"; // Enter your Wi-Fi name
const char *password = "ricorico";  // Enter Wi-Fi password

// MQTT Broker
const char *mqtt_broker = "lf145feb.ala.dedicated.gcp.emqxcloud.com";
const char *topic = "device/data";
const char *mqtt_username = "esp32";
const char *mqtt_password = "esp";
const int mqtt_port = 1883;


WiFiClient espClient;
PubSubClient client(espClient);


void connectWifi();
void connectMQTT();
void publishSensorValues(float ph,int ph_status,float humidity, float temp, int dht_status, float tds, int tds_status);


DHT dht(DHT_PIN, DHTTYPE);
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  pinMode(PH_PIN, INPUT);
  pinMode(TDS_PIN, INPUT);
  pinMode(DHT_PIN, INPUT);
  // Connecting to a WiFi network
  connectWifi();
  
  // Initialize the MQTT client
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);

  // Connect to the MQTT broker
  connectMQTT();

  // Subscribe to the topic
  client.subscribe(topic);
}

void connectWifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the Wi-Fi network");
  delay(10000);
}

void connectMQTT() {
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("MQTT broker connected");
    } else {
      Serial.print("Failed to connect, state: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}
int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

void publishSensorValues(float ph,int ph_status,float humidity, float temp, int dht_status, float tds, int tds_status) {
  // Create a payload string with sensor values
  char device_id[50] = "HIPONIC_DEVICE1";  // Replace with your string
  char payload[300];
  snprintf(payload, sizeof(payload), 
  "{\"device_id\": \"%s\", \"ph\": %.2f,\"ph_status\": %d, \"humidity\": %.2f, \"temp\": %.2f,\"dht_status\": %d,\"tds\": %.2f,\"tds_status\": %d }",
   device_id, ph ,ph_status, humidity, temp, dht_status, tdsValue, tds_status);
  
  // Publish the payload
  client.publish(topic, payload);
}
void readSensor(){
  int currentMillis = millis();
  if((currentMillis - last_reading) >= 30000U ){
    last_reading = millis();
    analogBuffer[analogBufferIndex] = analogRead(TDS_PIN);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if(analogBufferIndex == SCOUNT){ 
      analogBufferIndex = 0;
    }

     for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      
      // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0;
      
      //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
      float compensationCoefficient = 1.0+0.02*(temperature-25.0);
      //temperature compensation
      float compensationVoltage=averageVoltage/compensationCoefficient;
      
      //convert voltage value to tds value
      tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");
    }
    ph = analogRead(PH_PIN); 
    voltage = ph * (3.3 / 4095.0); 
    Serial.println(voltage); 
    Serial.println(ph);

    humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
    temp = dht.readTemperature();
  Serial.println(humidity); 
  Serial.println(temp);
  // Check if any reads failed and exit early (to try again).
  if(isnan(ph)){
    ph = -1;
    ph_status = 0;
  }else{
    ph_status = 1;
  } 
  if(isnan(humidity) || isnan(temp)){
    humidity = -1;
    temp = -1;
    dht_status = 0;
    }else {
      dht_status = 1;
    }
  if(isnan(tdsValue)){
    tdsValue = -1;
    tds_status = 0;
  }else {
    tds_status = 1;
  }
  publishSensorValues(ph,ph_status, humidity,temp, dht_status, tdsValue, tds_status);

    // Put the ESP32 into deep sleep mode for 30 seconds
    Serial.println("Entering deep sleep for 30 seconds...");
    esp_sleep_enable_timer_wakeup(30 * 1000000);
    esp_deep_sleep_start();
  }
}

void loop() {
  
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();
  readSensor();
}
