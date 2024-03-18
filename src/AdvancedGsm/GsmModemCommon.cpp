#include "GsmModemCommon.h"

#include <Arduino.h>

// Public

GsmModemCommon::GsmModemCommon(Stream& stream) : stream(stream) {}

void GsmModemCommon::begin(const char access_point_name[],
                           PacketDataProtocolType pdp_type,
                           const char user_name[],
                           const char password[]) {
  this->access_point_name = access_point_name ? access_point_name : "";
  this->pdp_type = pdp_type;
  this->user_name = user_name ? user_name : "";
  this->password = password ? password : "";

  const char* pdp_type_string =
      this->pdp_type == PacketDataProtocolType::IPv4v6 ? "IPV4V6"
      : this->pdp_type == PacketDataProtocolType::IPv6 ? "IPV6"
                                                       : "IP";
  ADVGSM_LOG(GsmSeverity::Info, "GsmModemCommon",
             GF("Begin connection to %s@%s (%s)"), this->user_name,
             this->access_point_name, pdp_type_string);

  this->active = true;
  bool success = checkConnection();
  if (!success) {
    this->retry_count++;
    int32_t delay = retry_base_delay_ms * (1 << (this->retry_count - 1));
    ADVGSM_LOG(GsmSeverity::Debug, "GsmModemCommon",
               "Connection not ready, retry %d delaying for %d ms",
               this->retry_count, delay);
    this->next_check = millis() + delay;
  }
}

