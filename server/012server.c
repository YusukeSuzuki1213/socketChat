#include "exp1.h"
#include "exp1lib.h"

int main(int argc, char** argv){
  int sock_listen; 
  exp1_init_clients();
  sock_listen = exp1_tcp_listen(11111);

  while (1) {
    exp1_do_server(sock_listen);
  }
  close(sock_listen);
}
