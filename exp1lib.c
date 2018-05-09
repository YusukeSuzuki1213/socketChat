#include "exp1.h"
#include "exp1lib.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CLIENTS 10//最大クライアント数
#define NAME_SIZE 128//idの文字列の上限
#define PASS_SIZE 128//passwordの文字列の上限
#define IPV4_SIZE 15//ipアドレス
#define MESSAGE_SIZE 1024//送ることができる文字列の上限
#define REGISTERE_NUM 10//登録できるユーザ数の上限
#define LOGIN_RESPONSE_NUM 64//なんだっけ？
#define MAX_DM_NUM_BEFORE_LOGINED 5//未ログイン時に受け取ることができるDM数の上限

struct t_Clients{
  int is_accept;//accept済みか
  int is_auth;//認証済みか
  char ip_address[IPV4_SIZE];
  int  port_no;
  double auth_time;//ログインした時間
  int reg_data_index;//reg_dataの要素数(idなどを拾ってくる)
};

struct t_Reg_data{
  char id[NAME_SIZE];
  char password[PASS_SIZE];
  char dm_before_logined[MAX_DM_NUM_BEFORE_LOGINED][MESSAGE_SIZE+NAME_SIZE+2];//未ログイン時に送られてきたDM
  int dm_num;//未ログイン時に送られてきたDMの数
};


struct t_Reg_data reg_data[REGISTERE_NUM] = {
  {"yusuke","suzuki",{{0}},0},
  {"test1","test",{{0}},0},
  {"test2","test",{{0}},0},
  {"test3","test",{{0}},0},
  {"test4","test",{{0}},0},
  //{-1, {0}, -1,{0},"","",{0},0},
};

struct t_Clients clients[MAX_CLIENTS];

int exp1_tcp_listen(int port) {
  int sock;
  struct sockaddr_in addr;
  int yes = 1;
  int ret;

  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket");
    exit(1);
  }

  bzero((char *) &addr, sizeof(addr));
  addr.sin_family = PF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  ret = bind(sock, (struct sockaddr *) &addr, sizeof(addr));
  if (ret < 0) {
    perror("bind");
    exit(1);
  }

  ret = listen(sock, 5);
  if (ret < 0) {
    perror("reader: listen");
    close(sock);
    exit(-1);
  }

  return sock; //return  server's socket descriptor
}

int exp1_tcp_connect(const char *hostname, int port) {
  int sock;
  int ret;
  struct sockaddr_in addr;
  struct hostent *host;

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  addr.sin_family = AF_INET;
  host = gethostbyname(hostname);
  addr.sin_addr = *(struct in_addr *) (host->h_addr_list[0]);
  addr.sin_port = htons(port);
  ret = connect(sock, (struct sockaddr *) &addr, sizeof addr);
  
  if (ret < 0) {
    return -1;
  } else {
    return sock;
  }
}

double gettimeofday_sec() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec * 1e-6;
}

int exp1_do_talk(int sock) {
  fd_set fds;
  char buf[MESSAGE_SIZE+NAME_SIZE];
  
  FD_ZERO(&fds);
  FD_SET(0, &fds);
  FD_SET(sock, &fds);
  
  select(sock+1, &fds, NULL, NULL, NULL);
  
  if (FD_ISSET(0, &fds) != 0 ) {
    fgets(buf, MESSAGE_SIZE, stdin);
    write(sock, buf, strlen(buf));
  }

  if (FD_ISSET(sock, &fds) != 0 ) {
    int ret = recv(sock, buf, MESSAGE_SIZE+NAME_SIZE, 0);
    if ( ret > 0 ) {
      write(1, buf, ret);
    }
    else {
      return -1;
    }
  }
  return 1;
}

void exp1_init_clients()
{
  for(int i = 0; i < MAX_CLIENTS; i++){
    clients[i].is_accept = 0;
    clients[i].is_auth=0;
    strcpy(clients[i].ip_address,"\0");
    clients[i].port_no = -1;
    clients[i].auth_time = -1.0;
    clients[i].reg_data_index=-1;
  }
  
}

