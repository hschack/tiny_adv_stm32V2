#ifndef Advanced_GsmModem_h
#define Advanced_GsmModem_h

#include "Shared.h"

#define GSM_NL "\r\n"
// #define GSM_OK "OK" GSM_NL
// #define GSM_ERROR "ERROR" GSM_NL
#define GSM_OK "OK"
#define GSM_ERROR "ERROR"
// The DTE shall not begin issuing a subsequent command line until at least
// one-tenth of a second has elapsed after receipt of the entire result code
// issued by the DCE
#define GSM_COMMAND_DELAY_MS 100

// AccessTechnology
//  GSM = 0,
//  GSM_EGPRS = 3,
//  E_UTRAN_NBS1 = 9 // NB-IoT

enum PacketDataProtocolType { IP, IPv6, IPv4v6 };

enum RegistrationStatus {
  Inactive = 0,  // Not registered, and not searching
  RegisteredHome = 1,
  Searching = 2,  // Not registered, but is currently searching
  Denied = 3,
  UnknownRegistrationStatus = 4,
  RegisteredRoaming = 5,
  RegisteredHomeSmsOnly = 6,
  RegisteredRoamingSmsOnly = 7,
  EmergencyOnly = 8,
  RegisteredHomeNoCircuitSwitchedFallback = 9,
  RegisteredRoamingNoCircuitSwitchedFallback = 10
};

enum ModemStatus {
  UnknownModemStatus = 0,
  Attention = 10,
  Configured = 20,
  SimReady = 30,
  HasSignal = 40,
  Registered = 50,
  PacketDataReady = 60
};

class GsmModem {
 public:
  virtual void begin(const char access_point_name[],
                     PacketDataProtocolType pdp_type = IPv4v6,
                     const char username[] = "",
                     const char password[] = "") = 0;
  // virtual int8_t getLocalIPs(IPAddress addresses[], uint8_t max) = 0;
  virtual int8_t getLocalIPs(String addresses[], uint8_t max) = 0;
  virtual String ICCID() = 0;
  virtual String IMEI() = 0;
  virtual String IMSI() = 0;
  virtual bool isActive() = 0;
  // virtual IPAddress localIP(uint8_t index = 0) = 0
  virtual String localIP(uint8_t index = 0) = 0;
  virtual void loop() = 0;
  virtual String manufacturer() = 0;
  virtual String model() = 0;
  virtual ModemStatus modemStatus() = 0;
  virtual String network() = 0;
  virtual String readResponseLine() = 0;
  virtual RegistrationStatus registrationStatus() = 0;
  virtual bool resetDefaultConfiguration() = 0;
  virtual String revision() = 0;
  // RSSI is in dBm
  virtual int32_t RSSI() = 0;
  //  virtual uint8_t status() = 0
  virtual void sendATCommand(const char command[]) = 0;
  virtual bool setClientCA(const String& certificate) = 0;
  virtual bool setClientPrivateKey(const String& certificate) = 0;
  virtual bool setDns(const char primaryDns[], const char secondaryDns[]) = 0;
  virtual bool setRootCA(const String& certificate) = 0;
};

#endif