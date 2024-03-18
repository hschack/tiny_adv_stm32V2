#ifndef Advanced_SIM7020MqttClient_h
#define Advanced_SIM7020MqttClient_h

#include "../AdvancedGsm/GsmMqttClient.h"
#include "SIM7020GsmModem.h"
#include "SIM7020TcpClient.h"

class SIM7020MqttClient : public GsmMqttClient {
  friend class SIM7020GsmModem;

 private:
  int16_t buffer_size;
  int32_t timeout_ms;

 public:
  SIM7020MqttClient(SIM7020TcpClient& client,
                    const char* server_name, // max length 50
                    uint16_t server_port = MqttPort,
                    bool use_tls = false,
                    int32_t timeout_ms = 30000, // 0 to 60000
                    int16_t buffer_size = 1024); // 20 to 1132

  int16_t connect(const char client_id[],
                  const char user_name[] = NULL,
                  const char password[] = NULL,
                  bool clean_session = true) override;
  bool connected() override;
  bool disconnect() override;
  bool disconnectAll() override;
  void loop() override;
  bool publish(const char topic[], const char payload[]) override;
  bool subscribe(const char topic[], int qos = 0) override;
  bool unsubscribe(const char topic[]) override;

 protected:
  int8_t mqtt_id;
  SIM7020GsmModem& modem;
  bool is_connected;

 private:
  int16_t createClientInstance();
  int16_t createClientInstanceExtended();

  const char* server_ca;
  const char* client_certificate;
  const char* client_private_key;
};

#endif