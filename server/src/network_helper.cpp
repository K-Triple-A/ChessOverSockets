#include "../include/network_helper.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

Networking::Networking(in_addr_t IP, int port) {
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = IP;
}

int Networking::bind() {
  if ((serverFD = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Error creating socket\n");
    return -1;
  }
  if (::bind(serverFD, (sockaddr *)&addr, sizeof(addr)) == -1) {
    close(serverFD);
    return -1;
  }
  return 0;
}

int Networking::listen() {
  if (::listen(serverFD, 500) == -1) {
    close(serverFD);
    return -1;
  }
  return 0;
}

int Networking::accept() {
  int clientFD = ::accept(serverFD, nullptr, nullptr);
  if (clientFD < 0) {
    close(serverFD);
    return -1;
  }
  return clientFD;
}

int Networking::sendString(int clientFD, char *buff, size_t buffSize) {
  int sz = (int)buffSize;
  int sent = 0;
  while (sent < sz) {
    sent += send(clientFD, buff + sent, sz - sent, 0);
    if (sent <= 0) {
      perror("error sending player name\n");
      return -1;
    }
  }
  return sent;
}

int Networking::recvString(int clientFD, char *buff, size_t buffSize) {
  int totalReceived = 0, sz = (int)buffSize;
  while (totalReceived < buffSize) {
    int revd = recv(clientFD, buff + totalReceived, sz, 0);
    if (revd <= 0) {
      return -1;
    }
    totalReceived += revd;
  }
  return totalReceived;
}

int Networking::sendInt(int clientFD, int *val) {
  return send(clientFD, val, sizeof(int), 0);
}

int Networking::recvInt(int clientFD, int *val) {
  return recv(clientFD, val, sizeof(int), 0);
}