/* ソケットディスクリプタの集合を初期化して、serverのソケットディスクリプタを登録してあげるよ。 */
void exp1_set_fds(fd_set* pfds, int accept_sock)
{
  int i;

  FD_ZERO(pfds);
  FD_SET(0,pfds);
  FD_SET(accept_sock, pfds);

  /*クライアントのソケットディスクリプタを集合に登録してあげるよ*/
  for(i = 0; i < MAX_CLIENTS; i++){
    if(clients[i].is_accept == 1){
      FD_SET(i, pfds);
    }
  }
}

/*接続可能なクライアントとして登録してあげるよ*/
void exp1_add(int sock,char* ip,int port)
{
  printf("add:%d\n",sock);
  if(sock < MAX_CLIENTS){
    clients[sock].is_accept = 1;
    strcpy(clients[sock].ip_address,ip);
    clients[sock].port_no = port;
    clients[sock].auth_time= gettimeofday_sec();
  }else{
    printf("connection overflow\n");
    exit(-1);
  }
}

void exp1_remove(int id)
{
  printf("remove:%d\n",id);
  clients[id].is_accept = 0;
  clients[id].is_auth = 0;
  strcpy(clients[id].ip_address,"\0");
  clients[id].port_no = -1;
  clients[id].auth_time=-1.0;
  
  for(int i=0; i<reg_data[clients[id].reg_data_index].dm_num;i++){
    strcpy(reg_data[clients[id].reg_data_index].dm_before_logined[i],"\0");
  }
  reg_data[clients[id].reg_data_index].dm_num=0;
  clients[id].reg_data_index= -1;
}

/* 現在ある通信可能なクライアントの数を教えてあげるよ */
int exp1_get_max_sock()
{
  int i;
  int max_sock = 0;

  for(i = 0; i < MAX_CLIENTS; i++){
    if(clients[i].is_accept == 1){
      max_sock = i;
    }
  }

  return max_sock;
}

void exp1_broadcast(char* buf, int size, int from)
{
  int i;

  for(i = 0; i < MAX_CLIENTS; i++){
    if(i == from){
      continue;
    }

    if(clients[i].is_accept == 1 && clients[i].is_auth==1){
      write(i, buf, size);
    }
  }
  
}

