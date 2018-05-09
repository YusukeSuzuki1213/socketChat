#include "exp1.h"
#include "exp1lib.h"

int main(int argc, char** argv){
  int sock;
  int ret=0;
  
  if(argc != 2){
    printf("usage: %s [ip address]\n",argv[0] );
    exit(-1);
  }
  
  sock = exp1_tcp_connect(argv[1],11111);
  
  /*接続が成功したら*/
  if(sock != -1 && ret != 1){
    while(1){
      ret = exp1_login(sock);
      if(ret == 1){
        break;
      }else if(ret == -2){
        close(sock);
        return -1;
      }
    }
  }
  
  while(ret ==1){
    ret = exp1_do_talk(sock);
  }

  close(sock);
}
