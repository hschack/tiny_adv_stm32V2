#include "GsmMqttClient.h"

GsmMqttClient::GsmMqttClient(GsmTcpClient& client,
                             const char server_name[],
                             uint16_t server_port,
                             bool use_tls)
    : client(&client),
      server_name(server_name),
      server_port(server_port),
      use_tls(use_tls) {}

uint16_t GsmMqttClient::keepAliveSeconds() {
  return this->keep_alive_s;
}

MqttVersion GsmMqttClient::mqttVersion() {
  return this->mqtt_version;
}

String GsmMqttClient::receiveBody() {
  String s = String(this->received_body);
  this->received_body[0] = '\0';
  return s;
}

String GsmMqttClient::receiveTopic() {
  String s = String(this->received_topic);
  this->received_topic[0] = '\0';
  return s;
}

void GsmMqttClient::setKeepAliveSeconds(uint16_t seconds) {
  this->keep_alive_s = seconds;
}

void GsmMqttClient::setMqttVersion(MqttVersion version) {
  this->mqtt_version = version;
}