void exp1_do_server(int sock_listen)
{
  fd_set fds;
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int max_sock;
  int i;
  addr.sin_family=AF_INET;
    
  exp1_set_fds(&fds, sock_listen);
  max_sock = exp1_get_max_sock();

  if(max_sock < sock_listen){
    max_sock = sock_listen;
  }

  /* 受信可能なクライアントを探すよ。ブロックもしちゃうよ */
  select(max_sock + 1, &fds, NULL, NULL, NULL);
  /* /\*サーバからの送信*\/ */
  /* if (FD_ISSET(0, &fds) != 0 ) { */
  /*   char tmp[MESSAGE_SIZE+8] ={0};//[server]で8文字 */
  /*   fgets(buf, MESSAGE_SIZE, stdin); */
  /*   sprintf(tmp, "[%s]%s","server", buf); */
  /*   exp1_broadcast(tmp,strlen(tmp),sock_listen); */
  /*  } */

  /* selectによりsock_listenソケットディスクリプタのソケットが受信可能だったら */
  if(FD_ISSET(sock_listen, &fds) != 0){
    int sock;
    sock = accept(sock_listen, (struct sockaddr *)&addr, &len);
    printf("accepted connection from %s, port=%d\n",
          inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    write_log(inet_ntoa(addr.sin_addr), ntohs(addr.sin_port),gettimeofday_sec(),"Accept","\n");
    exp1_add(sock,inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
  }

  for(i = 0; i < MAX_CLIENTS; i++){
    if(clients[i].is_accept == 0){
      continue;
    }
    /* selectにより受信可能なソケットのデータがあったら */
    if(FD_ISSET(i, &fds) != 0){
      char buf[MESSAGE_SIZE+NAME_SIZE]={0};
      int ret = recv(i, buf, MESSAGE_SIZE+NAME_SIZE, 0);
      /* 認証済みのとき */
      if(ret > 0 && clients[i].is_auth==1){
        /*DMのとき*/
        if(buf[0] == '@'){
          write_log(clients[i].ip_address,clients[i].port_no,gettimeofday_sec(),"DM",buf);
          write_client_log(gettimeofday_sec(),reg_data[clients[i].reg_data_index].id,buf);
          send_user_limited(buf,i);
        }
        /*ユーザログ*/
        else if(strncmp(buf,"/log",4)==0){
          print_client_log(i,reg_data[clients[i].reg_data_index].id);
        }
        /* チャットのとき */
        else{
          char tmp[MESSAGE_SIZE+NAME_SIZE] ={0};
          sprintf(tmp,"[%s]%s",reg_data[clients[i].reg_data_index].id, buf);
          write_log(clients[i].ip_address,clients[i].port_no,gettimeofday_sec(),"Talk",tmp);
          write_client_log(gettimeofday_sec(),reg_data[clients[i].reg_data_index].id,buf);
          write(STDOUT_FILENO, tmp, sizeof(tmp));
          exp1_broadcast(tmp, sizeof(tmp), i);
        }
        
        /* 未認証のとき */
      }else if(ret > 0 && clients[i].is_auth !=1){
        int allowed =exp1_auth(buf,i);
        /* 認証成功のとき */
        if(allowed == 1){
          char login_ms[NAME_SIZE+13]={0};
          sprintf(login_ms, "( [%s] logged in )\n",reg_data[clients[i].reg_data_index].id);
          char * response ="OK";
          write(i,response,strlen(response));
          write_log(clients[i].ip_address,clients[i].port_no,gettimeofday_sec(),"Login successful","\n");
          sleep(1);//少し待って上げる
          write_dm_before(i);
          write(STDOUT_FILENO,login_ms,strlen(login_ms));
          exp1_broadcast(login_ms,strlen(login_ms),i);
        }
        /* 認証失敗のとき */
        else{
          char *response ="Refused";
          write_log(clients[i].ip_address,clients[i].port_no,gettimeofday_sec(),"Failed to log in","\n");
          write(i,response,strlen(response));
        }
        
        /* 接続エラーのとき */
      }else {
        exp1_remove(i);//エラーが発生したので接続を切るよ
      }
    }
  }
}

/*Called by client's program*/
int exp1_login(int sock){
  char id[NAME_SIZE];
  char password[PASS_SIZE];
  char try_id_pass[NAME_SIZE+PASS_SIZE];
  char buf[LOGIN_RESPONSE_NUM];
  char dm[(MESSAGE_SIZE+NAME_SIZE)*MAX_DM_NUM_BEFORE_LOGINED]={0};
  
  printf("id:");  fgets(id, 128, stdin); 
  printf("password:"); fgets(password, 128, stdin);  lntrim(password);
  sprintf(try_id_pass, "%s%s", id, password);
  write(sock, try_id_pass, strlen(try_id_pass));
  
  int ret = recv(sock, buf, LOGIN_RESPONSE_NUM, 0);
  if(ret > 0 && (strncmp(buf,"OK",2)==0)){
    printf("Login successful!\n\n");
    printf("-----sent dm before logged in-----\n");
    int r = recv(sock,dm,(MESSAGE_SIZE+NAME_SIZE)*MAX_DM_NUM_BEFORE_LOGINED,0);
    if(r>0){
      printf("%s",dm);
    }
    printf("----------------------------------\n");
    return 1;
  }else if(ret > 0 && (strncmp(buf,"Refused",7) == 0)){
    printf("Failed to login\n\n");
    return -1;
  }else{
    return -2;
  }
}

/*認証*/
int exp1_auth( char* login_id_pass,int sock){
  char reged_id_pass[NAME_SIZE+PASS_SIZE+1];
  for(int i =0;i<REGISTERE_NUM;i++){
    sprintf(reged_id_pass, "%s\n%s", reg_data[i].id,reg_data[i].password);
    if(strcmp(login_id_pass,reged_id_pass) == 0 ){
      clients[sock].is_auth=1;
      clients[sock].reg_data_index = i;
      return 1;
    }
  }
  return -1;
}

/*direct message*/
void  send_user_limited(char* buf,int sock){
  char id[NAME_SIZE]={0};
  char message[MESSAGE_SIZE]={0};
  int i,j=0;
  char send_m[MESSAGE_SIZE+NAME_SIZE] ={0};
  char result_m[256]={0};

  /*idとmessageを切り離し*/
  for(i=0; i<NAME_SIZE+MESSAGE_SIZE; i++){
    if(buf[i +1]==' '){
      id[i]='\0';
      break;
    }
    if(buf[i +1]=='\0'){
      id[i]='\0';
      strcpy(result_m,"Message is empty\n");
      write(sock,result_m,sizeof(result_m));
      return;
    }
    id[i] = buf[i+1];
  }
  for( i=i+2 ;i<MESSAGE_SIZE+NAME_SIZE;i++){
    if(buf[i]=='\0'){
      message[j++]='\0';
      break;
    }
    message[j++] = buf[i];
  }

  /*ログイン者か？*/
  for(i =0; i<MAX_CLIENTS; i++){
    if(clients[i].is_auth==1 && strcmp(id,reg_data[clients[i].reg_data_index].id) == 0){
       sprintf(result_m,"(Sending DM to [%s] successful)\n",id);
      sprintf(send_m, "[%s %s]%s","DM from",id, message);
      write(i,send_m,sizeof(send_m));
      write(sock,result_m,sizeof(result_m));
      return;
    }
  }

  /*未ログイン者か？*/
  for(i=0; i<REGISTERE_NUM;i++){
    if(strcmp(id,reg_data[i].id)==0){
      sprintf(result_m, "[%s] is not logged in. Sending DM when he/she logs in. )\n",id);
      write(sock,result_m,sizeof(result_m));
      
      int  dm_num = reg_data[i].dm_num;
      if(dm_num != MAX_DM_NUM_BEFORE_LOGINED){
        char tmp[MESSAGE_SIZE+NAME_SIZE+2] = {0};
        sprintf(tmp,"[DM from %s]%s",reg_data[clients[sock].reg_data_index].id,message);
        strcpy(reg_data[i].dm_before_logined[dm_num],tmp);
        reg_data[i].dm_num = dm_num+1;
      }
      return;
    }
  }
  write(sock,"User not found\n",16);
  return;
}

/*未ログイン者へのdmを登録*/
void write_dm_before(int sock){
  char all_dm[(MESSAGE_SIZE+NAME_SIZE)*MAX_DM_NUM_BEFORE_LOGINED]={0};
  
  for(int i=0;i<reg_data[clients[sock].reg_data_index].dm_num; i++){
    strcat(all_dm,reg_data[clients[sock].reg_data_index].dm_before_logined[i]);
  }
  write(sock,all_dm,sizeof(all_dm));
  return;
}

void write_log(char* ip, int port,double time,char * action,char* content){
  FILE* fp;
  fp = fopen("../log/server.txt", "a");
  fprintf(fp,"(%s ip=%s  port=%d  UTC time=%lf) %s",action,ip,port,time,content);
  fclose(fp);
}

/*log->.txt*/
void write_client_log(double time,char* id, char* content){
  char path[NAME_SIZE+11]={0};
  FILE* fp;
  strcat(path,"../log/");
  strcat(path,id);
  strcat(path,".txt");
  fp=fopen(path,"a");
  fprintf(fp,"(UTC time=%lf) %s",time,content);
  fclose(fp);
}

/*クライアントのログを出力*/
void print_client_log(int sock,char* id){
  char path[NAME_SIZE+11]={0};
  FILE* fp;
   char readline[MESSAGE_SIZE+NAME_SIZE] = {0};
   
  strcat(path,"../log/");
  strcat(path,id);
  strcat(path,".txt");
  fp=fopen(path,"r");
 while ( fgets(readline, MESSAGE_SIZE+NAME_SIZE, fp) != NULL ) {
        write(sock,readline,strlen(readline));
        sleep(0.5);
    }
  fclose(fp);
}

/* 最期に入る改行を取り除く関数 */
void lntrim(char *str) {  
  char *p;  
  p = strchr(str, '\n');  
  if(p != NULL) {  
    *p = '\0';  
  }  
}
