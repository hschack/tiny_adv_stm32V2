/*
GSM MQTT client example

Build:       pio run -e m5stack-atom
Deploy:      pio run -e m5stack-atom -t upload
View result: pio device monitor --baud 115200
*/
#include <Arduino.h>
/*
Log settings (set before including modem)
*/

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS
#define LOG_OUTPUT Serial



/*
Modem device
*/
// Set serial for AT commands (to the module)
//SerialAT.begin(GSM_BAUDRATE, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
#define MODEM_TX PA2
#define MODEM_RX PA3
#define sw PA0
#define wake_up PB9
#define led PC13
int flag = 1;
int flag1 = 0;
int state=0;
//HardwareSerial Serial2(PA3, PA2);
HardwareSerial SerialAT(MODEM_RX, MODEM_TX);

//#define BAUDRATE 115200
//HardwareSerial SerialAT(2);

#include "SIM7020/SIM7020GsmModem.h"
#include "SIM7020/SIM7020MqttClient.h"

#define TestModem SIM7020GsmModem
#define TestTcpClient SIM7020TcpClient
#define TestMqttClient SIM7020MqttClient

//#define TestModem SIM7080GsmModem

#define GSM_BAUDRATE 115200

/*
Board settings (also see the environment settings in platformio.ini)
*/




// Set serial for output console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

/*
Sample code
*/

//#define WAIT_FOR_NON_LOCAL_IPV6
#define SEND_INTERVAL_MS 5000
//#define USE_INSECURE_HTTP

const char apn[] = "nbiot.tdc.dk";
//const PacketDataProtocolType pdp_type = PacketDataProtocolType::IPv6;
//const PacketDataProtocolType pdp_type = PacketDataProtocolType::IPv4v6;
const PacketDataProtocolType pdp_type = PacketDataProtocolType::IP;

// See https://test.mosquitto.org/
const char server[] = "test.mosquitto.org";
const char client_id[] = "testclient";
const char user_name[] = "rw";
const char password[] = "readwrite";
const char publish_topic[] = "dt/advgsm/demo/rw/txt";
const char subscribe_topic[] = "cmd/advgsm/demo/rw/#";

const int16_t port = 1883;  // unencrypted, unauthenticated
//const int16_t port = 1884; // unencrypted, authenticated
//const int16_t port = 8883;  // encrypted, unauthenticated
//const int16_t port = 8884; // encrypted, client certificate
//const int16_t port = 8885; // encrypted, authenticated
//const int16_t port = 8886; // encrypted: Lets Encrypt, unauthenticated
//const int16_t port = 8887; // encrypted: certificate deliberately expired

const String mos_root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQEL\n"
    "BQAwgZAxCzAJBgNVBAYTAkdCMRcwFQYDVQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwG\n"
    "A1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1vc3F1aXR0bzELMAkGA1UECwwCQ0ExFjAU\n"
    "BgNVBAMMDW1vc3F1aXR0by5vcmcxHzAdBgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hv\n"
    "by5vcmcwHhcNMjAwNjA5MTEwNjM5WhcNMzAwNjA3MTEwNjM5WjCBkDELMAkGA1UE\n"
    "BhMCR0IxFzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTES\n"
    "MBAGA1UECgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVp\n"
    "dHRvLm9yZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzCCASIwDQYJ\n"
    "KoZIhvcNAQEBBQADggEPADCCAQoCggEBAME0HKmIzfTOwkKLT3THHe+ObdizamPg\n"
    "UZmD64Tf3zJdNeYGYn4CEXbyP6fy3tWc8S2boW6dzrH8SdFf9uo320GJA9B7U1FW\n"
    "Te3xda/Lm3JFfaHjkWw7jBwcauQZjpGINHapHRlpiCZsquAthOgxW9SgDgYlGzEA\n"
    "s06pkEFiMw+qDfLo/sxFKB6vQlFekMeCymjLCbNwPJyqyhFmPWwio/PDMruBTzPH\n"
    "3cioBnrJWKXc3OjXdLGFJOfj7pP0j/dr2LH72eSvv3PQQFl90CZPFhrCUcRHSSxo\n"
    "E6yjGOdnz7f6PveLIB574kQORwt8ePn0yidrTC1ictikED3nHYhMUOUCAwEAAaNT\n"
    "MFEwHQYDVR0OBBYEFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMB8GA1UdIwQYMBaAFPVV\n"
    "6xBUFPiGKDyo5V3+Hbh4N9YSMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL\n"
    "BQADggEBAGa9kS21N70ThM6/Hj9D7mbVxKLBjVWe2TPsGfbl3rEDfZ+OKRZ2j6AC\n"
    "6r7jb4TZO3dzF2p6dgbrlU71Y/4K0TdzIjRj3cQ3KSm41JvUQ0hZ/c04iGDg/xWf\n"
    "+pp58nfPAYwuerruPNWmlStWAXf0UTqRtg4hQDWBuUFDJTuWuuBvEXudz74eh/wK\n"
    "sMwfu1HFvjy5Z0iMDU8PUDepjVolOCue9ashlS4EB5IECdSR2TItnAIiIwimx839\n"
    "LdUdRudafMu5T5Xma182OC0/u/xRlEm+tvKGGmfFcN0piqVl8OrSPBgIlb+1IKJE\n"
    "m/XriWr/Cq4h/JfB7NTsezVslgkBaoU=\n"
    "-----END CERTIFICATE-----\n";

