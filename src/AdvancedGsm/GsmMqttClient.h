#ifndef Advanced_GsmMqttClient_h
#define Advanced_GsmMqttClient_h

#include "../api/MqttClient.h"
#include "GsmTcpClient.h"

class GsmMqttClient : public MqttClient {
 protected:
  static const int16_t BodyBufferSize = 2000;
  static const int16_t TopicBufferSize = 100;

 public:
  GsmMqttClient(GsmTcpClient& client,
                const char server_name[],
                uint16_t server_port = MqttPort,
                bool use_tls = false);

  uint16_t keepAliveSeconds() override;
  MqttVersion mqttVersion() override;
  String receiveBody() override;
  String receiveTopic() override;
  void setKeepAliveSeconds(uint16_t seconds) override;
  void setMqttVersion(MqttVersion version) override;

 protected:
  GsmTcpClient* client;
  uint16_t keep_alive_s = 60;
  MqttVersion mqtt_version = MqttVersion::MQTT_3_1_1;
  char received_body[BodyBufferSize] = {0};
  char received_topic[TopicBufferSize] = {0};
  const char* server_name;
  uint16_t server_port;
  bool use_tls;
};

#endif