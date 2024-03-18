#ifndef Advanced_SIM7020TcpClient_h
#define Advanced_SIM7020TcpClient_h

#include "../AdvancedGsm/GsmTcpClient.h"
#include "SIM7020GsmModem.h"

class SIM7020HttpClient;

class SIM7020TcpClient : public GsmTcpClient {
  friend class SIM7020HttpClient;
  friend class SIM7020MqttClient;

 public:
  SIM7020TcpClient(SIM7020GsmModem& modem);

 protected:
  SIM7020GsmModem& modem;

 private:
  bool setTlsCertificate(int8_t type,
                         const char* certificate,
                         int8_t connection_id);
};

#endif