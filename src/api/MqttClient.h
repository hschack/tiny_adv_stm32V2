#ifndef GsmTcpClient_h
#define GsmTcpClient_h

#include <Arduino.h>
#include <Client.h>

enum MqttVersion { MQTT_3_1 = 3, MQTT_3_1_1 = 4 };

class MqttClient {
 public:
  static const int MqttPort = 1883;
  static const int MqttsPort = 8883;

  // TODO: Need beginPublish(), write(), endPublish()
  virtual int16_t connect(const char client_id[],
                          const char user_name[] = NULL,
                          const char password[] = NULL,
                          bool clean_session = true) = 0;
  // TODO retain, will, etc
  virtual bool connected() = 0;
  virtual bool disconnect() = 0;
  virtual bool disconnectAll() = 0;
  virtual uint16_t keepAliveSeconds() = 0;
  virtual void loop() = 0;
  virtual MqttVersion mqttVersion() = 0;
  virtual bool publish(const char topic[], const char payload[]) = 0;
  // bool publish(const char topic[], const char payload[], bool retained =
  // false, int qos = 0);
  //  Need callback or have polling methods similar to HTTP client
  //  responseCode(), responseBody()
  virtual String receiveBody() = 0;
  virtual String receiveTopic() = 0;
  virtual void setKeepAliveSeconds(uint16_t seconds) = 0;
  virtual void setMqttVersion(MqttVersion version) = 0;
  virtual bool subscribe(const char topic[], int qos = 0) = 0;
  virtual bool unsubscribe(const char topic[]) = 0;
};

#endif