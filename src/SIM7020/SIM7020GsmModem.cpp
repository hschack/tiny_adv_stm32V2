#include "SIM7020GsmModem.h"
#include "SIM7020HttpClient.h"
#include "SIM7020MqttClient.h"

#include "../AdvancedGsm/GsmLog.h"

#include <Arduino.h>

SIM7020GsmModem::SIM7020GsmModem(Stream& stream) : GsmModemCommon(stream) {}

bool SIM7020GsmModem::checkConnection() {
  int8_t rc;
  if (this->status < ModemStatus::Attention) {
    sendAT(GF(""));
    rc = waitResponse();
    if (rc != 1) {
      ADVGSM_LOG(GsmSeverity::Warn, "SIM7020", "Attention %s",
                 (rc == 0) ? "timed out" : "error");
      return false;
    }
    ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", "Attention success");
    this->status = ModemStatus::Attention;
    delay(GSM_COMMAND_DELAY_MS);
  }

  // Basic settings -- should respond straight away
  if (this->status < ModemStatus::Attention + 1) {
    // Echo Off
    sendAT(GF("E0"));
    rc = waitResponse();
    if (rc != 1) {
      ADVGSM_LOG(GsmSeverity::Warn, "SIM7020", "Setting echo off %s",
                 (rc == 0) ? "timed out" : "error");
      return false;
    }
    this->status = (ModemStatus)(ModemStatus::Attention + 1);
    delay(GSM_COMMAND_DELAY_MS);
  }

  if (this->status < ModemStatus::Attention + 2) {
    // TODO: Support error codes, e.g. E2
    sendAT(GF("+CMEE=0"));
    rc = waitResponse();
    if (rc != 1) {
      ADVGSM_LOG(GsmSeverity::Warn, "SIM7020", "Setting disable error codes %s",
                 (rc == 0) ? "timed out" : "error");
      return false;
    }
    this->status = (ModemStatus)(ModemStatus::Attention + 2);
    delay(GSM_COMMAND_DELAY_MS);
  }

  // if (this->status < ModemStatus::Attention + 3) {
  //   // Enable (Unsolicited) Local Time Stamp for getting network time
  //   sendAT(GF("+CLTS=1"));
  //   rc = waitResponse();
  //   if (rc != 1) {
  //     ADVGSM_LOG(GsmSeverity::Warn, "SIM7020", "Setting enable timestamp %s",
  //                (rc == 0) ? "timed out" : "error");
  //     return false;
  //   }
  //   this->status = (ModemStatus)(ModemStatus::Attention + 3);
  //   delay(GSM_COMMAND_DELAY_MS);
  // }

  // if (this->status < ModemStatus::Attention + 4) {
  //   // Enable battery checks
  //   sendAT(GF("+CBATCHK=1"));
  //   rc = waitResponse();
  //   if (rc != 1) {
  //     ADVGSM_LOG(GsmSeverity::Warn, "SIM7020",
  //                "Setting enable battery checks %s",
  //                (rc == 0) ? "timed out" : "error");
  //     return false;
  //   }
  //   this->status = (ModemStatus)(ModemStatus::Attention + 4);
  //   delay(GSM_COMMAND_DELAY_MS);
  // }

  // if (this->status < ModemStatus::Attention + 5) {
  //   // Set IPv6 format
  //   sendAT(GF("+CGPIAF=1,1,0,1"));
  //   rc = waitResponse();
  //   if (rc != 1) {
  //     ADVGSM_LOG(GsmSeverity::Warn, "SIM7020", "Setting IPv6 address format %s",
  //                (rc == 0) ? "timed out" : "error");
  //     return false;
  //   }
  //   this->status = (ModemStatus)(ModemStatus::Attention + 5);
  //   delay(GSM_COMMAND_DELAY_MS);
  // }

  // if (this->status < ModemStatus::Configured) {
  //   bool is_configured = confirmPacketDataConfiguration();
  //   if (!is_configured) {
  //     return false;
  //   }
  //   this->status = ModemStatus::Configured;
  //   delay(GSM_COMMAND_DELAY_MS);
  // }

  // TODO: SIM status, i.e. +CPIN: READY; this happens after resetting the modem

  if (this->status < ModemStatus::HasSignal) {
    int32_t rssi_dbm = this->RSSI();
    if (rssi_dbm == 0) {
      ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", "Waiting for signal");
      return false;
    }
    ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
               "Received Signal Strength Indicator: %d dBm", rssi_dbm);
    this->status = ModemStatus::HasSignal;
    delay(GSM_COMMAND_DELAY_MS);
  }

  if (this->status < ModemStatus::Registered) {
    RegistrationStatus registration_status = this->registrationStatus();
    switch (registration_status) {
      case RegistrationStatus::RegisteredHome:
      case RegistrationStatus::RegisteredRoaming:
        break;
      default:
        ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
                   "Waiting for registration (%d)%s", registration_status,
                   (registration_status == RegistrationStatus::Searching)
                       ? ": Searching"
                       : "");
        return false;
    }
    ADVGSM_LOG(
        GsmSeverity::Debug, "SIM7020", "Registered (%d)%s", registration_status,
        (registration_status == RegistrationStatus::RegisteredHome) ? ": Home"
                                                                    : "");
    this->status = ModemStatus::Registered;
    delay(GSM_COMMAND_DELAY_MS);
  }

  if (this->status < ModemStatus::PacketDataReady - 1) {
    // SIM7020 Manual says "AT+CGCONTRDP=[<cid>]", however TS 27.007 says
    // "AT+CGCONTRDP[=<cid>]", and adding "=0" returns ERROR
    sendAT(GF("+CGCONTRDP"));
    rc = waitResponse("+CGCONTRDP:");
    if (rc != 1) {
      ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
                 "Waiting for packet data context dynamic parameters %s",
                 (rc == 0) ? "timed out" : "error");
      return false;
    }
    rc = waitResponse();
    ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", "Packet data ready");
    this->status = (ModemStatus)(ModemStatus::PacketDataReady - 1);
    delay(GSM_COMMAND_DELAY_MS);
  }

  if (this->status < ModemStatus::PacketDataReady) {
    String addresses[4];
    int8_t count = this->getLocalIPs(addresses, 4);
    if (count == 0) {
      ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", "Waiting for IP address");
      return false;
    }
    ADVGSM_LOG(GsmSeverity::Info, "SIM7020", "Local IP Addresses: %s%s%s",
               addresses[0].c_str(), count > 0 ? ", " : "",
               count > 0 ? addresses[1].c_str() : "");
    this->status = ModemStatus::PacketDataReady;
    delay(GSM_COMMAND_DELAY_MS);
  }

  return true;
}