int8_t GsmModemCommon::compareIPAddress(const char ip_a[], const char ip_b[]) {
  // For sorting IP Addresses in the order they are likely to be used as a
  // server or appear in logs. Apply RFC 6724, assuming the destination is a
  // public address scope (and then in reducing scope), and preferring stable
  // (over temporary) addresses, e.g. if device is a server. Won't always be
  // correct (e.g. if on local link, or if using IPv4 when v6 available), but
  // provides a reasonable order of preference.

  // Note: When IPAddress supports IPv6, then this can be easier done by byte,
  // rather than string, comparison. (Also IPAddress might get a built in
  // scope() property)

  // IPv6 defined scope, or IPv4 scope equivalence from RFC 6724
  int8_t scope_a;
  int8_t scope_b;

  if (strcmp(ip_a, "::") == 0 || strcmp(ip_a, "0.0.0.0") == 0) {
    // Undefined
    scope_a = 0;
  } else if (strncmp(ip_a, "fe80:", 5) == 0 || strcmp(ip_a, "::1") == 0) {
    // Link-local or loopback
    scope_a = 0x2;
  } else if (strncmp(ip_a, "ff0", 3) == 0) {
    // IPv6 multicast scope
    char scope_hex = tolower(ip_a[3]);
    scope_a = scope_hex < 'a' ? scope_hex - '0' : scope_hex + 10 - 'a';
  } else if (strncmp(ip_a, "169.254.", 8) == 0 ||
             strncmp(ip_a, "127.", 4) == 0) {
    // IPv4 link-local or loopback
    scope_a = 0x2;
  } else {
    // global
    scope_a = 0xe;
  }

  if (strcmp(ip_b, "::") == 0 || strcmp(ip_b, "0.0.0.0") == 0) {
    // Undefined
    scope_b = 0;
  } else if (strncmp(ip_b, "fe80:", 5) == 0 || strcmp(ip_b, "::1") == 0) {
    // Link-local or loopback
    scope_b = 0x2;
  } else if (strncmp(ip_b, "ff0", 3) == 0) {
    // IPv6 multicast scope
    char scope_hex = tolower(ip_b[3]);
    scope_b = scope_hex < 'a' ? scope_hex - '0' : scope_hex + 10 - 'a';
  } else if (strncmp(ip_b, "169.254.", 8) == 0 ||
             strncmp(ip_b, "127.", 4) == 0) {
    // IPv4 link-local or loopback
    scope_b = 0x2;
  } else {
    // global
    scope_b = 0xe;
  }

  ADVGSM_LOG(GsmSeverity::Trace, "GsmModemCommon", "Scope A %s %d vs B %s %d",
             ip_a, scope_a, ip_b, scope_b);

  // Prioritise larger scope (i.e. global) over smaller
  if (scope_b - scope_a != 0) {
    return scope_b - scope_a;
  }

  // From RFC 6724 default table
  int8_t precedence_a;
  int8_t precedence_b;

  if (strcmp(ip_a, "::1") == 0) {
    precedence_a = 50;
  } else if (strncmp(ip_a, "::ffff:0:0:", 11) == 0 ||
             strchr(ip_a, ':') == nullptr) {
    // Any IPv4
    precedence_a = 35;
  } else if (strncmp(ip_a, "2002:", 5) == 0) {
    // 6to4 tunnel 2002://16
    precedence_a = 30;
  } else if (strncmp(ip_a, "2001::", 6) == 0 ||
             strncmp(ip_a, "2001:0:", 7) == 0) {
    // Teredo tunnel 2001::/32
    precedence_a = 5;
  } else if (strncmp(ip_a, "fc", 2) == 0 || strncmp(ip_a, "fd", 2) == 0) {
    // ULA fc00::/7, usually fd..
    precedence_a = 3;
  } else if (strncmp(ip_a, "3ffe:", 5) == 0 || strncmp(ip_a, "fec0:", 5) == 0 ||
             strncmp(ip_a, "::", 2) == 0) {
    // Note: Netmask not correctly checked
    precedence_a = 1;
  } else {
    // Any IPv6
    precedence_a = 40;
  }

  if (strcmp(ip_b, "::1") == 0) {
    precedence_b = 50;
  } else if (strncmp(ip_b, "::ffff:0:0:", 11) == 0 ||
             strchr(ip_b, ':') == nullptr) {
    // Any IPv4
    precedence_b = 35;
  } else if (strncmp(ip_b, "2002:", 5) == 0) {
    // 6to4 tunnel 2002://16
    precedence_b = 30;
  } else if (strncmp(ip_b, "2001::", 6) == 0 ||
             strncmp(ip_b, "2001:0:", 7) == 0) {
    // Teredo tunnel 2001::/32
    precedence_b = 5;
  } else if (strncmp(ip_b, "fc", 2) == 0 || strncmp(ip_b, "fd", 2) == 0) {
    // ULA fc00::/7, usually fd..
    precedence_b = 3;
  } else if (strncmp(ip_b, "3ffe:", 5) == 0 || strncmp(ip_b, "fec0:", 5) == 0 ||
             strncmp(ip_b, "::", 2) == 0) {
    // Note: Netmask not correctly checked
    precedence_b = 1;
  } else {
    // Any IPv6
    precedence_b = 40;
  }

  ADVGSM_LOG(GsmSeverity::Trace, "GsmModemCommon",
             "Precedence A %s %d vs B %s %d", ip_a, precedence_a, ip_b,
             precedence_b);

  // Prioritise larger scope (i.e. global) over smaller
  if (precedence_b - precedence_a != 0) {
    return precedence_b - precedence_a;
  }

  // Tie-break by string comparison, although this could be weird as '9' < ':' <
  // 'a', so '::' sorts in the middle
  return strcmp(ip_a, ip_b);
}

int8_t GsmModemCommon::getLocalIPs(String addresses[], uint8_t max) {
  // TS 27.007: no context should return addresses for all contexts
  // NOTE: +CGPADDR gets the address assigned during the last activation (even
  // if not currently connected)
  this->sendAT(GF("+CGPADDR"));
  bool response_finished = false;
  int8_t address_index = 0;
  while (address_index < max) {
    int8_t response = waitResponse(GFP(GSM_OK), GFP(GSM_ERROR), "+CGPADDR:");
    if (response != 3) {
      response_finished = true;
      break;
    }
    String address_line = this->stream.readStringUntil('\n');

    // Check first returned address
    int start1 = address_line.indexOf('"');
    if (start1 == -1) {
      continue;
    }
    int end1 = address_line.indexOf('"', start1 + 1);
    if (end1 < start1 + 2) {
      continue;
    }
    String address1 = address_line.substring(start1 + 1, end1);
    // Insertion sort in priority order
    int8_t insert1 = address_index;
    while (insert1 > 0 && compareIPAddress(addresses[insert1 - 1].c_str(),
                                           address1.c_str()) > 0) {
      addresses[insert1] = addresses[insert1 - 1];
      insert1--;
    }
    addresses[insert1] = address1;
    if (++address_index >= max) {
      break;
    }

    // Check if there is a second address (index 1)
    int start2 = address_line.indexOf('"', end1 + 1);
    if (start2 == -1) {
      continue;
    }
    int end2 = address_line.indexOf('"', start2 + 1);
    if (end2 < start1 + 2) {
      continue;
    }
    String address2 = address_line.substring(start1 + 1, end1);
    // Insertion sort in priority order
    int8_t insert2 = address_index;
    while (insert2 > 0 && compareIPAddress(addresses[insert2 - 1].c_str(),
                                           address2.c_str()) > 0) {
      addresses[insert2] = addresses[insert2 - 1];
      insert2--;
    }
    addresses[insert2] = address2;
  }
  if (!response_finished) {
    waitResponse();
  }
  return address_index;
}

