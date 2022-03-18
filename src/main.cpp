#include <M5Atom.h>
#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>
#include <TinyGPS++.h>
#include <BluetoothSerial.h>

#define TINY_GSM_MODEM_SIM7080

#define APN "soracom.io"
#define USER "sora"
#define PASS "sora"

const char server[] = "ambidata.io";
const char channelId[] = "<<Your channel ID>>";
const char writeKey[] = "<<Your writeKey>>";
char resource[1024];
const int port = 80; // if HTTPS, set port as 443

#include <TinyGsmClient.h>
TinyGsm modem(Serial1);
TinyGsmClient client(modem);
// TinyGsmClientSecure client(modem); // if HTTPS, set class as TinyGsmClientSecure
TinyGPSPlus gps;
BluetoothSerial SerialBT;

HttpClient http(client, server, port);
int statusCode = 0;

const int capacity = JSON_OBJECT_SIZE(5);
StaticJsonDocument<capacity> doc;
char payload[2048];

#define INTERVAL_MIN 3

void setup()
{
  M5.begin();

  // Connect GPS Module
  Serial.begin(9600, SERIAL_8N1, 22, -1);
  SerialBT.begin("ATOM GPS Logger");
  // Connect CAT-M Module
  Serial1.begin(115200, SERIAL_8N1, 32, 26);

  modem.restart();
  sprintf(resource, "/api/v2/channels/%s/data", channelId);

  while (!modem.waitForNetwork() && !modem.isNetworkConnected())
  {
    delay(1);
  }
  SerialBT.printf("Connected to %s\n", APN);

  modem.gprsConnect(APN, USER, PASS);
  while (!modem.isGprsConnected())
  {
    delay(1);
  }
  SerialBT.printf("Operator: %s\n", modem.getOperator().c_str());

  http.connectionKeepAlive();
  while (Serial.available() > 0)
  {
    char c = Serial.read();
    gps.encode(c);

    if (gps.location.isUpdated() && gps.speed.isUpdated() && gps.altitude.isUpdated())
    {
      SerialBT.println("Located!");
      doc["writeKey"] = writeKey;
      doc["d1"] = gps.speed.kmph();
      doc["d2"] = gps.altitude.meters();
      doc["lat"] = gps.location.lat();
      doc["lng"] = gps.location.lng();
      serializeJson(doc, payload);

      SerialBT.println(payload);
      SerialBT.println("Send to Ambient");
      int err = http.post(resource, "application/json", payload);
      if (err != 0)
      {
        SerialBT.println("Failed to Connect");
      }
      statusCode = http.responseStatusCode();
      if (statusCode == 200)
      {
        SerialBT.println("Success!");
      }
      else
      {
        SerialBT.printf("Failed, StatusCode: %d\n", statusCode);
      }
      http.stop();
      client.stop();
      modem.gprsDisconnect();
      modem.poweroff();
    }
  }
  esp_deep_sleep(1E6 * 60 * INTERVAL_MIN);
}
void loop()
{
}