// Root certificate for Let's Encrypt
const String le_root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIFYDCCBEigAwIBAgIQQAF3ITfU6UK47naqPGQKtzANBgkqhkiG9w0BAQsFADA/\n"
    "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n"
    "DkRTVCBSb290IENBIFgzMB4XDTIxMDEyMDE5MTQwM1oXDTI0MDkzMDE4MTQwM1ow\n"
    "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
    "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwggIiMA0GCSqGSIb3DQEB\n"
    "AQUAA4ICDwAwggIKAoICAQCt6CRz9BQ385ueK1coHIe+3LffOJCMbjzmV6B493XC\n"
    "ov71am72AE8o295ohmxEk7axY/0UEmu/H9LqMZshftEzPLpI9d1537O4/xLxIZpL\n"
    "wYqGcWlKZmZsj348cL+tKSIG8+TA5oCu4kuPt5l+lAOf00eXfJlII1PoOK5PCm+D\n"
    "LtFJV4yAdLbaL9A4jXsDcCEbdfIwPPqPrt3aY6vrFk/CjhFLfs8L6P+1dy70sntK\n"
    "4EwSJQxwjQMpoOFTJOwT2e4ZvxCzSow/iaNhUd6shweU9GNx7C7ib1uYgeGJXDR5\n"
    "bHbvO5BieebbpJovJsXQEOEO3tkQjhb7t/eo98flAgeYjzYIlefiN5YNNnWe+w5y\n"
    "sR2bvAP5SQXYgd0FtCrWQemsAXaVCg/Y39W9Eh81LygXbNKYwagJZHduRze6zqxZ\n"
    "Xmidf3LWicUGQSk+WT7dJvUkyRGnWqNMQB9GoZm1pzpRboY7nn1ypxIFeFntPlF4\n"
    "FQsDj43QLwWyPntKHEtzBRL8xurgUBN8Q5N0s8p0544fAQjQMNRbcTa0B7rBMDBc\n"
    "SLeCO5imfWCKoqMpgsy6vYMEG6KDA0Gh1gXxG8K28Kh8hjtGqEgqiNx2mna/H2ql\n"
    "PRmP6zjzZN7IKw0KKP/32+IVQtQi0Cdd4Xn+GOdwiK1O5tmLOsbdJ1Fu/7xk9TND\n"
    "TwIDAQABo4IBRjCCAUIwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYw\n"
    "SwYIKwYBBQUHAQEEPzA9MDsGCCsGAQUFBzAChi9odHRwOi8vYXBwcy5pZGVudHJ1\n"
    "c3QuY29tL3Jvb3RzL2RzdHJvb3RjYXgzLnA3YzAfBgNVHSMEGDAWgBTEp7Gkeyxx\n"
    "+tvhS5B1/8QVYIWJEDBUBgNVHSAETTBLMAgGBmeBDAECATA/BgsrBgEEAYLfEwEB\n"
    "ATAwMC4GCCsGAQUFBwIBFiJodHRwOi8vY3BzLnJvb3QteDEubGV0c2VuY3J5cHQu\n"
    "b3JnMDwGA1UdHwQ1MDMwMaAvoC2GK2h0dHA6Ly9jcmwuaWRlbnRydXN0LmNvbS9E\n"
    "U1RST09UQ0FYM0NSTC5jcmwwHQYDVR0OBBYEFHm0WeZ7tuXkAXOACIjIGlj26Ztu\n"
    "MA0GCSqGSIb3DQEBCwUAA4IBAQAKcwBslm7/DlLQrt2M51oGrS+o44+/yQoDFVDC\n"
    "5WxCu2+b9LRPwkSICHXM6webFGJueN7sJ7o5XPWioW5WlHAQU7G75K/QosMrAdSW\n"
    "9MUgNTP52GE24HGNtLi1qoJFlcDyqSMo59ahy2cI2qBDLKobkx/J3vWraV0T9VuG\n"
    "WCLKTVXkcGdtwlfFRjlBz4pYg1htmf5X6DYO8A4jqv2Il9DjXA6USbW1FzXSLr9O\n"
    "he8Y4IWS6wY7bCkjCWDcRQJMEhg76fsO3txE+FiYruq9RUWhiF1myv4Q6W+CyBFC\n"
    "Dfvp7OOGAN6dEOM4+qR9sdjoSYKEBpsr6GtPAQw4dy753ec5\n"
    "-----END CERTIFICATE-----\n";