String GsmModemCommon::ICCID() {
  return "";
}

String GsmModemCommon::IMEI() {
  this->sendAT(GF("+CGSN"));
  String response;
  if (waitResponse(1000, response) != 1) {
    return "unknown";
  }
  response.replace("\r\nOK\r\n", "");
  response.replace("\rOK\r", "");
  response.trim();
  return response;
}

String GsmModemCommon::IMSI() {
  this->sendAT(GF("+CIMI"));
  String response;
  if (waitResponse(1000, response) != 1) {
    return "unknown";
  }
  response.replace("\r\nOK\r\n", "");
  response.replace("\rOK\r", "");
  response.trim();
  return response;
}

bool GsmModemCommon::isActive() {
  return this->active;
}

String GsmModemCommon::localIP(uint8_t index) {
  String addresses[index];
  uint8_t count = getLocalIPs(addresses, index);
  if (count == 0) {
    return "";
  }
  return addresses[count];
}

void GsmModemCommon::loop() {
  // Serial.print("GsmModemCommon::loop\n");
  //  TODO: Heartbeat check on connection
  if (this->active) {
    // If not ready, then check connection status, with back off delay
    if (this->status < ModemStatus::PacketDataReady) {
      if (this->next_check > -1 && millis() > this->next_check) {
        bool success = checkConnection();
        if (success) {
          this->next_check = -1;
        } else {
          this->retry_count++;
          if (this->retry_count > this->retry_max) {
            ADVGSM_LOG(GsmSeverity::Fatal, "GsmModemCommon",
                       "Connection retry count exceeded; modem shutting down");
            this->active = false;
            this->next_check = -1;
            // TODO: high level communication retry after 1 minute /
            // communication sequence retry, with modem hard reset
          } else {
            int32_t delay =
                retry_base_delay_ms * (1 << (this->retry_count - 1));
            ADVGSM_LOG(GsmSeverity::Debug, "GsmModemCommon",
                       "Connection not ready, retry %d delaying for %d ms",
                       this->retry_count, delay);
            this->next_check = millis() + delay;
          }
        }
      }
    }

    // For any unsolicited responses
    this->waitResponse(GSM_COMMAND_DELAY_MS, NULL, NULL);
  }
}

String GsmModemCommon::manufacturer() {
  this->sendAT(GF("+CGMI"));
  String response;
  if (waitResponse(1000, response) != 1) {
    return "unknown";
  }
  response.replace("\r\nOK\r\n", "");
  response.replace("\rOK\r", "");
  response.trim();
  return response;
}

String GsmModemCommon::model() {
  this->sendAT(GF("+CGMM"));
  String response;
  if (waitResponse(1000, response) != 1) {
    return "unknown";
  }
  response.replace("\r\nOK\r\n", "");
  response.replace("\rOK\r", "");
  response.trim();
  return response;
}

ModemStatus GsmModemCommon::modemStatus() {
  return this->status;
}

String GsmModemCommon::network() {
  // Gets the PLMN (Public Land Mobile Network) operator details
  this->sendAT(GF("+COPS?"));
  if (waitResponse("+COPS:") != 1) {
    return "";
  }
  String plmn_details = this->stream.readStringUntil('\n');
  waitResponse();
  int start = plmn_details.indexOf('"');
  if (start == -1) {
    return "";
  }
  int end = plmn_details.indexOf('"', start + 1);
  if (end < start + 2) {
    return "";
  }
  String network = plmn_details.substring(start + 1, end);
  // TODO: Could include the Access Technology, e.g. "(NB-S1)"
  return network;
}

String GsmModemCommon::readResponseLine() {
  return this->stream.readStringUntil('\n');
}