bool SIM7020GsmModem::confirmPacketDataConfiguration() {
  int8_t rc;

  // Based on "APN Manual Configuration", from SIM7020 TCPIP Application Note
  const char* pdp_type_string =
      this->pdp_type == PacketDataProtocolType::IPv4v6 ? "IPV4V6"
      : this->pdp_type == PacketDataProtocolType::IPv6 ? "IPV6"
                                                       : "IP";
  char configuration_string[512] = {0};
  int16_t len1 =
      snprintf(configuration_string, sizeof(configuration_string),
               "\"%s\",\"%s\"", pdp_type_string, this->access_point_name);
  if (this->user_name && strlen(this->user_name) > 0) {
    int16_t len2 = snprintf(configuration_string + len1,
                            sizeof(configuration_string) - len1, ",\"%s\"",
                            this->user_name);
    if (this->password && strlen(this->password) > 0) {
      int16_t len3 = snprintf(configuration_string + len1 + len2,
                              sizeof(configuration_string) - len1 - len2,
                              ",\"%s\"", this->password);
    }
  }

  // Check existing default context
  sendAT(GF("*MCGDEFCONT?"));
  rc = waitResponse("*MCGDEFCONT:");
  if (rc != 1) {
    ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", "Getting default context %s",
               (rc == 0) ? "timed out" : "error");
  } else {
    String default_context = this->stream.readStringUntil('\n');
    default_context.trim();
    if (default_context == configuration_string) {
      ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
                 "Already configured: %s@%s (%s)", this->user_name,
                 this->access_point_name, pdp_type_string);
      return true;
    }
  }

  ADVGSM_LOG(GsmSeverity::Info, "SIM7020",
             "Reconfiguring default packet data context: %s@%s (%s)",
             this->user_name, this->access_point_name, pdp_type_string);

  sendAT(GF("+CFUN=0"));
  rc = waitResponse();
  if (rc != 1) {
    ADVGSM_LOG(GsmSeverity::Warn, "SIM7020", "Disabling radio %s",
               (rc == 0) ? "timed out" : "error");
  }
  delay(GSM_COMMAND_DELAY_MS);

  // Set Default PSD Connection Settings
  // Set the user name and password
  // AT*MCGDEFCONT=<PDP_type>[,<APN>[,<username>[,<password>]]]
  // <pdp_type> IPV4V6: Dual PDN Stack
  //            IPV6: Internet Protocol Version 6
  //            IP: Internet Protocol Version 4
  //            Non-IP: external packet data network
  sendAT(GF("*MCGDEFCONT="), configuration_string);
  rc = waitResponse();
  if (rc != 1) {
    ADVGSM_LOG(GsmSeverity::Warn, "SIM7020", "Setting default context %s",
               (rc == 0) ? "timed out" : "error");
    return false;
  }
  delay(GSM_COMMAND_DELAY_MS);

  sendAT(GF("+CFUN=1"));
  rc = waitResponse();
  if (rc != 1) {
    ADVGSM_LOG(GsmSeverity::Warn, "SIM7020", "Enabling radio %s",
               (rc == 0) ? "timed out" : "error");
    return false;
  }

  ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
             "Default packet data context reconfigured");
  return true;
}

