#include "include/network_helper.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sys/socket.h>
#include <thread>

#define PLAY 1
#define CREATE_ROOM 2
#define JOIN_ROOM 3
#define BUFFER_SIZE 1024
#define PORT 8080
std::mutex mtx, rooms;
std::map<int, std::string> players;
// allocatedRooms[i] = clientFD -> one player waiting in that room
std::map<std::string, int> allocatedRooms;
static const char charset[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
std::string getRoomCode() {
  static std::mt19937 rng(std::random_device{}());
  static std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
  std::string id;
  for (int i = 0; i < 6; i++) {
    id += charset[dist(rng)];
  }
  return id;
}
std::string generateUniqueRoomId(int clientFD) {
  std::lock_guard<std::mutex> lock(rooms);
  std::string roomId;
  do {
    roomId = getRoomCode();
  } while (allocatedRooms.find(roomId) != allocatedRooms.end());
  allocatedRooms[roomId] = clientFD;
  return roomId;
}
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
    int currentFD = (round == 0) ? whiteFD : blackFD;
    int waitingFD = (round == 0) ? blackFD : whiteFD;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(currentFD, &readfds);
    FD_SET(waitingFD, &readfds);

    int maxfd = (whiteFD > blackFD) ? whiteFD : blackFD;

    // Wait for either socket to be readable (blocking)
    int ready = select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);
    if (ready < 0) {
      std::cerr << "select() error\n";
      break;
    }

    // Check if waiting player disconnected before current player's move
    if (FD_ISSET(waitingFD, &readfds)) {
      // Peek to see if disconnected
      char buf;
      ssize_t peekRet = recv(waitingFD, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
      if (peekRet == 0 ||
          (peekRet < 0 && (errno != EAGAIN && errno != EWOULDBLOCK))) {
        // Waiting player disconnected
        std::cerr << "Player with FD " << waitingFD
                  << " disconnected (waiting player).\n";

        // Notify current player of disconnection
        pieceMove disconnectedMove = {{-3, -3}, {-3, -3}};
        send(currentFD, &disconnectedMove, sizeof(disconnectedMove), 0);

        // Cleanup
        shutdown(currentFD, SHUT_RDWR);
        shutdown(waitingFD, SHUT_RDWR);
        close(whiteFD);
        close(blackFD);
        std::lock_guard<std::mutex> lock(mtx);
        players.erase(whiteFD);
        players.erase(blackFD);
        return;
      }
      // Unexpected data from waiting player? You can ignore or handle if needed
    }

    // Now check if current player sent a move
    if (FD_ISSET(currentFD, &readfds)) {
      pieceMove mv;
      ssize_t ret = recv(currentFD, &mv, sizeof(mv), 0);
      if (ret <= 0) {
        // Current player disconnected or error
        std::cerr << "Player with FD " << currentFD
                  << " disconnected (current player).\n";

        // Notify waiting player
        pieceMove disconnectedMove = {{-3, -3}, {-3, -3}};
        send(waitingFD, &disconnectedMove, sizeof(disconnectedMove), 0);

        // Cleanup
        close(whiteFD);
        close(blackFD);
        std::lock_guard<std::mutex> lock(mtx);
        players.erase(whiteFD);
        players.erase(blackFD);
        return;
      }

      if (mv.from.x < 0) {
        // Game end signal or special move
        close(whiteFD);
        close(blackFD);
        std::lock_guard<std::mutex> lock(mtx);
        players.erase(whiteFD);
        players.erase(blackFD);
        return;
      }

      // Forward move to waiting player
      ret = send(waitingFD, &mv, sizeof(mv), 0);
      if (ret <= 0) {
        // Opponent disconnected during send
        std::cerr << "Opponent with FD " << waitingFD
                  << " disconnected during send.\n";
        close(whiteFD);
        close(blackFD);
        std::lock_guard<std::mutex> lock(mtx);
        players.erase(whiteFD);
        players.erase(blackFD);
        return;
      }

      // Switch turns
      round = !round;
    }
  }

  // Cleanup at loop exit
  close(whiteFD);
  close(blackFD);
  std::lock_guard<std::mutex> lock(mtx);
  players.erase(whiteFD);
  players.erase(blackFD);
}

void clientHandler(int clientFD) {
  char playerName[BUFFER_SIZE];
  int req;

  if (Networking::recvString(clientFD, playerName, sizeof(playerName)) <= 0) {
    perror("Error receiving player name\n");
    close(clientFD);
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
    std::string temp = generateUniqueRoomId(clientFD);
    char roomId[7];
    for (int i = 0; i < 6; i++) {
      roomId[i] = temp[i];
    }
    roomId[6] = '\0';
    Networking::sendString(clientFD, roomId, 7);
  } else if (req == JOIN_ROOM) {
    while (1) {
      char roomId[7];
      int received = Networking::recvString(clientFD, roomId, 7);
      if (received != 7) {
        std::cerr << "Failed to receive room code\n";
        break;
      }
      int guestFD = -1;
      {
        std::lock_guard<std::mutex> lock(rooms);
        if (allocatedRooms.find(roomId) != allocatedRooms.end()) {
          guestFD = allocatedRooms[roomId];
          allocatedRooms.erase(roomId);
        }
      }
      int roomExist = (guestFD == -1 ? -1 : 1);
      // send the room status
      Networking::sendInt(clientFD, &roomExist);
      if (guestFD != -1) {
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
        break;
      }
    }
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
