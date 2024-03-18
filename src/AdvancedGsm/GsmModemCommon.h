#ifndef Advanced_GsmModemCommon_h
#define Advanced_GsmModemCommon_h

#include "../api/GsmModem.h"
#include "GsmLog.h"

#include <Stream.h>

class GsmModemCommon : public GsmModem {
 public:
  GsmModemCommon(Stream& stream);
  void begin(const char access_point_name[],
             PacketDataProtocolType pdp_type,
             const char username[],
             const char password[]) override;
  int8_t getLocalIPs(String addresses[], uint8_t max) override;
  String ICCID() override;
  String IMEI() override;
  String IMSI() override;
  bool isActive() override;
  String localIP(uint8_t index = 0) override;
  void loop() override;
  String manufacturer() override;
  String model() override;
  ModemStatus modemStatus() override;
  String network() override;
  String readResponseLine() override;
  RegistrationStatus registrationStatus() override;
  bool resetDefaultConfiguration() override;
  String revision() override;
  int32_t RSSI() override;
  void sendATCommand(const char command[]) override;
  bool setDns(const char primaryDns[], const char secondaryDns[]) override;
  int8_t waitResponse();
  int8_t waitResponse(GsmConstStr r1,
                      GsmConstStr r2 = GFP(GSM_ERROR),
                      GsmConstStr r3 = NULL,
                      GsmConstStr r4 = NULL,
                      GsmConstStr r5 = NULL);
  int8_t waitResponse(uint32_t timeout_ms,
                      GsmConstStr r1 = GFP(GSM_OK),
                      GsmConstStr r2 = GFP(GSM_ERROR),
                      GsmConstStr r3 = NULL,
                      GsmConstStr r4 = NULL,
                      GsmConstStr r5 = NULL);
  int8_t waitResponse(uint32_t timeout_ms,
                      String& data,
                      GsmConstStr r1 = GFP(GSM_OK),
                      GsmConstStr r2 = GFP(GSM_ERROR),
                      GsmConstStr r3 = NULL,
                      GsmConstStr r4 = NULL,
                      GsmConstStr r5 = NULL);

 protected:
  const char* access_point_name;
  bool active = false;
  char gsmNL[3] = GSM_NL;
  int32_t next_check = -1;
  const char* password;
  PacketDataProtocolType pdp_type;
  int16_t retry_base_delay_ms = 250;
  int8_t retry_count = 0;
  int8_t retry_max = 8;
  ModemStatus status;
  Stream& stream;
  const char* user_name;

  virtual bool checkConnection() = 0;
  int8_t compareIPAddress(const char ip_a[], const char ip_b[]);
  // inline int16_t streamGetIntBefore(char lastChar);
  virtual int16_t streamGetIntBefore(char lastChar);
  virtual bool streamSkipUntil(const char c, const uint32_t timeout_ms = 1000L);
  //  template <typename... Args>
  //  void sendAT(Args... command);

  // inline bool streamSkipUntil(const char c, const uint32_t timeout_ms =
  // 1000L):
  virtual int8_t checkResponse(uint32_t timeout_ms,
                               String& data,
                               GsmConstStr r1 = GFP(GSM_OK),
                               GsmConstStr r2 = GFP(GSM_ERROR),
                               GsmConstStr r3 = NULL,
                               GsmConstStr r4 = NULL,
                               GsmConstStr r5 = NULL) = 0;

  // Define template functions in the header, so that derived classes
  // instantiate See:
  // https://www.modernescpp.com/index.php/surprise-included-inheritance-and-member-functions-of-class-templates
 protected:
  template <typename... Args>
  void sendAT(Args... command) {
    // Serial.print("GsmModemCommon::sendAT\n");
    streamWrite("AT", command..., this->gsmNL);
    this->stream.flush();
  };

 private:
  template <typename T>
  inline void streamWrite(T last) {
    this->stream.print(last);
  }

  template <typename T, typename... Args>
  inline void streamWrite(T head, Args... tail) {
    this->stream.print(head);
    streamWrite(tail...);
  }
};

#endif
