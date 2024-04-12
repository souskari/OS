#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>

#define MAX_CLIENTS 500
#define MAX_MSG_SIZE 4096
#define MSG_FILE "msg.txt"

int server_sockfd[50]={0};
//long acceptable_msg_table[MAX_CLIENTS][MAX_CLIENTS]={-1};
int STOP=0;
int start_port;
int end_port;
int MAX_SIZE=500;
FILE* file;

int set_non_block_mode(int s)
{
	int fl = fcntl(s, F_GETFL, 0);
	return fcntl(s, F_SETFL, fl | O_NONBLOCK);
}

int sock_err(const char* function, int s)
{
	int err;
	err = errno;
	fprintf(stderr, "%s: socket error: %d\n", function, err);
	return -1;
}

void s_close(int s)
{
	close(s);
}

void send_to(int client_ind, long acceptable_msg_table[500][500], sockaddr_in* addr) {
	int datagram_size=0;
	char datagram[1000];
	char num_msg_str[5] = { 0 };
	int num;
	
	for (int j = 0; j < MAX_CLIENTS; j++) {
		if (acceptable_msg_table[client_ind][j] != -1) {
			num=htonl(acceptable_msg_table[client_ind][j]);
			//printf("num: %d\n", ntohl(num));
			memcpy(num_msg_str, &num, 4);
			for (int i = 0; i < 4; i++) {
				datagram[datagram_size + i] = num_msg_str[i]; 
			}
			datagram_size += 4;
		}
	}
	//printf(datagram);

	int res=sendto(server_sockfd[client_ind], datagram, datagram_size, 0, (struct sockaddr*)&addr[client_ind], sizeof(struct sockaddr_in));
	printf("sendto: %d bytes = %d\n", res, datagram_size);
}

