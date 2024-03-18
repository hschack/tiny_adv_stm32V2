#include "SIM7020HttpClient.h"

SIM7020HttpClient::SIM7020HttpClient(SIM7020TcpClient& client,
                                     const char server_name[],
                                     uint16_t server_port,
                                     bool use_tls)
    : GsmHttpClient(client, server_name, server_port, use_tls),
      modem(client.modem) {
  this->is_connected = false;
  this->http_client_id = -1;
  this->scheme = use_tls ? SCHEME_HTTPS : SCHEME_HTTP;
}

uint8_t SIM7020HttpClient::connected() {
  return is_connected;
};

int16_t SIM7020HttpClient::createClientInstance() {
  ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
             GF("HTTP creating instance %d, %s, %d"), scheme, server_name,
             server_port);

  int16_t rc;
  if (scheme == SCHEME_HTTP) {
    this->modem.sendAT(GF("+CHTTPCREATE=\""), GSM_PREFIX_HTTP, server_name, ':',
                       server_port, "/\"");
  } else if (scheme == SCHEME_HTTPS) {
    this->modem.sendAT(GF("+CHTTPCREATE=\""), GSM_PREFIX_HTTPS, server_name,
                       ':', server_port, "/\"");
  } else {
    return -600;
  }

  // Wait for response
  rc = this->modem.waitResponse(30000, GF(GSM_NL "+CHTTPCREATE:"));
  if (rc == 0) {
    return -703;
  } else if (rc != 1) {
    this->modem.sendAT(GF("+CHTTPCREATE?"));
    this->modem.waitResponse();
    return -603;
  }
  int8_t http_client_id = this->modem.streamGetIntBefore('\n');
  ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", GF("HTTP %d instance created"),
             http_client_id);
  rc = this->modem.waitResponse();
  if (rc == 0) {
    return -704;
  } else if (rc != 1) {
    return -604;
  }

  return http_client_id;
}

int16_t SIM7020HttpClient::createClientInstanceExtended() {
  ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
             GF("HTTP creating extended instance %d, %s, %d"), scheme,
             server_name, server_port);

  int16_t rc;
  // Create with certificate

  const int16_t max_chunk_size = 1000;
  char buffer[max_chunk_size] = {0};

  char comma[] = ",";
  int16_t prefix_length = 8;
  int16_t server_name_length = strlen(server_name);
  int16_t server_port_length =
      1 + (server_port >= 10 ? 1 : 0) + (server_port >= 100 ? 1 : 0) +
      (server_port >= 1000 ? 1 : 0) + (server_port >= 10000 ? 1 : 0);
  int16_t server_ca_length = strlen(this->server_ca);
  int16_t server_ca_length_string_length =
      1 + (server_ca_length >= 10 ? 1 : 0) + (server_ca_length >= 100 ? 1 : 0) +
      (server_ca_length >= 1000 ? 1 : 0) + (server_ca_length >= 10000 ? 1 : 0);
  int16_t server_ca_hex_length = server_ca_length * 2;
  int16_t server_ca_hex_length_string_length =
      1 + (server_ca_hex_length >= 10 ? 1 : 0) +
      (server_ca_hex_length >= 100 ? 1 : 0) +
      (server_ca_hex_length >= 1000 ? 1 : 0) +
      (server_ca_hex_length >= 10000 ? 1 : 0);
  int16_t header_length = prefix_length + server_name_length + 1 +
                          server_port_length + 1 + 1 + 1 + 1;
  ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
             GF("Header length pref=%d nm=%d pt=%d t=%d"), prefix_length,
             server_name_length, server_port_length, header_length);
  int16_t tail_length = 1 + 1 + 1 + 1 + 1 + 1;
  int16_t total_length = header_length + server_ca_hex_length_string_length +
                         1 + server_ca_hex_length + tail_length;
  //  int16_t total_length = header_length + server_ca_length_string_length + 1
  //  + server_ca_hex_length + tail_length;

  int8_t is_more = 1;
  char c = '\0';

  // First chunk (header)
  ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", GF("Create first chunk %d"),
             header_length);
  this->modem.sendAT(GF("+CHTTPCREATEEXT="), is_more, ",", total_length, ",",
                     header_length, ",\"", GSM_PREFIX_HTTPS, server_name, ':',
                     server_port, "/,,,\"");
  rc = this->modem.waitResponse();
  if (rc == 0) {
    return -721;
  } else if (rc != 1) {
    return -621;
  }

  // CA Cert chunks
  int16_t cert_index = 0;
  int16_t cert_section_end = 0;
  while (cert_index < server_ca_length) {
    if (cert_index == 0) {
      cert_section_end =
          (max_chunk_size - server_ca_hex_length_string_length - 1) / 2;
      // cert_section_end = (max_chunk_size - server_ca_length_string_length -
      // 1) / 2;
    } else {
      cert_section_end += max_chunk_size / 2;
    }
    if (cert_section_end > server_ca_length) {
      cert_section_end = server_ca_length;
    }
    int16_t chunk_length = (cert_section_end - cert_index) * 2;
    if (cert_index == 0) {
      chunk_length = chunk_length + server_ca_hex_length_string_length + 1;
    }
    ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
               GF("Loop chunk %d, cert start %d, end %d"), chunk_length,
               cert_index, cert_section_end);
    this->modem.stream.print(GF("AT+CHTTPCREATEEXT="));
    this->modem.stream.print(is_more);
    this->modem.stream.print(',');
    this->modem.stream.print(total_length);
    this->modem.stream.print(',');
    this->modem.stream.print(chunk_length);
    this->modem.stream.print(",\"");
    if (cert_index == 0) {
      this->modem.stream.print(server_ca_hex_length);
      // this->modem.stream.print(server_ca_length);
      this->modem.stream.print(",");
    }
    while (cert_index < cert_section_end) {
      c = this->server_ca[cert_index];
      this->modem.stream.printf("%02x", c);
      cert_index++;
    }
    this->modem.stream.print("\"\r\n");
    ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", GF("Wait http response"));
    rc = this->modem.waitResponse();
    if (rc == 0) {
      return -722;
    } else if (rc != 1) {
      return -622;
    }
  }

  // Last chunk (tail)
  is_more = 0;
  this->modem.sendAT(GF("+CHTTPCREATEEXT="), is_more, ",", total_length, ",",
                     tail_length, ",\",0,,0,\"");

  // Wait for response
  rc = this->modem.waitResponse(30000, GF(GSM_NL "+CHTTPCREATEEXT:"));
  if (rc == 0) {
    return -723;
  } else if (rc != 1) {
    this->modem.sendAT(GF("+CHTTPCREATE?"));
    this->modem.waitResponse();
    return -623;
  }
  int8_t http_client_id = this->modem.streamGetIntBefore('\n');
  ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
             GF("HTTP %d extended instance created"), http_client_id);
  rc = this->modem.waitResponse();
  if (rc == 0) {
    return -724;
  } else if (rc != 1) {
    return -624;
  }

  return http_client_id;
}