int8_t SIM7020GsmModem::checkResponse(uint32_t timeout_ms,
                                      String& data,
                                      GsmConstStr r1,
                                      GsmConstStr r2,
                                      GsmConstStr r3,
                                      GsmConstStr r4,
                                      GsmConstStr r5) {
  /*String r1s(r1); r1s.trim();
  String r2s(r2); r2s.trim();
  String r3s(r3); r3s.trim();
  String r4s(r4); r4s.trim();
  String r5s(r5); r5s.trim();
  DBG("### ..:", r1s, ",", r2s, ",", r3s, ",", r4s, ",", r5s);*/
  data.reserve(64);
  uint8_t index = 0;
  uint32_t finish_millis = millis() + timeout_ms;
  do {
    TINY_GSM_YIELD();
    while (this->stream.available() > 0) {
      TINY_GSM_YIELD();
      int8_t a = stream.read();
      if (a <= 0)
        continue;  // Skip 0x00 bytes, just in case

      data += static_cast<char>(a);
      if (r1 && data.endsWith(r1)) {
        index = 1;
        goto finish;
      } else if (r2 && data.endsWith(r2)) {
        index = 2;
        goto finish;
      } else if (r3 && data.endsWith(r3)) {
#if defined TINY_GSM_DEBUG
        if (r3 == GFP(GSM_CME_ERROR)) {
          streamSkipUntil('\n');  // Read out the error
        }
#endif
        index = 3;
        goto finish;
      } else if (r4 && data.endsWith(r4)) {
        index = 4;
        goto finish;
      } else if (r5 && data.endsWith(r5)) {
        index = 5;
        goto finish;
      } else {
        if (checkUnsolicitedResponse(data))
          continue;
        if (checkUnsolicitedHttpResponse(data))
          continue;
        if (checkUnsolicitedMqttResponse(data))
          continue;
      }
    }
  } while (millis() < finish_millis);
finish:
  if (!index) {
    data.trim();
    if (data.length()) {
      ADVGSM_LOG(GsmSeverity::Warn, "SIM7020", "Unhandled: %s", data.c_str());
    }
    data = "";
  }
  // data.replace(GSM_NL, "/");
  // DBG('<', index, '>', data);
  return index;
}

