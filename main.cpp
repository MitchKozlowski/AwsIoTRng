#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

void syncTime();

const char* WIFI_SSID = "Your SSID";
const char* TOPIC = "esp32/test/data";
const char* WIFI_PASSWORD = "Your Wi-fi Password";
const char* AWS_IOT_ENDPOINT = "a22owu9uxp37rn-ats.iot.us-east-1.amazonaws.com";

static const char AWS_CERT_CA[] = R"EOF(
-----BEGIN CERTIFICATE-----
*Your Cert Here*
-----END CERTIFICATE-----
)EOF";


static const char AWS_CERT_CRT[] = R"EOF(
-----BEGIN CERTIFICATE-----
"Your Cert Here"
-----END CERTIFICATE-----
)EOF";


static const char AWS_CERT_PRIVATE[] = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
*Your Cert Here*
-----END RSA PRIVATE KEY-----
)EOF";


WiFiClientSecure net;
PubSubClient client(net);

void connectAWS() {
  IPAddress dns1(8, 8, 8, 8);
  IPAddress dns2(8, 8, 4, 4);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, dns1, dns2);
 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  syncTime();

  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
  net.setHandshakeTimeout(30);

  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setSocketTimeout(30);
  
Serial.print("Connecting to AWS IoT");
while (!client.connect("esp32-test-device")) {
  Serial.print(".");
  Serial.print(" [MQTT state: ");
  Serial.print(client.state());
  Serial.print("] ");

  char lastError[100];
  net.lastError(lastError, 100);
  Serial.print("[TLS error: ");
  Serial.print(lastError);
  Serial.println("]");

  delay(1000);
}
  Serial.println("\nConnected to AWS IoT Core!");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  connectAWS();
}

void loop() {
  if (!client.connected()) {
    connectAWS();
  }
  client.loop();

  static unsigned long lastPublish = 0;
  if (millis() - lastPublish > 5000) {
    lastPublish = millis();
    int placeholderReading = random(0, 100);
    String payload = "{\"reading\": " + String(placeholderReading) + "}";
    Serial.print("Publishing: ");
    Serial.println(payload);
    client.publish(TOPIC, payload.c_str());
  }
}

void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Syncing time");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();
  Serial.println("Time synced");
}