SIM7020GsmModem& SIM7020HttpClient::getModem() {
  return this->modem;
}

bool SIM7020HttpClient::setClientCA(const char certificate[]) {
  this->server_ca = certificate;
  return true;
}

bool SIM7020HttpClient::setClientPrivateKey(const char certificate[]) {
  this->client_certificate = certificate;
  return true;
}

bool SIM7020HttpClient::setRootCA(const char certificate[]) {
  this->server_ca = certificate;
  return true;
}

int SIM7020HttpClient::startRequest(const char url_path[],
                                    const char http_method[],
                                    const char content_type[],
                                    int content_length,
                                    const byte body[]) {
  ADVGSM_LOG(GsmSeverity::Info, "SIM7020", GF("HTTP start %s %s (%d, %d)"),
             http_method, url_path, is_connected, http_client_id);
  int16_t rc;
  // Connect if needed
  if (!is_connected) {
    // Create if needed
    if (this->http_client_id == -1) {
      if (scheme == SCHEME_HTTPS && this->server_ca != nullptr) {
        rc = createClientInstanceExtended();
      } else {
        rc = createClientInstance();
      }
      if (rc < 0) {
        return rc;
      }

      // Store the connection
      this->http_client_id = rc;
      this->modem.http_clients[this->http_client_id] = this;
    }

    ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", GF("HTTP %d connecting"),
               http_client_id);

    // Connect
    this->modem.sendAT(GF("+CHTTPCON="), this->http_client_id);
    rc = this->modem.waitResponse(60000);
    if (rc == 0) {
      return -710;
    } else if (rc != 1) {
      return -610;
    }

    is_connected = true;
  }

  // Send the request
  if (strcmp(http_method, HTTP_METHOD_GET) == 0) {
    this->modem.sendAT(GF("+CHTTPSEND="), this->http_client_id, ",0,",
                       url_path);
    rc = this->modem.waitResponse(5000);
    if (rc == 0) {
      return -711;
    } else if (rc != 1) {
      return -611;
    }
    // } else if (strcmp(http_method, GSM_HTTP_METHOD_POST) == 0) {

    // } else if (strcmp(http_method, GSM_HTTP_METHOD_PUT) == 0) {

    // } else if (strcmp(http_method, GSM_HTTP_METHOD_DELETE) == 0) {

  } else {
    return -610;
  }

  return 0;
}

void SIM7020HttpClient::stop() {
  if (this->http_client_id > -1) {
    this->modem.sendAT(GF("+CHTTPDISCON="), this->http_client_id);
    this->modem.waitResponse(30000);
    this->modem.sendAT(GF("+CHTTPDESTROY="), this->http_client_id);
    this->modem.waitResponse(30000);
  }
}