bool SIM7020GsmModem::checkUnsolicitedHttpResponse(String& data) {
  if (data.endsWith(GF("+CHTTPNMIH:"))) {
    int8_t http_client_id = streamGetIntBefore(',');
    SIM7020HttpClient* http_client = http_clients[http_client_id];
    int16_t response_code = streamGetIntBefore(',');
    ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", "HTTP %d got response code %d",
               http_client_id, response_code);
    if (http_client != nullptr) {
      http_client->response_status_code = response_code;
    }
    int16_t header_length = streamGetIntBefore(',');
    if (header_length > 0) {
      for (int i = 0; i < header_length; i++) {
        uint32_t startMillis = millis();
        while (!stream.available() && (millis() - startMillis < 1000)) {
          TINY_GSM_YIELD();
        }
        char c = stream.read();
        if (http_client != nullptr) {
          http_client->headers[i] = c;
        }
      }
      http_client->headers[header_length] = '\0';
    }
    ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", "HTTP %d header length %d",
               http_client_id, header_length);
    data = "";
    return true;
  } else if (data.endsWith(GF("+CHTTPNMIC:"))) {
    int8_t http_client_id = streamGetIntBefore(',');
    SIM7020HttpClient* http_client = http_clients[http_client_id];
    int16_t more_flag = streamGetIntBefore(',');
    int16_t content_length = streamGetIntBefore(',');
    int16_t package_length = streamGetIntBefore(',');
    if (package_length > 0) {
      int16_t previous_data_length = strlen(http_client->body);
      char hex[3] = {0, 0, 0};
      ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", "HTTP %d reading hex %d to %d ",
                 http_client_id, previous_data_length,
                 previous_data_length + package_length);
      for (int i = previous_data_length;
           i < previous_data_length + package_length; i++) {
        uint32_t startMillis = millis();
        while (!stream.available() && (millis() - startMillis < 1000)) {
          TINY_GSM_YIELD();
        }
        hex[0] = stream.read();
        hex[1] = stream.read();
        if (http_client != nullptr) {
          http_client->body[i] = strtol(hex, NULL, 16);
        }
      }
      http_client->body[previous_data_length + package_length] = '\0';
    }
    if (more_flag == 0) {
      http_client->body_completed = true;
    }
    ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
               "HTTP %d got content length %d (more %d)", http_client_id,
               package_length, more_flag);
    data = "";
    return true;
  } else if (data.endsWith(GF("+CHTTPERR:"))) {
    int8_t http_client_id = streamGetIntBefore(',');
    int8_t error_code = streamGetIntBefore('\n');
    SIM7020HttpClient* http_client = http_clients[http_client_id];
    http_client->is_connected = false;
    if (http_client_id >= 0) {
      if (error_code == -2) {
        // <error code> -2 = closed by remote host (expected automatic
        // disconnection)
        ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
                   "HTTP %d closed by remote host", http_client_id);
      } else {
        ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
                   "### HTTP %d closed with error: %d", http_client_id,
                   error_code);
      }
    }
    data = "";
    return true;
  }
  return false;
}

bool SIM7020GsmModem::checkUnsolicitedMqttResponse(String& data) {
  if (data.endsWith(GF("+CMQPUB:"))) {
    int8_t mqtt_id = streamGetIntBefore(',');
    SIM7020MqttClient* mqtt_client = mqtt_clients[mqtt_id];
    streamSkipUntil('"');
    String topic = this->stream.readStringUntil('"');
    streamSkipUntil(',');
    int8_t qos = streamGetIntBefore(',');
    int8_t retained = streamGetIntBefore(',');
    int8_t duplicate = streamGetIntBefore(',');
    int16_t payload_hex_length = streamGetIntBefore(',');
    int16_t payload_length = payload_hex_length / 2;
    ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", "MQTT %d received topic '%s'",
               mqtt_id, topic.c_str());
    streamSkipUntil('"');
    if (payload_hex_length > 0) {
      char hex[3] = {0, 0, 0};
      for (int i = 0; i < payload_length; i++) {
        uint32_t startMillis = millis();
        while (!stream.available() && (millis() - startMillis < 1000)) {
          TINY_GSM_YIELD();
        }
        hex[0] = stream.read();
        hex[1] = stream.read();
        if (mqtt_client != nullptr && i < mqtt_client->BodyBufferSize - 1) {
          mqtt_client->received_body[i] = strtol(hex, NULL, 16);
        }
      }
      mqtt_client->received_body[payload_length] = '\0';
    }
    streamSkipUntil('\n');
    ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", "MQTT %d received length %d",
               mqtt_id, payload_length);
    topic.toCharArray(mqtt_client->received_topic,
                      mqtt_client->TopicBufferSize);
    data = "";
    return true;
  }
  return false;
}

