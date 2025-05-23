#include "pieces.h"
enum king_status { good, checkmate, draw, lose, win };
const char icons[2][6][7] = {{"♟︎", "♜", "♞", "♝", "♚", "♛"},
                             {"♙", "♖", "♘", "♗", "♔", "♕"}};
const int n = 8;

struct pieceMove {
  spot from;
  spot to;
};

class Chess {
private:
  int gstFD; // guest player socket file descriptor
  spot kingspt;
  king_status mode;
  bool castling;
  color player; // white player starts first
public:
  static chess_piece *board[n][n]; // Each spot contains a pointer to piece

  Chess(int sktFD) : mode(good), castling(1), kingspt({7, 4}), gstFD(sktFD) {}

  int play(); // searchs for available players and returns the guest name and
              // starts the game
  int makeRoom();           // makes a room and returns roomId
  int joinRoom();
  void init_board(); // puts and initializes players pieces on the board
  void CleanUP();
  void draw_board(); // clears console and prints current board
  void Print_Killed(color);
  int getPlayerColor();

  king_status update_status();
  bool make_move(spot, spot);
  bool check_move(spot, spot);
  bool do_castling(spot);
  bool safe_spot(spot);
  bool can_move();
  void update_board(spot, spot);

  void sendmv(pieceMove);
  void recvmv();
};
