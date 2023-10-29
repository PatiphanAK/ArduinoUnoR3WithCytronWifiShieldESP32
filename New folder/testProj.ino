#include <WiFiEspAT.h>
#include <PubSubClient.h>

// Emulate Serial1 on pins 6/7 if not present
#if defined(ARDUINO_ARCH_AVR) && !defined(HAVE_HWSERIAL1)
#include <SoftwareSerial.h>
SoftwareSerial Serial1(2, 3); // RX, TX
#define AT_BAUD_RATE 9600
#else
#define AT_BAUD_RATE 115200
#endif

const char ssid[] = "NT wifi  1505-2.4G";
const char pass[] = "0882857409";
const char* mqtt_server = "broker.netpie.io";
const char* mqtt_client = "0b594322-9c72-42af-90dc-ff2bd57c2bca";
const char* token = "w7o8TVeB7UwK9fPnZereqyPK7TLKKb2w";
const char* secret = "heC2SmEuuWapgd6tgkVijABY396DEReF";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


int trigPin = 6; // ขาที่เชื่อมกับ TRIG
int echoPin = 7; // ขาที่เชื่อมกับ ECHO

long returnDistance() {
  // ส่งสัญญาณ TRIG
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // รับสัญญาณ ECHO
  int pulseWidth = pulseIn(echoPin, HIGH);
  // คำนวณระยะทาง (เป็นเซนติเมตร)
  long distance = pulseWidth / 29 / 2;

  // คืนค่าระยะทาง
  return distance;
}

float returnTemp() {
  int sensorPin = A0; // ขา Analog ที่เชื่อม MCP9700
  float referenceVoltage = 5.0; // ค่าแรงดันของขา Vref
  // อ่านค่าแรงดันจาก MCP9700
  int sensorValue = analogRead(sensorPin);

  // คำนวณอุณหภูมิ (อาศัยสูตรในคู่มือ MCP9700)
  float temperature = (sensorValue / 1024.0) * referenceVoltage - 0.5;
  temperature = temperature / 0.01;

  // คืนค่าอุณหภูมิ
  return temperature;
}


void setup_wifi() {
  while (!Serial);
  Serial1.begin(9600);
  WiFi.init(Serial1);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println();
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  WiFi.setPersistent(); // set the following settings as persistent
  WiFi.endAP(); // to disable default automatic start of persistent AP at startup

//  use this lines for custom SoftAP IP. it determines the IP of stations too
//  IPAddress ip(192, 168, 2, 1);
//  WiFi.configureAP(ip);

  Serial.println();
  Serial.print("Start AP with SSID: ");
  Serial.println(ssid);

  //  int status = WiFi.beginAP(ssid); // for AP open without passphrase/encryption
  int status = WiFi.beginAP(ssid, pass);

  // waiting for connection to Wifi network set with the SetupWiFiConnection sketch
  Serial.println("Waiting for connection to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print('.');
  }
  Serial.println();
  Serial.println("Connected to WiFi network.");
  
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(13, HIGH);   
  } else {
    digitalWrite(13, LOW);  
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ClientID-RandomPanwit";
    // Attempt to connect
    if (client.connect(mqtt_client,token, secret)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("@msg/tatar", "hello world from my Arduino");
      // ... and resubscribe
      //client.subscribe("@msg/tatar");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
if (!client.connected()) {
    reconnect();
  }
  client.loop();
  int temp = returnTemp();
  long distance = returnDistance();
  String place = "Tatar's Room";
  // สร้างข้อความ JSON
  String data = "{\"data\": {\"temperature\":" + String(temp) + ", \"distance\":" + String(distance) + ",\"place\": \"" + place + "\"}}";
  Serial.println(data);
  // แปลงข้อความ JSON เป็น char array
  char msg[200];
  data.toCharArray(msg, (data.length() + 1));

  // ส่งข้อมูลไปยัง Netpie Shadows
  client.publish("@shadow/data/update", msg);

  // รอสักครู่
  delay(5000);
}