bool SIM7020GsmModem::checkUnsolicitedResponse(String& data) {
  if (data.endsWith(GF("+CLTS:"))) {
    //        streamSkipUntil('\n');  // Refresh time and time zone by
    //        network
    data = "";
    //        DBG("### Unsolicited local timestamp.");
    return true;
  } else if (data.endsWith(GF("+CTZV:"))) {
    //        streamSkipUntil('\n');  // Refresh network time zone by
    //        network
    data = "";
    //        DBG("### Unsolicited time zone updated.");
    return true;
  } else if (data.endsWith(GF(GSM_NL "SMS Ready" GSM_NL))) {
    data = "";
    //        DBG("### Unexpected module reset!");
    //        init();
    data = "";
    return true;
  }
  return false;
}

int8_t SIM7020GsmModem::getLocalIPs(String addresses[], uint8_t max) {
  // SIM7020 requires to specify the context ID to query addresses, i.e.
  // "+CGPADDR=1" works (but "+CGPADDR" does not). The value returned for IPv6
  // (address 2) is only the suffix:
  //   +CGPADDR: 1,"10.89.132.147","::5B5B:EA87:AF0C:8447"
  //
  // Reading the dynamic parameters ("AT+CGCONTRDP") shows the DNS settings, but
  // also only has the IPv6 suffix:
  //   +CGCONTRDP:
  //   1,5,"telstra.iot","10.89.132.147.255.255.255.0",,"101.168.244.101","101.168.244.103",,,,,1500
  //   +CGCONTRDP:
  //   1,5,"telstra.iot","::5B5B:EA87:AF0C:8447/64",,"2001:8004:2D43:C00::","2001:8004:2C42:B16::1",,,,,1500
  //
  // The "+IPCONFIG" command shows full addresses
  // Before connection, it has a single localhost address "127.0.0.1".
  // Once the context is connected (as above), it shows: link local IPv6
  // ("fe80..."), IPv4, and localhost (It can calculate the link local from the
  // suffix) After getting the network prefix it then shows four addresses: link
  // local IPv6, then global IPv6, then IPv4, then localhost
  //   +IPCONFIG: fe80:0:0:0:719d:1439:899a:42d7
  //   +IPCONFIG: 2001:8004:4810:0:719d:1439:899a:42d7
  //   +IPCONFIG: 10.88.134.167
  //   +IPCONFIG: 127.0.0.1
  //
  // The global IPv6 address has to wait for a router advertisement with the
  // prefix (not exposed in AT commands)
  //
  // Rather than the above order, use insert sort to sort the largest scope
  // (global) first, then in RFC 6724 precedence order, which puts the two
  // public addresses first:
  //
  //   2001:8004:4810:0:719d:1439:899a:42d7
  //   10.88.134.167
  //   fe80:0:0:0:719d:1439:899a:42d7
  //   127.0.0.1
  // AT+IPCONFIG in 7020
  this->sendAT(GF("+CGPADDR"));  //reths

  bool response_finished = false;
  int8_t address_index = 0;
  while (address_index < max) {
    int8_t response = waitResponse(GFP(GSM_OK), GFP(GSM_ERROR), "+CGPDDR:");
    if (response != 3) {
      response_finished = true;
      break;
    }
    String address_line = this->stream.readStringUntil('\n');
    address_line.trim();
    // Insertion sort in priority order
    int8_t insert = address_index;
    while (insert > 0 && compareIPAddress(addresses[insert - 1].c_str(),
                                          address_line.c_str()) > 0) {
      addresses[insert] = addresses[insert - 1];
      insert--;
    }
    addresses[insert] = address_line;
    address_index++;
  }
  if (!response_finished) {
    waitResponse();
  }
  return address_index;
}