//#include <M5Unified.h>
//#include <FastLED.h>
//#include <Arduino.h>

// Allocate memory for concrete object
#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TestModem testModem(debugger);
#else
TestModem testModem(SerialAT);
#endif

int delay_ms = SEND_INTERVAL_MS;
int next_message_ms = 0;
// Access via the API
GsmModem& modem = testModem;
bool ready = false;

bool publish_done = false;

void setup() {
#if ADVGSM_LOG_SEVERITY > 0
#ifdef LOG_OUTPUT
  AdvancedGsmLog.Log = &LOG_OUTPUT;
#endif
#endif
 // M5.begin();
  Serial.begin(115200);
  SerialAT.begin(115200);
  pinMode(sw, INPUT_PULLUP);
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  pinMode(wake_up, OUTPUT);
  digitalWrite(wake_up, LOW);
  delay(5000);

  SerialMon.printf("### MQTT client example started at %d\n", millis());



  //modem.resetDefaultConfiguration();

  modem.begin(apn, pdp_type);
  delay(100);

  SerialMon.print("Setup complete\n");
}

bool isReady() {
  // Get non-link-local IP address
  String addresses[4];
  int8_t count = modem.getLocalIPs(addresses, 4);
  bool found_global_ipv6 = false;
  for (int8_t index = 0; index < count; index++) {
    SerialMon.printf("IP address[%d] = %s\n", index, addresses[index].c_str());
    if (addresses[index].indexOf(":") > 0 &&
        !addresses[index].startsWith("fe80:")) {
      found_global_ipv6 = true;
    }
  }
#ifdef WAIT_FOR_NON_LOCAL_IPV6
  return found_global_ipv6;
#else
  return modem.modemStatus() >= ModemStatus::PacketDataReady;
#endif
}

void connectedLoop() {
  int now = millis();
  if (now > next_message_ms) {
    if (!publish_done) {
      SerialMon.printf("### Testing MQTT to: %s (%d)\n", server, port);

      bool use_tls = false;
      if (port >= 8883 && port <= 8887) {
        bool ca_success;
        if (port == 8886) {
          ca_success = modem.setRootCA(le_root_ca);
        } else {
          ca_success = modem.setRootCA(mos_root_ca);
        }
        if (!ca_success) {
          SerialMon.printf("### Set Root CA failed, delaying %d ms", delay_ms);
          next_message_ms = millis() + delay_ms;
          delay_ms = delay_ms * 2;
          return;
        }
        use_tls = true;
      }

      TestTcpClient testTcpClient(testModem);
      TestMqttClient testMqttClient(testTcpClient, server, port, use_tls);
      //TestMqttClient testMqttClient(testTcpClient, server, port, use_tls, 30000);
      MqttClient& mqtt = testMqttClient;
      mqtt.disconnectAll();
      int16_t rc;
      if (port == 1884 || port == 8885) {
        rc = mqtt.connect(client_id, user_name, password);
      } else {
        rc = mqtt.connect(client_id);
      }
      if (rc != 0) {
        Serial.printf("### MQTT connect error: %d, delaying %d ms\n", rc,
                      delay_ms);
        next_message_ms = millis() + delay_ms;
        delay_ms = delay_ms * 2;
        return;
      }

      // Subscribe
      Serial.printf("Subscribing\n");
      mqtt.subscribe(subscribe_topic);
      delay(100);

      // Publish
      Serial.printf("Publishing\n");
      mqtt.publish(publish_topic, "Message from device");
      delay(100);

      // Wait for messages
      int finish = millis() + SEND_INTERVAL_MS;
      while (millis() < finish) {
        modem.loop();
        String topic = mqtt.receiveTopic();
        if (topic.length() > 0) {
          String body = mqtt.receiveBody();
          Serial.printf("Received [%s]: %s\n", topic.c_str(), body.c_str());
        }
        delay(100);
      }

      Serial.printf("Disconnecting\n");
      mqtt.disconnect();

      Serial.printf("Done\n");
      publish_done = true;
    }
  }
}

void loop() {
  modem.loop();
  if (modem.modemStatus() >= ModemStatus::PacketDataReady) {
    if (!ready) {
      ready = isReady();
      Serial.printf("Ready %d (%d)\n", ready, modem.modemStatus());
    }
    if (ready) {
      connectedLoop();
    }
  }
}