// Функция для обработки сообщения от клиента
void recv_msg(struct sockaddr_in* addr, char* buf, int ind, long acceptable_msg_table[500][500]) {
    uint32_t msg_id, date, time1, time2, msg_len;
    memcpy(&msg_id, buf, 4);
    memcpy(&date, buf + 4, 4);
    memcpy(&time1, buf + 8, 4);
    memcpy(&time2, buf + 12, 4);
    memcpy(&msg_len, buf + 16, 4);
	
	msg_id=ntohl(msg_id);
	date=ntohl(date);
	msg_id=ntohl(msg_id);
	time1=ntohl(time1);
	time2=ntohl(time2);
	msg_len=ntohl(msg_len);
	
	unsigned int num;
	char* p = (char*)&num;
	for (int j = 0; j < 4; j++, ++p)
		*p = buf[j];
	num = ntohl(num);
	int message_id=num;
	
	//printf("Now we have #%d message\n",message_id);
	// Проверка на повтор
	printf("Message_id :%d\n",message_id);
	for(int i=0;i<25;i++){
		//printf("Acceptable array: %d ",acceptable_msg_table[ind][i]);
		if (acceptable_msg_table[ind][i]==message_id){
			printf("duplicate message #%d\n",message_id);
			return;
		}
		else if (acceptable_msg_table[ind][i]==-1) {
			acceptable_msg_table[ind][i] = message_id;
			break;
			//printf("Now we have #%d message\n",acceptable_msg_table[ind][i]);
		}
	}
	//для айпишки в файл
	struct sockaddr_in tmp_addr;
	socklen_t addr_len = sizeof(tmp_addr);
	getsockname(server_sockfd[ind], (struct sockaddr*)&tmp_addr, &addr_len);
	printf("indx: %d\n", ind);
	int port = ntohs(tmp_addr.sin_port);
	//printf("addr[ind]: %d\n",addr[ind]);
    int ip=ntohl(addr[ind].sin_addr.s_addr);
    
	printf("%d.%d.%d.%d:%d %02d.%02d.%04d %02d:%02d:%02d %02d:%02d:%02d %s\n",(ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF,port, date % 100, (date % 10000) / 100, date / 10000,time1 / 10000, (time1 % 10000) / 100, time1 % 100,time2 / 10000, (time2 % 10000) / 100, time2 % 100, buf + 20);
    // Запись сообщения в файл
    if (file) {
        fprintf(file, "%d.%d.%d.%d:%d %02d.%02d.%04d %02d:%02d:%02d %02d:%02d:%02d %s\n",(ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF,port, date % 100, (date % 10000) / 100, date / 10000,time1 / 10000, (time1 % 10000) / 100, time1 % 100,time2 / 10000, (time2 % 10000) / 100, time2 % 100, buf + 20);
        
    }
}

int main(int argc, char* argv[]) {
	long acceptable_msg_table[MAX_CLIENTS][MAX_CLIENTS]={-1};
	struct sockaddr_in addr[10000];
	//printf("%u\n", ntohl(13015052));
	start_port = atoi(argv[1]);
	end_port = atoi(argv[2]);

	// Инициализация сокетов
	int n_clients = 0;
	for (int port = 0; port < end_port - start_port + 1; port++) {
		server_sockfd[port] = socket(AF_INET, SOCK_DGRAM, 0);
		//printf("socket: %d\n", server_sockfd[port - start_port]);
		if (server_sockfd[port] == -1) {
			perror("socket");
			return 1;
		}
		set_non_block_mode(server_sockfd[port]);
		
		memset(&addr, 0, sizeof(addr));
		addr[port].sin_family = AF_INET;
		addr[port].sin_port = htons(start_port+port);
		addr[port].sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(server_sockfd[port], (struct sockaddr*)&addr[port], sizeof(addr[port])) == -1) {
			perror("bind");
			return 1;
		}
		
	}

	fd_set rfd;
	fd_set wfd;
    int nfds = server_sockfd[0];  
	//acceptable_msg_table=(int**)malloc(MAX_CLIENTS*sizeof(int*));
	for (int i = 0; i < MAX_CLIENTS; i++) {
		for (int j = 0; j < MAX_CLIENTS; j++){
			acceptable_msg_table[i][j]=-1;
		}
	} 
	file = fopen(MSG_FILE, "w+");
    while (!STOP) {
    	
		FD_ZERO(&rfd); 
		FD_ZERO(&wfd);
        // Добавление сокетов в множество для select
        for (int i = 0; i < end_port - start_port + 1; i++) {
        	FD_SET(server_sockfd[i], &wfd);
            FD_SET(server_sockfd[i], &rfd);
         
            if (server_sockfd[i] > nfds) {
                nfds = server_sockfd[i];
                
            }
        }
		struct timeval tv = {0, 1000};
		//printf("nfds: %d\n", nfds);
		int ret = select(nfds+1, &rfd, &wfd, 0, &tv);
		

		if (ret == -1) {
			perror("select");
			continue;
		}
		else if (ret == 0) {
			perror("select_time_out");
			continue;
		}
		//struct Client tmp_client;
		// Обработка событий
		for (int i = 0; i < end_port - start_port+1; i++) {
				if (FD_ISSET(server_sockfd[i], &rfd)) {
					//printf("IS SET\n");
					//char buf[MAX_MSG_SIZE]={0};
					char buf[MAX_MSG_SIZE]={0};
				
					//struct sockaddr_in from;
					socklen_t len_s=sizeof(addr[i]);
					//memset(&buf,0,MAX_MSG_SIZE);
					int recv_len = recvfrom(server_sockfd[i], buf, MAX_MSG_SIZE, 0,(struct sockaddr*)&addr[i], &len_s );

					if (recv_len == -1) {
						perror("recvfrom");
						continue;
					}
					else{
						printf("recv %d bytes\n", recv_len);
						//printf(buf);
					}
					
					recv_msg(addr,buf,i, acceptable_msg_table);
				
					if (strncmp(&buf[20], "stop", 4) == 0) {
						send_to(i,acceptable_msg_table,addr);
						printf("STOP\n");
						STOP=1;
						//usleep(1000000000);
						for(int h=0;h<end_port - start_port + 1;h++){
							s_close(server_sockfd[h]);
						}
						fclose(file);
						return 1;
					}
					else{
						send_to(i,acceptable_msg_table,addr);
					}
				}
				else{
					for (int k = 0; k < MAX_CLIENTS; k++){
								acceptable_msg_table[i][k] = -1;
								
					} 
				}
					//usleep(10000);
					
				
			
		}
	}
for(int h=0;h<end_port - start_port + 1;h++){
	s_close(server_sockfd[h]);
}
fclose(file);
return 0;
}
                               