RegistrationStatus GsmModemCommon::registrationStatus() {
  // Registration status results are aligned across versions.
  // Override if needed:
  //  +CREG?
  //  +CGREP? (GPRS)
  //  +CEREG? (EPS)
  //  +C5GREG? (5G)
  this->sendAT(GF("+CEREG?"));
  if (waitResponse("+CEREG:") != 1) {
    return RegistrationStatus::UnknownRegistrationStatus;
  }
  streamSkipUntil(',');  // skip mode
  int16_t status = this->stream.parseInt();
  if (waitResponse() != 1) {
    return RegistrationStatus::UnknownRegistrationStatus;
  }
  return static_cast<RegistrationStatus>(status);
}

bool GsmModemCommon::resetDefaultConfiguration() {
  ADVGSM_LOG(GsmSeverity::Info, "GsmModemCommon",
             "Resetting default configuration");
  sendAT(GF("Z"));
  int8_t rc = waitResponse();
  if (rc != 1) {
    ADVGSM_LOG(GsmSeverity::Warn, "GsmModemCommon", "Reset %s",
               (rc == 0) ? "timed out" : "error");
    return false;
  }
  ADVGSM_LOG(GsmSeverity::Debug, "GsmModemCommon", "Reset success");
  return true;
}

String GsmModemCommon::revision() {
  this->sendAT(GF("+CGMR"));
  String response;
  if (waitResponse(1000, response) != 1) {
    return "unknown";
  }
  response.replace("\r\nOK\r\n", "");
  response.replace("\rOK\r", "");
  response.trim();
  return response;
}

int32_t GsmModemCommon::RSSI() {
  this->sendAT(GF("+CSQ"));
  if (waitResponse("+CSQ:") != 1) {
    return 0;
  }
  int16_t rssi_index = streamGetIntBefore(',');
  if (waitResponse() != 1) {
    return 0;
  }
  if (rssi_index == 99) {
    return 0;
  }
  double rssidBm = -113.0 + (rssi_index * 2);
  return rssidBm;
}

void GsmModemCommon::sendATCommand(const char command[]) {
  streamWrite("AT", command, this->gsmNL);
  this->stream.flush();
}

bool GsmModemCommon::setDns(const char primaryDns[], const char secondaryDns[]) {
  this->sendAT(GF("+CDNSCFG="), "\"", primaryDns, "\",\"", secondaryDns, "\"");
  if (waitResponse() != 1) {
    return false;
  }
  return true;
}

inline int16_t GsmModemCommon::streamGetIntBefore(char lastChar) {
  char buf[7];
  size_t bytesRead =
      this->stream.readBytesUntil(lastChar, buf, static_cast<size_t>(7));
  // if we read 7 or more bytes, it's an overflow
  if (bytesRead && bytesRead < 7) {
    buf[bytesRead] = '\0';
    int16_t res = atoi(buf);
    return res;
  }

  return -9999;
}

inline bool GsmModemCommon::streamSkipUntil(const char c,
                                            const uint32_t timeout_ms) {
  uint32_t startMillis = millis();
  while (millis() - startMillis < timeout_ms) {
    while (millis() - startMillis < timeout_ms && !this->stream.available()) {
      TINY_GSM_YIELD();
    }
    if (this->stream.read() == c) {
      return true;
    }
  }
  return false;
}

int8_t GsmModemCommon::waitResponse() {
  return waitResponse(GSM_OK);
}

int8_t GsmModemCommon::waitResponse(GsmConstStr r1,
                                    GsmConstStr r2,
                                    GsmConstStr r3,
                                    GsmConstStr r4,
                                    GsmConstStr r5) {
  return waitResponse(1000, r1, r2, r3, r4, r5);
}

int8_t GsmModemCommon::waitResponse(uint32_t timeout_ms,
                                    GsmConstStr r1,
                                    GsmConstStr r2,
                                    GsmConstStr r3,
                                    GsmConstStr r4,
                                    GsmConstStr r5) {
  String s;
  return waitResponse(timeout_ms, s, r1, r2, r3, r4, r5);
}

int8_t GsmModemCommon::waitResponse(uint32_t timeout_ms,
                                    String& data,
                                    GsmConstStr r1,
                                    GsmConstStr r2,
                                    GsmConstStr r3,
                                    GsmConstStr r4,
                                    GsmConstStr r5) {
  return checkResponse(timeout_ms, data, r1, r2, r3, r4, r5);
}
