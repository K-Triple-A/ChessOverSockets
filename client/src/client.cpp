#include "../include/client.h"

#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;
chess_piece *Chess::board[n][n] = {nullptr};

extern char player_name[1024];
extern char guest_name[1024];
extern fd_set rset; // used for server crash
extern int maxfdp1; // used for server crash

int Chess::play() {
  int temp = 1;
  if (send(gstFD, &temp, sizeof(int), 0) <= 0) {
    return -1;
  }

  cout << "Searching for players..." << endl;
  int playerColor;
  if (recv(gstFD, &playerColor, sizeof(playerColor), 0) <= 0) {
    std::cerr << "Error receiving guest player color!\n";
    return -1;
  }
  Chess::player = (color)(playerColor);
  int totalReceived = 0, sz = (int)sizeof(guest_name);
  while (totalReceived < sz) {
    FD_ZERO(&rset);
    FD_SET(gstFD,&rset);
    maxfdp1= gstFD + 1;
    select(maxfdp1,&rset,NULL,NULL,NULL);
    if(FD_ISSET(gstFD,&rset))
    {
        int revd = recv(gstFD, guest_name + totalReceived, sz, 0);
        if (revd <= 0) {
           return -1;
        }
        totalReceived += revd;
    }
  }

  return 0;
}

int Chess::makeRoom() {
  int req = 2;
  if (send(gstFD, &req, sizeof(int), 0) <= 0) {
    return -1;
  }
  char roomId[7];
  if(FD_ISSET(gstFD,&rset))
  {
     if (recv(gstFD, roomId, 7, 0) <= 0) {
      return -1;
     }
  }
  cout << "Your room ID is : " << roomId << endl;
  int playerColor;
  FD_ZERO(&rset);
  FD_SET(gstFD,&rset);
  maxfdp1= gstFD + 1;
  select(maxfdp1,&rset,NULL,NULL,NULL);
  if(FD_ISSET(gstFD,&rset))
  {
     if (recv(gstFD, &playerColor, sizeof(playerColor), 0) <= 0) {
        std::cerr << "Error receiving guest player color in create room!\n";
        return -1;
        }
  }
  Chess::player = (color)(playerColor);
  int totalReceived = 0, sz = (int)sizeof(guest_name);
  while (totalReceived < sz) {
     FD_ZERO(&rset);
     FD_SET(gstFD,&rset);
     maxfdp1= gstFD + 1;
     select(maxfdp1,&rset,NULL,NULL,NULL);
     if(FD_ISSET(gstFD,&rset))
     {
          int revd = recv(gstFD, guest_name + totalReceived, sz, 0);
          if (revd <= 0) {
            return -1;
          }
          totalReceived += revd;
     }
  }
  return 0;
}

int Chess::joinRoom() {
  int req = 3;
  if (send(gstFD, &req, sizeof(int), 0) <= 0) {
    return -1;
  }
  cout << "Enter the room ID: ";
  fflush(stdout); // to flush output because we are select with stdin
  char roomId[100];
  do { 
    FD_ZERO(&rset);
    FD_SET(fileno(stdin),&rset);
    FD_SET(gstFD,&rset);
    maxfdp1=max(fileno(stdin),gstFD) + 1;
    select(maxfdp1,&rset,NULL,NULL,NULL);
    if(FD_ISSET(fileno(stdin),&rset))
    {
    cin >> roomId;
    }
    if (strlen(roomId) == 6) {
      send(gstFD, roomId, 7, 0);
      int roomExist = -1;
      if(FD_ISSET(gstFD,&rset))
      {
          if (recv(gstFD, &roomExist, sizeof(int), 0) <= 0) {
           return -1;
           }
      }
      if (roomExist == 1)
        break;
      else {
        cout << "Room ID not exist\nEnter a valid room ID: ";
        fflush(stdout);// to flush output because we are select with stdin
      }
    } else {
      cout << "Enter a 6-characters room ID: ";
      fflush(stdout);// to flush output because we are select with stdin
    }

  } while (1);
  int playerColor;
  FD_ZERO(&rset);
  FD_SET(gstFD,&rset);
  maxfdp1= gstFD + 1;
  select(maxfdp1,&rset,NULL,NULL,NULL);
  if(FD_ISSET(gstFD,&rset))
  {
      if (recv(gstFD, &playerColor, sizeof(playerColor), 0) <= 0) {
      std::cerr << "Error receiving guest player color!\n";
      return -1;
      }
  }
  Chess::player = (color)(playerColor);
  int totalReceived = 0, sz = (int)sizeof(guest_name);
  while (totalReceived < sz) {
    FD_ZERO(&rset);
    FD_SET(gstFD,&rset);
    maxfdp1= gstFD + 1;
    select(maxfdp1,&rset,NULL,NULL,NULL);
    if(FD_ISSET(gstFD,&rset))
    {
         int revd = recv(gstFD, guest_name + totalReceived, sz, 0);
         if (revd <= 0) {
         return -1;
         }
         totalReceived += revd;
    }
  }
  return 0;
}

