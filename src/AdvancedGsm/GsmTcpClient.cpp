#include "GsmTcpClient.h"

// GsmTcpClient::GsmTcpClient(GsmModem& modem) : modem(modem) {}

// GsmModem& GsmTcpClient::getModem() {
//   return this->modem;
// }

// Client.h implementation

// TODO: Create these

int GsmTcpClient::connect(IPAddress ip, uint16_t port) {
  return 0;
}
int GsmTcpClient::connect(const char* host, uint16_t port) {
  return 0;
}
size_t GsmTcpClient::write(uint8_t) {
  return 0;
}
size_t GsmTcpClient::write(const uint8_t* buf, size_t size) {
  return 0;
}
int GsmTcpClient::available() {
  return 0;
}
int GsmTcpClient::read() {
  return 0;
}
int GsmTcpClient::read(uint8_t* buf, size_t size) {
  return 0;
}
int GsmTcpClient::peek() {
  return 0;
}
void GsmTcpClient::flush() {}
void GsmTcpClient::stop() {}
uint8_t GsmTcpClient::connected() {
  return 0;
}
GsmTcpClient::operator bool() {
  return false;
}
