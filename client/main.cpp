#include "include/client.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/select.h>
#include <unistd.h>
#include <limits>

using namespace std;

#define BUFFER_SIZE 1024
#define PORT 8080

const char *IP = "127.0.0.1";

char player_name[1024];
char guest_name[1024];

sockaddr_in serverAddr;

void Print_ASKII_Art()
{
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

int createServerSocket()
{
    int sktFD = -1;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, IP, &serverAddr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if ((sktFD = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error creating socket\n");
        return -1;
    }
    return sktFD;
}

int connectServer(int sktFD)
{
    if (::connect(sktFD, (sockaddr *)&serverAddr, sizeof(serverAddr)))
    {
        perror("Error connecting to the server!\n");
        return -1;
    }
    int sz = BUFFER_SIZE;
    int sent = 0;
    while (sent < sz)
    {
        sent += send(sktFD, player_name + sent, sz - sent, 0);
        if (sent <= 0)
        {
            perror("error sending player name\n");
            return -1;
        }
    }
    return 0;
}

int main()
{
    Print_ASKII_Art();

    int serverFD = createServerSocket();
    if (serverFD < 0)
    {
        exit(1);
    }

    cout << "Enter your name: ";
    cin >> player_name;

    cout << "Connecting to server..." << endl;
    if (connectServer(serverFD) < 0)
    {
        close(serverFD);
        exit(1);
    }
    cout << "Connected successfully!" << endl;

    int op;
    bool accept_draw = 1;
    bool received_draw = 0;
    while (true)
    {
        Chess game(serverFD);
        cout << "1- Play\n2- Create a room\n3- Join a room\n4- quit from game\nEnter (1,2,3,4): ";
        cin >> op;

        if (op == 1)
        {
            if (game.play() < 0)
            {
                perror("Error play game\n");
                continue;
            }
        }
        else if (op == 2)
        {
            int roomId = game.makeRoom();
            if (roomId < 0)
            {
                perror("Error creating room\n");
                continue;
            }
        }
        else if (op == 3)
        {
            int roomId;
            cout << "Enter the room ID: ";
            cin >> roomId;
            if (game.joinRoom(roomId) < 0)
            {
                perror("Error joining room\n");
                continue;
            }
        }
        else if(op == 4)
        {
            send(serverFD, &op, sizeof(int), 0); // to inform server that I want to quit
            close(serverFD);
            break;
        }
        else
        {
            cout << "Enter a valid choice!" << endl;
            continue;
        }
        bool ord = (op == 1 ? 1 : 0);
        if(op==1)
        {
            recv(game.gstFD,&ord,sizeof(int),0); // take order based on who requests to play first
            if(ord == 1)
            {
            game.player=white;
            }
            else
            {
            game.player=black;
            }
        }
        game.init_board();
        game.draw_board();
        bool f = 1;

        while (true)
        {
            if (ord)
            {
                if (f)
                {
                    king_status myst = game.update_status();
                    if (myst == lose)
                    {
                        cout << "You lose!" << endl;
                        game.sendmv({{-1, -1}, {-1, -1}});
                        break;
                    }
                    else if (myst == win)
                    {
                        cout << "You win!" << endl;
                        game.sendmv({{-3,-3},{-3,-3}});
                        break;
                    }
                    else if (myst == draw)
                    {
                        cout << "draw" << endl;
                        if(received_draw == 0)
                        game.sendmv({{-2, -2}, {-2, -2}});
                        else
                        {
                        game.sendmv({{-2, -1}, {-2, -2}});
                        received_draw = 0;
                        }
                        break;
                    }
                    f = 0;
                }
                if(!accept_draw)
                {
                cout << guest_name <<" didn't accept your draw offer"<<endl;
                accept_draw = 1;
                }
                cout << "Enter a move in form like: d2 d4\nif you want to offer draw Enter -4 -4\nif you want to resign Enter -1 -1" << endl;

                char from[3], to[3];
                cin >> from >> to;
                if(from[0] == '-' && from[1] == '1' && to[0] == '-' && to[1] == '1')
                {
                    cout << "You lose!" << endl;
                    game.sendmv({{-1, -1}, {-1, -1}});
                    break;
                }
                else if(from[0] =='-' && from[1]=='4' && to[0] =='-'&& to[1] == '4')
                {
                    cout<<"You offer draw"<< endl;
                    game.sendmv({{-4,-4},{-4,-4}});
                    ord = !ord;
                    f = 1;
                    continue;
                }
                int y1 = from[0] - 'a', x1 = (8 - (from[1] - '0'));
                int y2 = to[0] - 'a', x2 = (8 - (to[1] - '0'));
                if (game.make_move({x1, y1}, {x2, y2}))
                {
                    f = 1;
                    game.draw_board();
                    game.sendmv({{x1, y1}, {x2, y2}});
                    ord = !ord;
                }
                else
                {
                    cout << "Not a valid move!" << endl;
                }
            }
            else
            {
                cout << guest_name << " turn" << endl;
                int returned;
                returned = game.recvmv();
                game.draw_board();
                if(returned != 100)
                ord = !ord;
                if(returned == -1)
                accept_draw = 0;
                if(returned == -2)
                received_draw = 1;
            }
        }
    }
    return 0;
}

