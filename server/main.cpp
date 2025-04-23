#include "include/network_helper.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <thread>

#define PLAY 1
#define CREATE_ROOM 2
#define JOIN_ROOM 3
#define BUFFER_SIZE 1024
#define PORT 8080

void clientHandler(int clientFD) {
  char playerName[BUFFER_SIZE];
  int req;
  int guestFD = -1;

  if (Networking::recvString(clientFD, playerName, sizeof(playerName)) <= 0) {
    perror("Error receiving player name\n");
    return;
  }
  std::cerr << "Player \"" << playerName << "\" has connected to the server"
            << std::endl;
  if (Networking::recvInt(clientFD, &req) <= 0) {
    perror("Error receiving player request\n");
    return;
  }
  std::cerr << "New request: " << req << std::endl;

  if (req == PLAY) {
  } else if (req == CREATE_ROOM) {
  } else if (req == JOIN_ROOM) {
  }

  close(clientFD);
}

int main() {
  Networking server(INADDR_ANY, PORT);

  if (server.bind() < 0) {
    perror("Error binding a socket!\n");
    exit(1);
  }

  if (server.listen() < 0) {
    perror("Error listening to the socket!\n");
    exit(1);
  }

  while (true) {
    int clientFD = -1;
    if ((clientFD = server.accept()) < 0) {
      perror("Error accepting connection\n");
      exit(1);
    }
    std::thread t(clientHandler, clientFD);
    t.detach();
  }

  return 0;
}
