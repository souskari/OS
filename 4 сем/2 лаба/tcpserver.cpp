#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define SOCKET_AMOUNT 1000
#define MAX_LEN 350000
#include <windows.h>
#include <winsock2.h>
// Директива линковщику: использовать библиотеку сокетов
#pragma comment(lib, "ws2_32.lib")
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
int stop = 0;
int soc_cl_cnt = 0;
int put_flag = 0;
int byte_mode = 0;//1, если по 1 байту
FILE* message_file;
void delete_and_shift_array(int* array, int index) {
	// Проверка корректности индекса
	if (index < 0 || index >= soc_cl_cnt) {
		return;
	}

	// Сдвиг элементов
	for (int i = index; i < soc_cl_cnt - 1; i++) {
		array[i] = array[i + 1];
	}

	// Уменьшение размера массива
	soc_cl_cnt--;
}
int get_client_data(int socket, int port, int ip) {
	char num_msg[4], date[10] = { 0 }, time_1[10] = { 0 }, time_2[10] = { 0 };
	unsigned int t1, t2, d, day, month, year, h1, h2, m1, m2, s1, s2;
	int j = 0;
	char num_char;
	int res;
	int res_num;
	while (j != 4) {
		Sleep(15);
		res_num = recv(socket, &num_char, 1, 0);
		Sleep(15);
		num_msg[j] = num_char;
		j++;
	}
	j = 0;
	//printf("num recvest\n");
	char data_char = '\0';
	while (j != 4) {
		res = recv(socket, &data_char, 1, 0);
		Sleep(15);
		date[j] = data_char;
		j++;
	}
	j = 0;
	//res = recv(socket, date, 4, 0);
	//printf("date recvest\n");
	d = ntohl(*(unsigned int*)date);
	
	day = d % 100;
	month = (d % 10000) / 100;
	year = d / 10000;
	printf("\date: %02d.%02d.%04d ", day,month, year);
	char time1_char = '\0';
	while (j != 4) {
		res = recv(socket, &time1_char, 1, 0);
		Sleep(15);
		time_1[j] = time1_char;
		j++;
	}
	j = 0;
	//res = recv(socket, time_1, 4, 0);
	//printf("time1 recvest\n");
	t1 = ntohl(*(unsigned int*)time_1);
	
	h1 = t1 / 10000;
	m1 = (t1 / 100) % 100;
	s1 = t1 % 100;
	printf("%02d:%02d:%02d ", h1, m1, s1);
	//printf("\ntime 1: %u\n", t1);
	//res = recv(socket, time_2, 4, 0);
	char time2_char = '\0';
	while (j != 4) {
		res = recv(socket, &time2_char, 1, 0);
		Sleep(15);
		time_2[j] = time2_char;
		j++;
	}
	j = 0;
	//printf("time2 recvest\n");

	t2 = ntohl(*(unsigned int*)time_2);

	h2 = t2 / 10000;
	m2 = (t2 / 100) % 100;
	s2 = t2 % 100;
	printf("%02d:%02d:%02d \n", h2, m2, s2);
	char n[4];
	//res = recv(socket, n, 4, 0);
	char n_char = '\0';
	while (j != 4) {
		res = recv(socket, &n_char, 1, 0);
		Sleep(15);
		n[j] = n_char;
		j++;
	}
	j = 0;
	//printf("len bil\n");
	//printf("len recvest\n");
	char msg[MAX_LEN]={0};
	int msg_len = ntohl(*(unsigned int*)n);
	int i = 0;
	for (i; i < 350000; i++) {

		recv(socket, &msg[i], 1, 0);
		//printf("message[i]: %s\n", &msg[i]);
		Sleep(15);
		if (msg[i] == 0)
			break;
		
	}
	if (i  != msg_len) {
		printf("ne sovpalo\n");
		printf("i:%d!=len:%d\n",i,msg_len);
		return 1;
	}
	printf("message: %s\n", msg);
	//printf("%u %u.%u.%u %u:%u:%u %u:%u:%u %s\n",  port, day, month, year, h1, m1, s1, h2, m2, s2, msg);
	//Sleep(1000000000000);
	if (strncmp("stop", msg, 4) == 0 && strlen(msg) == 4)
	{
		printf("STOP\n");
		stop = 1;
		put_flag = 0;

	}
	fprintf(message_file, "%d.%d.%d.%d:%u %02u.%02u.%04u %02u:%02u:%02u %02u:%02u:%02u %s\n", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, port, day, month, \
		year, h1, m1, s1, h2, m2, s2, msg);
	
	return 1;
}
int set_non_block_mode(int s)
{
	unsigned long mode = 1;
	return ioctlsocket(s, FIONBIO, &mode);
}
int init()
{
	// Для Windows следует вызвать WSAStartup перед началом использования сокетов
	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));

}
void deinit()
{
	// Для Windows следует вызвать WSACleanup в конце работы
	WSACleanup();
}
int sock_err(const char* function, int s)
{
	int err;
	err = WSAGetLastError();

	fprintf(stderr, "%s: socket error: %d\n", function, err);
	return -1;
}
void s_close(int s)
{
	closesocket(s);

}


