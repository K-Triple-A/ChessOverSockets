#include "include/client.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/select.h>
#include <unistd.h>

using namespace std;

#define BUFFER_SIZE 1024
#define PORT 8080

const char *IP = "127.0.0.1";

char player_name[1024];
char guest_name[1024];

sockaddr_in serverAddr;

void Print_ASKII_Art() {
  cout << "    _____ _                          \n"
       << "   / ____| |                         \n"
       << "  | |    | |__   ___  ___ ___        \n"
       << "  | |    | '_ \\ / _ \\/ __/ __|       \n"
       << "  | |____| | | |  __/\\__ \\__ \\       \n"
       << "   \\_____|_| |_|\\___||___/___/       \n"
       << "    / __ \\                           \n"
       << "   | |  | |_   _____ _ __            \n"
       << "   | |  | \\ \\ / / _ \\ '__|           \n"
       << "   | |__| |\\ V /  __/ |              \n"
       << "   _\\____/  \\_/ \\___|_|      _       \n"
       << "  / ____|          | |      | |      \n"
       << " | (___   ___   ___| | _____| |_ ___ \n"
       << "  \\___ \\ / _ \\ / __| |/ / _ \\ __/ __|\n"
       << "  ____) | (_) | (__|   <  __/ |_\\__ \\\n"
       << " |_____/ \\___/ \\___|_|\\_\\___|\\__|___/\n";
}

int createServerSocket() {
  int sktFD = -1;
  memset(&serverAddr, 0, sizeof(serverAddr));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(PORT);

  if (inet_pton(AF_INET, IP, &serverAddr.sin_addr) <= 0) {
    perror("Invalid address/ Address not supported");
    return -1;
  }

  if ((sktFD = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Error creating socket\n");
    return -1;
  }
  return sktFD;
}

int connectServer(int sktFD) {
  if (::connect(sktFD, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
    perror("Error connecting to the server!\n");
    return -1;
  }
  int sz = BUFFER_SIZE;
  int sent = 0;
  while (sent < sz) {
    sent += send(sktFD, player_name + sent, sz - sent, 0);
    if (sent <= 0) {
      perror("error sending player name\n");
      return -1;
    }
  }
  return 0;
}

int main() {
  Print_ASKII_Art();

  int serverFD = createServerSocket();
  if (serverFD < 0) {
    exit(1);
  }

  cout << "Enter your name: ";
  cin >> player_name;

  cout << "Connecting to server..." << endl;
  if (connectServer(serverFD) < 0) {
    close(serverFD);
    exit(1);
  }
  cout << "Connected successfully!" << endl;

  int op;
  while (true) {
    Chess game(serverFD);

    cout << "1- Play\n2- Create a room\n3- Join a room\nEnter (1,2,3): ";
    cin >> op;

    if (op == 1) {
      if (game.play() < 0) {
        perror("Error play game\n");
        continue;
      }
    } else if (op == 2) {
      if (game.makeRoom() < 0) {
        perror("Error creating room\n");
        continue;
      }
    } else if (op == 3) {
      if (game.joinRoom() < 0) {
        perror("Error joining room\n");
        continue;
      }
    } else {
      cout << "Enter a valid choice!" << endl;
      continue;
    }

    game.init_board();
    game.draw_board();
    bool ord = game.getPlayerColor();
    bool f = 1;

    while (true) {
      if (!ord) {
        if (f) {
          king_status myst = game.update_status();
          if (myst == lose) {
            cout << "You lose!" << endl;
            game.sendmv({{-1, -1}, {-1, -1}});
            break;
          } else if (myst == win) {
            cout << "You win!" << endl;
            break;
          } else if (myst == draw) {
            cout << "draw" << endl;
            game.sendmv({{-2, -2}, {-2, -2}});
            break;
          }
          f = 0;
        }
        cout << "Enter a move in form like: d2 d4" << endl;

        char from[3], to[3];
        cin >> from >> to;
        int y1 = from[0] - 'a', x1 = (8 - (from[1] - '0'));
        int y2 = to[0] - 'a', x2 = (8 - (to[1] - '0'));
        if (game.make_move({x1, y1}, {x2, y2})) {
          f = 1;
          game.draw_board();
          game.sendmv({{x1, y1}, {x2, y2}});
          ord = !ord;
        } else {
          cout << "Not a valid move!" << endl;
        }
      } else {
        cout << guest_name << " turn" << endl;
        game.recvmv();
        game.draw_board();
        ord = !ord;
      }
    }
  }
  return 0;
}
