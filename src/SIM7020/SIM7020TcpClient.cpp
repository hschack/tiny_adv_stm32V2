#include "SIM7020TcpClient.h"

SIM7020TcpClient::SIM7020TcpClient(SIM7020GsmModem& modem) : modem(modem) {}

bool SIM7020TcpClient::setTlsCertificate(int8_t type,
                                         const char* certificate,
                                         int8_t connection_id) {
  /*  type 0 : Root CA, TLS parameter 6
      type 1 : Client CA, TLS parameter 7
      type 2 : Client Private Key, TLS parameter 8
  */
  const int16_t chunk_size = 500;
  if (certificate == NULL) {
    return false;
  }

  int16_t length = strlen(certificate);
  int16_t count_escaped = 0;
  for (int16_t i = 0; i < length; i++) {
    if (certificate[i] == '\r' || certificate[i] == '\n') {
      count_escaped++;
    }
  }
  ADVGSM_LOG(GsmSeverity::Info, "SIM7020",
             GF("Set TLS certificate %d length %d with %d escaped characters"),
             type, length, count_escaped);
  int16_t total_length = length + count_escaped;
  int8_t is_more = 1;
  int16_t index = 0;
  int16_t chunk_end = 0;
  char c = '\0';

  while (index < length) {
    chunk_end += chunk_size;
    if (chunk_end >= length) {
      chunk_end = length;
      is_more = 0;
    }

    int8_t mux_type = 6 + type;
    this->modem.stream.print(GF("AT+CTLSCFG="));
    this->modem.stream.print(connection_id);
    this->modem.stream.print(',');
    this->modem.stream.print(mux_type);
    this->modem.stream.print(',');
    this->modem.stream.print(total_length);
    this->modem.stream.print(',');
    this->modem.stream.print(is_more);
    this->modem.stream.print(",\"");

    while (index < chunk_end) {
      c = certificate[index];
      if (c == '\r') {
        this->modem.stream.print("\\r");
      }
      if (c == '\n') {
        this->modem.stream.print("\\n");
      } else {
        this->modem.stream.print(c);
      }
      index++;
    }
    this->modem.stream.print("\"" GSM_NL);
    int8_t rc = this->modem.waitResponse();
    if (rc == 0) {
      ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
                 GF("Set certificate timed out"));
      return false;
    } else if (rc != 1) {
      ADVGSM_LOG(GsmSeverity::Error, "SIM7020", GF("Set certificate error"));
      return false;
    }
  }
  return true;
}