int Chess::getPlayerColor() { return player; }

//*******************************
void Chess::init_board() {
  color enemy = color(!player);
  for (int j = 0; j < n; j++) {
    board[6][j] = new Pawn(player);
    board[1][j] = new Pawn(enemy);
  }
  board[7][0] = new Rook(player);
  board[7][7] = new Rook(player);
  board[7][1] = new Knight(player);
  board[7][6] = new Knight(player);
  board[7][2] = new Bishop(player);
  board[7][5] = new Bishop(player);
  board[7][3] = new Queen(player);
  board[7][4] = new King(player);

  board[0][0] = new Rook(enemy);
  board[0][7] = new Rook(enemy);
  board[0][1] = new Knight(enemy);
  board[0][6] = new Knight(enemy);
  board[0][2] = new Bishop(enemy);
  board[0][5] = new Bishop(enemy);
  board[0][3] = new Queen(enemy);
  board[0][4] = new King(enemy);
}
void Chess::Print_Killed(color p) {
  for (int i = 0; i < 6; i++) {
    if (chess_piece::killed[p][i] && i == 0) {
      cout << chess_piece::killed[p][i] << 'x' << icons[p][i] << ' ';
    } else {
      int tmp = chess_piece::killed[p][i];
      while (tmp--)
        cout << icons[player][i] << ' ';
    }
  }
}
void Chess::draw_board() {
  system("clear");
  cout << guest_name << ' ';
  Print_Killed(player);
  cout << endl;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      if (j == 0) {
        cout << n - i << ' ' << "| ";
      }
      if (board[i][j]) {
        board[i][j]->print_piece();
      } else {
        cout << "  | ";
      }
    }
    cout << endl;
    for (int j = 0; j < 4 * n + 3; j++) {
      if (j > 1)
        cout << '-';
      else
        cout << ' ';
    }
    cout << endl;
  }
  cout << "    ";
  for (char ch = 'a'; ch <= 'h'; ch++)
    cout << ch << "   ";
  cout << endl;
  cout << player_name << ' ';
  Print_Killed(color(!player));
  cout << endl;
}
//*******************************
bool Chess::make_move(spot from, spot to) {
  if (check_move(from, to)) {
    if (from.x == kingspt.x && from.y == kingspt.y && board[to.x][to.y] &&
        board[to.x][to.y]->get_type() == rook) {
      return do_castling(to);
    }
    update_board(from, to);
    return 1;
  }
  return 0;
}

