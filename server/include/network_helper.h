#include <arpa/inet.h>
#include <unistd.h>

class Networking {
private:
  int serverFD;
  sockaddr_in addr;

public:
  Networking(in_addr_t IP, int port);
  int bind();
  int listen();
  int accept();
  void handle();

  static int sendString(int clientFD, char *buff, size_t buffSize);
  static int recvString(int clientFD, char *buff, size_t buffSize);
  static int recvInt(int clientFD, int *val);
  static int sendInt(int clientFD, int *val);
};
