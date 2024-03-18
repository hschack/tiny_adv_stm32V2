#include "SIM7020MqttClient.h"

SIM7020MqttClient::SIM7020MqttClient(SIM7020TcpClient& client,
                                     const char* server_name,
                                     uint16_t server_port,
                                     bool use_tls,
                                     int32_t timeout_ms,
                                     int16_t buffer_size)
    : GsmMqttClient(client, server_name, server_port, use_tls),
      modem(client.modem) {
  this->mqtt_id = -1;
  this->is_connected = false;
  this->buffer_size = buffer_size;
  this->timeout_ms = timeout_ms;
  if (timeout_ms < 0 || timeout_ms > 60000) {
    ADVGSM_LOG(
        GsmSeverity::Error, "SIM7020",
        GF("SIM7020 MQTT timeout must be 0 - 60000 ms"));
  }
  if (buffer_size < 20 || buffer_size > 1132) {
    ADVGSM_LOG(
        GsmSeverity::Error, "SIM7020",
        GF("SIM7020 MQTT buffer size must be 20 - 1132"));
  }
}

int16_t SIM7020MqttClient::connect(const char client_id[],
                                   const char user_name[],
                                   const char password[],
                                   bool clean_session) {
  // Create if needed
  int16_t rc;
  if (this->mqtt_id == -1) {
    // if (use_tls && this->server_ca != nullptr) {
    //   rc = createClientInstanceExtended();
    // } else {
    rc = createClientInstance();
    // }
    if (rc < 0) {
      return rc;
    }

    // Store the connection
    this->mqtt_id = rc;
    this->modem.mqtt_clients[this->mqtt_id] = this;
  }

  ADVGSM_LOG(GsmSeverity::Info, "SIM7020",
             GF("MQTT %d connect client '%s' (user '%s')"), mqtt_id, client_id,
             user_name ? user_name : "");

  if (strlen(client_id) > 120) {
    ADVGSM_LOG(
        GsmSeverity::Error, "SIM7020",
        GF("SIM7020 maximum MQTT client ID length is 120; cannot connect"));
    return -611;
  }
  if (user_name != nullptr) {
    if (strlen(user_name) > 100) {
      ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
               GF("SIM7020 maximum user name length is 100; cannot connect"));
      return -612;
    }
    if (strlen(password) > 100) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
               GF("SIM7020 maximum password length is 100; cannot connect"));
      return -613;
    }
  }

  // Set hex format
  this->modem.sendAT(GF("+CREVHEX=1"));
  this->modem.waitResponse();

  // Connect
  // TODO: Should store client_id as field?
  this->modem.stream.printf(GF("AT+CMQCON=%d,%d,\"%s\",%d,%d"), this->mqtt_id,
                            this->mqtt_version, client_id, this->keep_alive_s,
                            clean_session);
  this->modem.stream.print(GF(",0"));  // Will
  if (user_name != nullptr) {
    this->modem.stream.printf(GF(",%s,%s"), user_name, password);
  }
  this->modem.stream.print(GSM_NL);

  rc = this->modem.waitResponse(60000);
  if (rc == 0) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
               GF("MQTT %d connect client '%s' timed out for user '%s' with password length %d"), this->mqtt_id,
               client_id, user_name, strlen(password));
    return -710;
  } else if (rc != 1) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
               GF("MQTT %d connect client '%s' error for user '%s' with password length %d, error %d"), this->mqtt_id,
               client_id, user_name, strlen(password), rc);
    return -610;
  }

  is_connected = true;
  return 0;
}

bool SIM7020MqttClient::connected() {
  return this->is_connected;
}

int16_t SIM7020MqttClient::createClientInstance() {
  ADVGSM_LOG(GsmSeverity::Info, "SIM7020",
             GF("MQTT creating instance %s, %d (TLS %s)"), server_name,
             server_port, use_tls ? "yes" : "no");

  // TODO: Confirm if it already exists? Can match name, but not port, etc
  // So, maybe need to delete anyway.

  if (strlen(server_name) > 50) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
               GF("SIM7020 maximum server name length is 50; cannot connect"));
    return -605;
  }

  int16_t rc;
  this->modem.sendAT(GF("+CMQTSYNC=1"));
  rc = this->modem.waitResponse();
  if (rc == 0) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020", GF("MQTT sync timed out"));
    return -701;
  } else if (rc != 1) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020", GF("MQTT sync error %d"), rc);
    return -601;
  }

  if (this->use_tls) {
    this->modem.sendAT(GF("+CMQTTSNEW=\""), server_name, "\",\"", server_port,
                       "\",", timeout_ms, ',', buffer_size);
    rc = this->modem.waitResponse(timeout_ms, GF(GSM_NL "+CMQTTSNEW:"));
    if (rc == 0) {
      ADVGSM_LOG(GsmSeverity::Error, "SIM7020", GF("MQTTS new timed out"));
      return -703;
    } else if (rc != 1) {
      ADVGSM_LOG(GsmSeverity::Error, "SIM7020", GF("MQTTS new error"));
      this->modem.sendAT(GF("+CMQTTSNEW?"));
      this->modem.waitResponse();
      return -603;
    }
  } else {
    this->modem.sendAT(GF("+CMQNEW=\""), server_name, "\",\"", server_port,
                       "\",", timeout_ms, ',', buffer_size);
    rc = this->modem.waitResponse(timeout_ms, GF(GSM_NL "+CMQNEW:"));
    if (rc == 0) {
      ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
                 GF("MQTT (no TLS) new timed out"));
      return -702;
    } else if (rc != 1) {
      ADVGSM_LOG(GsmSeverity::Error, "SIM7020", GF("MQTT (no TLS) new error"));
      this->modem.sendAT(GF("+CMQNEW?"));
      this->modem.waitResponse();
      return -602;
    }
  }

  int8_t mqtt_id = this->modem.streamGetIntBefore('\n');
  ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", GF("MQTT %d created"), mqtt_id);
  rc = this->modem.waitResponse();
  if (rc == 0) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020", GF("MQTT %d create ok timed out"),
               mqtt_id);
    return -704;
  } else if (rc != 1) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020", GF("MQTT %d create ok error %d"),
               mqtt_id, rc);
    return -604;
  }
  delay(100);

  return mqtt_id;
}