void Chess::update_board(spot from, spot to) {
  if (board[to.x][to.y] && board[to.x][to.y]->get_color() == !player) {
    chess_piece::killed[!player][board[to.x][to.y]->get_type()]++;
    delete board[to.x][to.y];
    board[to.x][to.y] = NULL;
  }
  swap(board[from.x][from.y], board[to.x][to.y]);
  if (board[to.x][to.y]->get_type() == king) {
    castling = 0;
    kingspt = to;
  }
  if (board[to.x][to.y]->get_type() == rook) {
    castling = 0;
  } else if (to.x == 0 && board[to.x][to.y]->get_type() == pawn) {
    delete board[to.x][to.y];
    board[to.x][to.y] = new Queen(player);
  }
}
bool Chess::check_move(spot from, spot to) {
  // out of bounds
  if (from.x < 0 || from.y < 0 || to.x < 0 || to.y < 0 || from.x >= 8 ||
      from.y >= 8 || to.x >= 8 || to.y >= 8)
    return 0;
  // moving empty spot
  if (board[from.x][from.y] == NULL)
    return 0;
  // moving enemey's piece
  color enemy = color(!player);
  if (board[from.x][from.y]->get_color() == enemy)
    return 0;
  // check castling
  bool sgn = 0;
  if (from.x == kingspt.x && from.y == kingspt.y && board[to.x][to.y] &&
      board[to.x][to.y]->get_type() == rook) {
    if (!castling || mode == checkmate)
      return 0;
    return 1;
  }
  // attacking allied piece
  if (board[to.x][to.y] && board[to.x][to.y]->get_color() == player)
    return 0;
  // not a valid piece move
  if (!board[from.x][from.y]->can_reach(from, to))
    return 0;

  chess_piece *toptr = board[to.x][to.y];
  // any killed
  if (board[to.x][to.y] && board[to.x][to.y]->get_color() == enemy) {
    board[to.x][to.y] = NULL;
  }
  // try to make the move and check the king if is safe
  swap(board[from.x][from.y], board[to.x][to.y]);
  spot temp = kingspt;
  if (board[to.x][to.y]->get_type() == king)
    kingspt = to;
  bool ok = 1;
  if (!safe_spot(kingspt)) {
    ok = 0;
  }
  swap(board[from.x][from.y], board[to.x][to.y]);
  board[to.x][to.y] = toptr;
  kingspt = temp;
  return ok;
}
bool Chess::do_castling(spot to) {
  if (kingspt.x == 7 && kingspt.y == 4 &&
      (to.x == 7 && (to.y == 0 || to.y == 7))) {
    if (kingspt.y < to.y) {
      for (int j = kingspt.y + 1; j < to.y; j++) {
        if (board[7][j])
          return 0;
      }
      for (int j = kingspt.y + 1; j < to.y; j++) {
        if (!safe_spot({7, j}))
          return 0;
      }
      swap(board[kingspt.x][kingspt.y], board[7][6]);
      swap(board[to.x][to.y], board[7][5]);
      kingspt = {7, 6};
      if (!safe_spot(kingspt)) {
        swap(board[kingspt.x][kingspt.y], board[7][6]);
        swap(board[to.x][to.y], board[7][5]);
        kingspt = {7, 4};
        return 0;
      }
    } else {
      for (int j = kingspt.y - 1; j > to.y; j--) {
        if (board[7][j])
          return 0;
      }
      for (int j = kingspt.y - 1; j > to.y; j--) {
        if (!safe_spot({7, j}))
          return 0;
      }
      swap(board[kingspt.x][kingspt.y], board[7][2]);
      swap(board[to.x][to.y], board[7][3]);
      kingspt = {7, 2};
      if (!safe_spot(kingspt)) {
        swap(board[kingspt.x][kingspt.y], board[7][2]);
        swap(board[to.x][to.y], board[7][3]);
        kingspt = {7, 4};
        return 0;
      }
    }
    castling = 0;
    return 1;
  }
  return 0;
}
bool Chess::safe_spot(spot spt) {
  color enemy = color(!player);
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      if (board[i][j] && board[i][j]->get_color() == enemy) { // enemy
        if (board[i][j]->get_type() != pawn) {
          if (board[i][j]->can_reach({i, j}, spt))
            return 0;
        } else {
          if (spt.x - i == 1 && abs(spt.y - j) == 1)
            return 0;
        }
      }
    }
  }
  return 1;
}
king_status Chess::update_status() {
  if (mode == win || mode == draw || mode == win_disconnected)
    return mode;
  bool any_piece_move = can_move();
  if (!safe_spot(kingspt)) {
    if (any_piece_move) {
      return mode = checkmate;
    }
    CleanUP();
    return lose;
  } else {
    if (!any_piece_move) {
      CleanUP();
      return mode = draw;
    }
  }
  return mode = good;
}
bool Chess::can_move() {
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      if (board[i][j] && board[i][j]->get_color() == player)
        for (int k = 0; k < n; k++) {
          for (int u = 0; u < n; u++) {
            if (check_move({i, j}, {k, u})) {
              return 1;
            }
          }
        }
    }
  }
  return 0;
}
//********************************
void Chess::sendmv(pieceMove mv) { send(gstFD, &mv, sizeof(mv), 0); }

king_status Chess::recvmv() {
  spot from = {-1, -1}, to = {-1, -1};
  pieceMove rvdMove;

  int bs = recv(gstFD, &rvdMove, sizeof(rvdMove), 0);

  from = rvdMove.from;
  to = rvdMove.to;
  if (bs <= 0) {
    if (bs == 0 || (bs < 0 && (errno != EAGAIN && errno != EWOULDBLOCK))) {
      return mode = win_disconnected;
    }
    return mode;
  }
  if (from.x == -1) {
    return mode = win;
  } else if (from.x == -2) {
    return mode = draw;

  } else if (from.x == -3) {
    return mode = win_disconnected;
  }
  from.x = 7 - from.x;
  to.x = 7 - to.x;

  // castling case
  if (board[from.x][from.y]->get_type() == king && board[to.x][to.y] &&
      board[to.x][to.y]->get_type() == rook) {
    if (from.y < to.y) {
      swap(board[from.x][from.y], board[0][6]);
      swap(board[to.x][to.y], board[0][5]);
    } else {
      swap(board[from.x][from.y], board[0][2]);
      swap(board[to.x][to.y], board[0][3]);
    }
    return good;
  }

  if (board[to.x][to.y] &&
      board[to.x][to.y]->get_color() == player) { // my piece died!
    chess_piece::killed[player][board[to.x][to.y]->get_type()]++;
    delete board[to.x][to.y];
    board[to.x][to.y] = NULL;
  }
  swap(board[from.x][from.y], board[to.x][to.y]);
  if (to.x == 7 && board[to.x][to.y]->get_type() == pawn) {
    delete board[to.x][to.y];
    color enemy = color(!player);
    board[to.x][to.y] = new Queen(enemy);
  }
  return good;
}
void Chess::CleanUP() {
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      if (board[i][j])
        delete board[i][j];
    }
  }
  close(gstFD);
}
