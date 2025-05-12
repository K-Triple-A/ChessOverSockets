#include "include/network_helper.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <thread>

#define PLAY 1
#define CREATE_ROOM 2
#define JOIN_ROOM 3
#define QUIT 4
#define BUFFER_SIZE 1024
#define PORT 8080

sem_t mutex_play;  // used for play
sem_t waiting_player_play; // used for play
int to_play = 0; // used for play
int waiting_player_fd =-1; // used for play
char waiting_player[1024]; // used for play
struct spot
{
    int x, y;
};

struct pieceMove
{
    spot from;
    spot to;
};
void Game_handler (int clientFd1,char* player1,char* player2,int to_send_fd)
{
    pieceMove mymove;
    int req;
    int inner_killed = 1;
    int is_endd = 0;
    while((recv(clientFd1,&mymove,sizeof(mymove),0)>0) && inner_killed)
    {
        if(mymove.from.x == -1) // sender is lost and to_send player is win
        {
           is_endd =1;
        }
        else if(mymove.from.x == -3) // I am the winner
        {
            break;
        }
        else if(mymove.from.x == -2 && mymove.from.y == -1) // game ended to draw from second player
        {
            break;
        }
        send(to_send_fd,&mymove,sizeof(mymove),0);
        while(recv(to_send_fd,&mymove,sizeof(mymove),0)>0)
        {
            if(mymove.from.x == -3) // I am the winner
            {
                inner_killed = 0;
                break;
            }
            else if(mymove.from.x == -2 && mymove.from.y == -1) // game ended to draw from second player
            {
                inner_killed = 0;
                break;
            }
            send(clientFd1,&mymove,sizeof(mymove),0);
            break;
        }
        if(is_endd==1)
        break;
    }
    std::cerr <<"I am "<<player1<<" and my game is over\n";
    std::cerr <<"I am "<<player2<<" and my game is over\n";
}
void clientHandler(int clientFD)
{
    char playerName[BUFFER_SIZE];
    int req;
    int guestFD = -1;
    if (Networking::recvString(clientFD, playerName, sizeof(playerName)) <= 0)
    {
        perror("Error receiving player name\n");
        return;
    }
    std::cerr << "Player \"" << playerName << "\" has connected to the server"
              << std::endl;
    while(1)
    {
        if (Networking::recvInt(clientFD, &req) <= 0)
        {
            perror("Error receiving player request\n");
            close(clientFD);
            return;
        }
        std::cerr << "New request from "<< playerName <<" : "<< req << std::endl;

        if (req == PLAY)
        {
            sem_wait(&mutex_play);

            /* critcal section */
            if(to_play==1)
            {
                std::cerr<<"waiting player is "<<waiting_player<<" and I am "<<playerName<<"\n";
                Networking::sendString(clientFD,waiting_player,sizeof(waiting_player));
                Networking::sendString(waiting_player_fd,playerName,sizeof(playerName));
                Networking::sendInt(waiting_player_fd,&to_play);
                to_play=0;
                Networking::sendInt(clientFD,&to_play);
                sem_post(&mutex_play); // to take new play requests from others
                Game_handler(waiting_player_fd,waiting_player,playerName,clientFD);
                sem_post(&waiting_player_play); // to make waiting player to request new game
            }
            else
            {
                to_play=1;
                waiting_player_fd=clientFD;
                strcpy(waiting_player,playerName);
                sem_post(&mutex_play);
                sem_wait(&waiting_player_play);
            }
        }
        else if (req == CREATE_ROOM)
        {
        }
        else if (req == JOIN_ROOM)
        {
        }
        else if (req == QUIT)// play requests to quit
        {
            std::cerr << "Player  : " << playerName << " has quited from the server" << std::endl;
            break;
        }
    }
    close(clientFD);
}
int main()
{

    sem_init(&mutex_play,0,1);
    sem_init(&waiting_player_play,0,0);
    Networking server(INADDR_ANY, PORT);

    if (server.bind() < 0)
    {
        perror("Error binding a socket!\n");
        exit(1);
    }

    if (server.listen() < 0)
    {
        perror("Error listening to the socket!\n");
        exit(1);
    }

    while (true)
    {
        int clientFD = -1;
        if ((clientFD = server.accept()) < 0)
        {
            perror("Error accepting connection\n");
            exit(1);
        }
        std::thread t(clientHandler, clientFD);
        t.detach();
    }

    return 0;
}