int16_t SIM7020MqttClient::createClientInstanceExtended() {
  return 0;
}

bool SIM7020MqttClient::disconnect() {
  if (this->mqtt_id > -1) {
    ADVGSM_LOG(GsmSeverity::Info, "SIM7020", GF("MQTT %d disconnect"),
               this->mqtt_id)
    this->modem.sendAT(GF("+CMQDISCON="), this->mqtt_id);
    int16_t rc = this->modem.waitResponse(30000);
    if (rc == 0) {
      ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
                GF("MQTT %d disconnect timed out"), this->mqtt_id);
      return false;
    } else if (rc != 1) {
      ADVGSM_LOG(GsmSeverity::Error, "SIM7020", GF("MQTT %d disconnect failed %d"),
                this->mqtt_id, rc);
      return false;
    }
    this->mqtt_id = -1;
    is_connected = false;
    return true;
  }
  is_connected = false;
  ADVGSM_LOG(GsmSeverity::Warn, "SIM7020", GF("MQTT %d already disconnected"),
              this->mqtt_id)
  return false;
}

bool SIM7020MqttClient::disconnectAll() {
  bool success = true;
  // TODO: Confirm if it already exists and selectively clean up
  for (int8_t client_id = 0; client_id < 1; client_id++) {
    ADVGSM_LOG(GsmSeverity::Info, "SIM7020", GF("MQTT %d disconnect"),
               client_id)
    this->modem.sendAT(GF("+CMQDISCON="), client_id);
    int16_t rc = this->modem.waitResponse(30000);
    if (rc == 0) {
      ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
                GF("MQTT %d disconnect timed out"), this->mqtt_id);
      success = false;
    } else if (rc != 1) {
      ADVGSM_LOG(GsmSeverity::Error, "SIM7020", GF("MQTT %d disconnect failed %d"),
                this->mqtt_id, rc);
      success = false;
    }
  }
  this->mqtt_id = -1;
  is_connected = false;
  return success;
}

void SIM7020MqttClient::loop() {}

// NOTE: Topic max length is 128 characters
bool SIM7020MqttClient::publish(const char topic[], const char payload[]) {
  int8_t qos = 0;
  int8_t retained = 0;
  int8_t duplicate = 0;
  int16_t payload_length = strlen(payload);
  int16_t payload_hex_length = payload_length * 2;
  this->modem.stream.printf(GF("AT+CMQPUB=%d,\"%s\",%d,%d,%d,%d,\""),
                            this->mqtt_id, topic, qos, retained, duplicate,
                            payload_hex_length);
  int16_t payload_index = 0;
  while (payload_index < payload_length) {
    char c = payload[payload_index];
    this->modem.stream.printf("%02x", c);
    payload_index++;
  }
  this->modem.stream.print("\"\r\n");
  int16_t rc = this->modem.waitResponse(5000);
  if (rc == 0) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
               GF("MQTT %d publish '%s' timed out"), this->mqtt_id, topic);
    return false;
  } else if (rc != 1) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020", GF("MQTT %d publish '%s' failed %d"),
               this->mqtt_id, topic, rc);
    return false;
  }

  return true;
}

bool SIM7020MqttClient::subscribe(const char topic[], int qos) {
  this->modem.sendAT(GF("+CMQSUB="), this->mqtt_id, ",\"", topic, "\",", qos);
  int8_t rc = this->modem.waitResponse(5000);
  if (rc == 0) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
               GF("MQTT %d subscribe '%s' timed out"), this->mqtt_id, topic);
    return false;
  } else if (rc != 1) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
               GF("MQTT %d subscribe '%s' failed %d"), this->mqtt_id, topic, rc);
    return false;
  }
  return false;
}

bool SIM7020MqttClient::unsubscribe(const char topic[]) {
  this->modem.sendAT(GF("+CMQUNSUB="), this->mqtt_id, ",\"", topic, '"');
  int8_t rc = this->modem.waitResponse(5000);
  if (rc == 0) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
               GF("MQTT %d unsubscribe '%s' timed out"), this->mqtt_id, topic);
    return false;
  } else if (rc != 1) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
               GF("MQTT %d unsubscribe '%s' failed %d"), this->mqtt_id, topic, rc);
    return false;
  }
  return true;
}