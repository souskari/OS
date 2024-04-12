#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MASSAGE_LEN 341000

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
// Функция определяет IP-адрес узла по его имени.
// Адрес возвращается в сетевом порядке байтов.
unsigned int get_host_ipn(const char* name)
{
	struct addrinfo* addr = 0;
	unsigned int ip4addr = 0;
	// Функция возвращает все адреса указанного хоста
	// в виде динамического однонаправленного списка
	if (0 == getaddrinfo(name, 0, 0, &addr))
	{
		struct addrinfo* cur = addr;
		while (cur)
		{
			// Интересует только IPv4 адрес, если их несколько - то первый
			if (cur->ai_family == AF_INET)
			{
				ip4addr = ((struct sockaddr_in*) cur->ai_addr)->sin_addr.s_addr;
				break;
			}
			cur = cur->ai_next;
		}
		freeaddrinfo(addr);
	}
	return ip4addr;
}
unsigned int time_convert(char* time){
	char *token_time = strtok(time, ":");
	int hour = atoi(token_time)*10000;
		    
	token_time = strtok(NULL, ":");
	int minute = atoi(token_time)*100;
		    
	token_time = strtok(NULL, ":");
	int seconds = atoi(token_time);
	return hour+minute+seconds;
}

unsigned int date_convert(char* date){
	char *token_date = strtok(date, ".");
	int day = atoi(token_date);
		    
	token_date = strtok(NULL, ".");
	int month = atoi(token_date)*100;
		    
	token_date = strtok(NULL, ".");
	int year = atoi(token_date)*10000;
	return day+month+year;
}

// Отправляет http-запрос на удаленный сервер
int send_request(int s, char* file_name)
{
	
}


int main(int argc, char* argv[])
{
	int s;
	struct sockaddr_in addr;
	//char consol_str[MAX_MASSAGE_LEN];
	char ip_adress[15];
	char file_name[50];
	char port_str[5];
	
	if(argc<2){
		printf("Less arguments!\n");
		return 0;
	}
	
	char* token = strtok(argv[1], ":");
    	strncpy(ip_adress, token, 15);

    	token = strtok(NULL, ":");
    	strncpy(port_str, token, 5);
    	int port=atoi(port_str);
	//printf("%d\n",port);
	//printf("пока все ок \n");
	s = socket(AF_INET, SOCK_STREAM, 0);
 
	if (s < 0) return sock_err("socket", s);
	 // Заполнение структуры с адресом удаленного узла
	//printf("пока все ок \n");
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = get_host_ipn(argv[1]);
	
	
	int try_cnt=10;
	int success_flag=1;
	
	//printf("заход в цикл вайл\n");
	while(try_cnt){
		//printf("сейчас будет коннект\n");
		if (connect(s, (struct sockaddr*) &addr, sizeof(addr)) == 0) break;
		printf("we keep trying to connecting\n");
		try_cnt--;
		usleep(100000);
	}
	//printf("выход из цикла\n");
    	
   	//////////////////////////////////////////////////
   	
	if(try_cnt<=0){
		success_flag=0;
		s_close(s);
	 	return sock_err("connect", s);
	}
	if(success_flag){
		printf("success connection\n");
		try_cnt = send(s, "put", 3 * sizeof(char), 0);
		printf("put was sended\n");
	}
	// ВМЕСТО СЕНД РЕКВЕСТА //
	/* ТЕСТИРОВАНИЕ РАБОТЫ ПАРСИНГА СТРОК */
	FILE *file = fopen(argv[2], "r");
	char line[MAX_MASSAGE_LEN]={0};
	unsigned int msg_id,msg_cnt=0;
	
	if (file==NULL){
		printf("Error with file oppening\n");
		return 0;
	}
	int reply=0;
	int result;
	char ok[3]={0};
	while (fgets(line, MAX_MASSAGE_LEN, file)) {
   	 // Парсинг строки
   	 	if(line[0]=='\n') continue;
   	 	msg_id++;
   	 	msg_cnt++;
    		char *token = strtok(line, " "); // разделение строки по пробелам
		char date_str[11], time1_str[9], time2_str[9], message[MAX_MASSAGE_LEN]={0};
		
		// Чтение и сохранение отдельных частей строки
		if (token != NULL) strcpy(date_str, token);
		
		token = strtok(NULL, " ");
		if (token != NULL) strcpy(time1_str, token);
		
		token = strtok(NULL, " ");
		if (token != NULL) strcpy(time2_str, token);
		
		token = strtok(NULL, "\n");
		if (token != NULL) strcpy(message, token);
		
		// запись финальных значений
		unsigned int htonl_len = htonl(strlen(message));
		unsigned int htonl_date = htonl(date_convert(date_str));
		unsigned int htonl_time1 = htonl(time_convert(time1_str));
		unsigned int htonl_time2 = htonl(time_convert(time2_str));
		msg_id=htonl(msg_id);
		
		// отправка сообщений //
		//printf("до отправки\n");
		result = send(s, (const void *)&msg_id, 4, 0); 
		result =send(s, (const void *)&htonl_date, 4, 0);
		result =send(s, (const void *)&htonl_time1, 4, 0);
		result =send(s, (const void *)&htonl_time2, 4, 0);
		result =send(s, (const void *)&htonl_len, 4, 0);
		result =send(s, message, strlen(message), 0);
		//printf("отправка сообщений завершена\n");
		result = recv(s, ok, 2, 0);
		//printf("ОК ещё не пришел, но реквест был\n");
		if (ok[0] == 'o' && ok[1] == 'k') {
			//printf("ОК пришел\n");
			reply++;
		}
		//printf("\n%d\n", strlen(message));
    	}
    	int ok_try_cnt=10;
    	printf("message cnt: %d ; reply cnt: %d\n", msg_cnt,reply);
    	if(msg_cnt==reply) {
    		//printf("\nвсе успешно завершилось\n");
    		s_close(s);
    	}
	else {
		while(msg_cnt!=reply and ok_try_cnt){
			result = recv(s, ok, 2, 0);
			//printf("ОК ещё не пришел, но реквест был\n");
			if (ok[0] == 'o' && ok[1] == 'k') {
				//printf("ОК пришел\n");
				reply++;
			}
			if (ok[0] == 'o' && ok[1]!='k'){
				usleep(400000);
				result = recv(s, ok, 2, 0);
				if (ok[0] == 'k' || ok[1] == 'k'){
					reply++;
				}
			}
			ok_try_cnt--;
			usleep(400000);
		}
	}
    	///////////////////////////
	
	return 1;
	
}