int main(int argc, char* argv[])
{
	int s;
	struct sockaddr_in addr;
	FILE* std_out = fopen("stdout.txt", "w+");
	// Инициалиазация сетевой библиотеки
	init();
	// Создание TCP-сокета
	int ip = 0;
	s = socket(AF_INET, SOCK_STREAM, 0);
	set_non_block_mode(s);
	printf("socket created\n");
	if (s < 0) return sock_err("socket", s);
	// Заполнение адреса прослушивания

	int port = atoi(argv[1]);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port); // Сервер прослушивает порт 9000
	addr.sin_addr.s_addr = htonl(INADDR_ANY); // Все адреса

	// Связывание сокета и адреса прослушивания
	if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		return sock_err("bind", s);

	// Начало прослушивания
	if (listen(s, 1) < 0)
		return sock_err("listen", s);

	WSAEVENT events[2];
	events[0] = WSACreateEvent();
	events[1] = WSACreateEvent();

	// WSAEventSelect для прослушивающего сокета
	WSAEventSelect(s, events[0], FD_ACCEPT);
	int* socet_client=(int*)calloc(SOCKET_AMOUNT, sizeof(int));


	// WSAEventSelect для клиентских сокетов
	for (int i = 0; i < SOCKET_AMOUNT; i++) {
		WSAEventSelect(socet_client[i], events[1], FD_READ | FD_WRITE | FD_CLOSE);
	}
	int rcv;
	char put[4] = { 0 };
	//char msg[MAX_LEN] = { 0 };
	message_file = fopen("msg.txt", "w+");
	while (!stop) {
		WSANETWORKEVENTS ne;
		DWORD dwWaitResult = WSAWaitForMultipleEvents(2, events, FALSE, 1000, FALSE);
		//printf("EXIT FTOM MULRIPLE\n");
		// Сброс событий
		WSAResetEvent(events[0]);
		WSAResetEvent(events[1]);

		// Обработка события на прослушивающем сокете
		if (dwWaitResult == WSA_WAIT_FAILED) {
			fprintf(std_out,"WSAWaitForMultipleEvents failed\n");
			break;
		}
		else if (0 == WSAEnumNetworkEvents(s, events[0], &ne) && (ne.lNetworkEvents & FD_ACCEPT)) {
			sockaddr_in new_s_addr;
			int new_s_addr_len = sizeof(new_s_addr);
			int len_s = sizeof(addr);
			printf("wait for accept\n");
			int new_s = accept(s, (struct sockaddr*)&addr, &len_s);
			if (new_s < 0) {
				fprintf(std_out, "ERROR with accept\n");
				return sock_err("accept", s);
				break;
			}
			set_non_block_mode(new_s);
			ip = ntohl(addr.sin_addr.s_addr);
			printf("Client #%d connected: %d.%d.%d.%d:%i\n", soc_cl_cnt + 1,(ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, (ip) & 0xFF, port);

			// Добавление нового сокета в массив
			socet_client[soc_cl_cnt] = new_s;
			//soc_cl_cnt++;
			WSAEventSelect(socet_client[soc_cl_cnt], events[1], FD_READ | FD_WRITE | FD_CLOSE);
			soc_cl_cnt++;
		}
		for (int i = 0; i < soc_cl_cnt; i++) {
			if (0 == WSAEnumNetworkEvents(socet_client[i], events[1], &ne)) {
				// Обработка событий для сокета клиента
				if (ne.lNetworkEvents & FD_READ) {
					// Есть данные для чтения, можно вызывать recv/recvfrom на socet_client[i]
					//printf("WE IN READ\n");
					if (put_flag == 0) {
						char put_char='\0';
						rcv = recv(socet_client[i], put, 3, 0);
						Sleep(15);
						printf("Put recvest\n");
						printf("Put arrey: %s\n", put);
							//Sleep(500);
						put[3] = '\0';
						/* Проверка на put */
						if (rcv < 0) s_close(socet_client[i]);
						if (strcmp("put", put) == 0 || put[0]=='t') {
							if (put[0] == 't') byte_mode = 1;
							printf("Put comlete\n");
							put_flag = 1;
							//send(socet_client[i], "ok", 2, 0);//ok

						}
						//continue;
					}
					else {

						int try_recv = get_client_data(socet_client[i], port, ip);
						if (stop == 1) {
							put_flag = 0;
							send(socet_client[i], "ok", 2, 0);//ok
							s_close(socet_client[i]);
							s_close(s);
							delete_and_shift_array(socet_client, i);
							break;
						}
						else {
							printf("Sending OK \n");
							send(socet_client[i], "ok", 2, 0);
						}
					}
					
				}
				if (ne.lNetworkEvents & FD_CLOSE) {
					//printf("WE IN CLOSE\n");
					// Удаленная сторона закрыла соединение
					put_flag = 0;
					stop = 0;
					s_close(socet_client[i]);
					delete_and_shift_array(socet_client, i);
					//break;
				}
				
			}
		}
	}
	
	fclose(message_file);
	//Sleep(10000000);
	s_close(s);
	deinit();
	return 0;
}
