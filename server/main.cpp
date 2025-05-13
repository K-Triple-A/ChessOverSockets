#include "include/network_helper.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <sys/socket.h>
#include <thread>

#define PLAY 1
#define CREATE_ROOM 2
#define JOIN_ROOM 3
#define BUFFER_SIZE 1024
#define PORT 8080

std::mutex mtx;

std::map<int, std::string> players;

// shared variables
int playable = 0;

struct spot {
  int x;
  int y;
};
struct pieceMove {
  spot from;
  spot to;
};

void setPlayable(int val) {
  std::lock_guard<std::mutex> lock(mtx);
  playable = val;
}

int getPlayable() {
  std::lock_guard<std::mutex> lock(mtx);
  int ret = playable;
  playable = 0;
  return ret;
}

void gameHandler(int whiteFD, int blackFD) {
  int round = 0; // 0 -> white, 1 -> black;
  while (true) {
    pieceMove mv;
    recv((round == 0 ? whiteFD : blackFD), &mv, sizeof(mv), 0);
    send((round == 0 ? blackFD : whiteFD), &mv, sizeof(mv), 0);
    if (mv.from.x < 0)
      return;
    round = !round;
  }
}

void clientHandler(int clientFD) {
  char playerName[BUFFER_SIZE];
  int req;

  if (Networking::recvString(clientFD, playerName, sizeof(playerName)) <= 0) {
    perror("Error receiving player name\n");
    return;
  }
  std::cout << "Player \"" << playerName
            << "\" has connected to the server, FD = " << clientFD << std::endl;

  players[clientFD] = playerName;

  if (Networking::recvInt(clientFD, &req) <= 0) {
    perror("Error receiving player request\n");
    return;
  }

  std::cout << "New request: " << req << std::endl;
  if (req == PLAY) {
    int guestFD = getPlayable();
    std::cerr << "isPlayable = " << guestFD << std::endl;
    if (guestFD) {
      std::string &guestName = players[guestFD];
      char temp[BUFFER_SIZE];
      temp[guestName.size()] = '\0';
      for (int i = 0; i < guestName.size(); i++) {
        temp[i] = guestName[i];
      }
      int playerColor = 0;
      Networking::sendInt(clientFD, &playerColor);
      Networking::sendString(clientFD, temp, sizeof(temp));
      playerColor = !playerColor;
      Networking::sendInt(guestFD, &playerColor);
      Networking::sendString(guestFD, playerName, sizeof(playerName));

      gameHandler(clientFD, guestFD);

      /* close(clientFD);
      close(guestFD); */
    } else {
      setPlayable(clientFD);
    }
  } else if (req == CREATE_ROOM) {
  } else if (req == JOIN_ROOM) {
  }
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

  std::cout << "Server is up and running..\n";

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