String SIM7020GsmModem::ICCID() {
  this->sendAT(GF("+CCID"));
  String response;
  if (waitResponse(1000, response) != 1) {
    return "unknown";
  }
  response.replace("\r\nOK\r\n", "");
  response.replace("\rOK\r", "");
  response.trim();
  return response;
}

// TODO: Handle these, e.g. move the HTTP / MQTT destroy into the relevant
// sections

// sendAT(GF("Z"));  // Reset (to user settings)
// if (waitResponse(30000) != 1) {
//   return false;
// }

// Clean up any old connections
// TODO: Check the list of what exists first
// for (int8_t client_id = 0; client_id < 5; client_id++) {
//   sendAT(GF("+CHTTPDESTROY="), client_id);
//   waitResponse();
// }

// for (int8_t client_id = 0; client_id < 5; client_id++) {
// for (int8_t client_id = 0; client_id < 1; client_id++) {
//   sendAT(GF("+CMQDISCON="), client_id);
//   waitResponse();
// }

// SimStatus ret = getSimStatus();
// // if the sim isn't ready and a pin has been provided, try to unlock the
// sim if (ret != SIM_READY && pin != NULL && strlen(pin) > 0) {
//   simUnlock(pin);
//   return (getSimStatus() == SIM_READY);
// } else {
//   // if the sim is ready, or it's locked but no pin has been provided,
//   // return true
//   return (ret == SIM_READY || ret == SIM_LOCKED);
// }

bool SIM7020GsmModem::setCertificate(int8_t type, const char* certificate) {
  /*  type 0 : Root CA
      type 1 : Client CA
      type 2 : Client Private Key
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
             GF("Set certificate %d length %d with %d escaped characters"),
             type, length, count_escaped);
  int16_t total_length = length + count_escaped;

  // NOTE: If a certificate (or partial) already exists, there is no way to
  // clear it; instead, ERROR is returned when you exceed the length (usually
  // the first command), and it is cleared, and you need to start again. Allow
  // (and ignore) one error.

  int8_t error_count = 0;
  bool success = false;

  while (!success && error_count < 2) {
    int8_t is_more = 1;
    int16_t index = 0;
    int16_t chunk_end = 0;
    char c = '\0';

    int8_t result_code;
    while (index < length) {
      chunk_end += chunk_size;
      if (chunk_end >= length) {
        chunk_end = length;
        is_more = 0;
      }

      stream.print(GF("AT+CSETCA="));
      stream.print(type);
      stream.print(',');
      stream.print(total_length);
      stream.print(',');
      stream.print(is_more);
      stream.print(",0,\"");

      while (index < chunk_end) {
        c = certificate[index];
        if (c == '\r') {
          stream.print("\\r");
        }
        if (c == '\n') {
          stream.print("\\n");
        } else {
          stream.print(c);
        }
        index++;
      }
      stream.print("\"" GSM_NL);
      result_code = waitResponse();
      if (result_code != 1) {
        break;
      }
    }
    if (result_code == 0) {
      ADVGSM_LOG(GsmSeverity::Error, "SIM7020",
                 GF("Set certificate timed out"));
      return false;
    } else if (result_code != 1) {
      error_count++;
      ADVGSM_LOG(GsmSeverity::Debug, "SIM7020",
                 GF("Set certificate error %d result at index %d (first error "
                    "is expected, to clear)"),
                 error_count, index);
      continue;
    }
    success = true;
  }
  if (!success) {
    ADVGSM_LOG(GsmSeverity::Error, "SIM7020", GF("Set certificate error"));
    return false;
  }
  ADVGSM_LOG(GsmSeverity::Debug, "SIM7020", GF("Certificate set on attempt %d"),
             error_count + 1);
  return true;
}

bool SIM7020GsmModem::setClientCA(const String& certificate) {
  return setCertificate(1, certificate.c_str());
}

bool SIM7020GsmModem::setClientPrivateKey(const String& certificate) {
  return setCertificate(2, certificate.c_str());
}

bool SIM7020GsmModem::setRootCA(const String& certificate) {
  return setCertificate(0, certificate.c_str());
}
