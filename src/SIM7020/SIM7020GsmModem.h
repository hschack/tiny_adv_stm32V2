#ifndef Advanced_SIM7020GsmModem_h
#define Advanced_SIM7020GsmModem_h

#include "../AdvancedGsm/GsmModemCommon.h"

#define GSM_MUX_COUNT 5

class SIM7020HttpClient;
class SIM7020MqttClient;
class SIM7020TcpClient;

class SIM7020GsmModem : public GsmModemCommon {
  friend class SIM7020HttpClient;
  friend class SIM7020MqttClient;
  friend class SIM7020TcpClient;

 public:
  explicit SIM7020GsmModem(Stream& stream);
  int8_t getLocalIPs(String addresses[], uint8_t max) override;
  String ICCID() override;
  bool setClientCA(const String& certificate) override;
  bool setClientPrivateKey(const String& certificate) override;
  bool setRootCA(const String& certificate) override;

 protected:
  SIM7020HttpClient* http_clients[GSM_MUX_COUNT];
  SIM7020MqttClient* mqtt_clients[GSM_MUX_COUNT];

  bool checkConnection() override;
  int8_t checkResponse(uint32_t timeout_ms,
                       String& data,
                       GsmConstStr r1 = GFP(GSM_OK),
                       GsmConstStr r2 = GFP(GSM_ERROR),
                       GsmConstStr r3 = NULL,
                       GsmConstStr r4 = NULL,
                       GsmConstStr r5 = NULL) override;

 private:
  bool checkUnsolicitedHttpResponse(String& data);
  bool checkUnsolicitedMqttResponse(String& data);
  bool checkUnsolicitedResponse(String& data);
  bool confirmPacketDataConfiguration();
  bool setCertificate(int8_t type, const char* certificate);
};

#endif