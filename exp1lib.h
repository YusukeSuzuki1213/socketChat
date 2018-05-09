#include <sys/select.h>

/**
 * prototype 
 */
int exp1_tcp_listen(int port);
int exp1_tcp_connect(const char *hostname, int port);
double gettimeofday_sec();
int exp1_do_talk(int sock);
void exp1_init_clients();
void exp1_set_fds(fd_set* pfds, int accept_sock);
void exp1_add(int sock,char* ip, int port);
void exp1_remove(int id);
int exp1_get_max_sock();
void exp1_broadcast(char* buf, int size, int from);
void exp1_do_server(int sock_listen);
int exp1_login(int sock);
int exp1_auth(char* login_info,int sock);
void lntrim(char *str);
void send_user_limited(char* buf,int sock);
char* get_client_id(int sock);
int get_index_from_sock_no(int sock_no);
void write_dm_before(int sock);
void write_log(char* ip, int port,double time,char * action,char* content);
void write_client_log(double time,char* id, char* content);
void print_client_log(int sock,char* id);
