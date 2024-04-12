#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define MAX_RECV_SIZE 20 * sizeof(int)
#define MAX_MSG_SIZE 35000
#include <WS2tcpip.h>
#include <errno.h> 
#include <windows.h>
#include <winsock2.h>
// ��������� ����������: ������������ ���������� �������
#pragma comment(lib, "ws2_32.lib")
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

int max_nums = 20;
int lines_cnt = 0;
int success_lines_cnt = 0;
int sent_count = 0;
int sent_messages[MAX_MSG_SIZE]; 
int last_recived, num_received = 0;

struct Message {
	uint32_t msg_id;
	uint32_t date;
	uint32_t time1;
	uint32_t time2;
	uint32_t text_len;
	char text[35000 - 20];
};

struct Message* file_lines = (struct Message*)malloc(sizeof(struct Message)*MAX_MSG_SIZE);
int init()
{
	// ��� Windows ������� ������� WSAStartup ����� ������� ������������� �������
	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));

}
void deinit()
{
	// ��� Windows ������� ������� WSACleanup � ����� ������
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

unsigned int time_convert(char* time) {
	char* token_time = strtok(time, ":");
	unsigned int hour = atoi(token_time) * 10000;

	token_time = strtok(NULL, ":");
	unsigned int minute = atoi(token_time) * 100;

	token_time = strtok(NULL, ":");
	unsigned int seconds = atoi(token_time);
	return hour + minute + seconds;
}

unsigned int date_convert(char* date) {
	char* token_date = strtok(date, ".");
	unsigned int day = atoi(token_date);

	token_date = strtok(NULL, ".");
	unsigned int month = atoi(token_date) * 100;

	token_date = strtok(NULL, ".");
	unsigned int year = atoi(token_date) * 10000;
	return day + month + year;
}


void send_message(int s, struct Message* msg,struct sockaddr_in* addr)
{

	if (sendto(s, (const char*)msg, 20+ sizeof(msg->text), 0, (struct sockaddr*)addr, sizeof(struct sockaddr_in))>0) {
		printf("sendto complete for message #%d\n", ntohl(msg->msg_id));
	}
	else {
		printf("ERROR: Sending error.\n");
		sock_err("sendto", s);
	}
//	char str[20]; // ������������, ��� ����� �� ����� �������� ������ 20 ��������
//	sprintf(str, "%d", msg->msg_id);
//	int res=sendto(s, (char*)&msg, sizeof(struct Message), 0, (struct sockaddr*)addr, sizeof(struct sockaddr_in*));
}
void get_lines(FILE* file) {
	int msg_num = 0;
	char line[34100];
	while (fgets(line, sizeof(line), file)) {
		// ������ ������ ���������
		if (line[0] == '\n') continue;
		char date[12], time1[12], time2[12], message[34100] = { 0 };
		sscanf(line, "%[^ ] %[^ ] %[^ ] %[^\n]", date, time1, time2, message);
		msg_num++;

		//������� ������ � ��������� 
		char line[MAX_MSG_SIZE];
		struct Message msg;

		unsigned int len = strlen(message);
		unsigned int date_int = date_convert(date);
		unsigned int time1_int = time_convert(time1);
		unsigned int time2_int = time_convert(time2);

		msg.msg_id = htonl(msg_num);
		msg.date = htonl(date_int);
		msg.time1 = htonl(time1_int);
		msg.time2 = htonl(time2_int);
		msg.text_len = htonl(len);
		strcpy(msg.text, message);
		file_lines[lines_cnt] = msg;
		lines_cnt++;
	}
}
// ������� ��������� ����������� �� ��������� �������.
// ���������� 0, ���� � ������� 100 ����������� �� ���� �������� �� ����� �����������
unsigned int recv_response(int s)
{
	char datagram[1024];
	struct timeval tv = { 0, 100 * 1000 }; // 100 msec
	int res;
	fd_set fds;
	FD_ZERO(&fds); FD_SET(s, &fds);
	// �������� - ���� � ������ �������� �����������
	// (�������� � ������� tv)
	int received;
	res = select(s + 1, &fds, 0, 0, &tv);
	printf("res: %d\n", res);
	if (res > 0)
	{
		// ������ ����, ���������� ��
		struct sockaddr_in addr;
		int addrlen = sizeof(addr);
		int recv_ids[20];
		memset(recv_ids, 0, 20*sizeof(int));
		int received = recvfrom(s, (char*)recv_ids, sizeof(recv_ids)*lines_cnt, 0, (struct sockaddr*)&addr, &addrlen);
		printf("recv good\n");
		if (received <= 0)
		{
			// ������ ���������� ���������� �����������
			sock_err("recvfrom", s);
			//return 0;
		}
		else {
			last_recived = num_received;
			num_received = received / sizeof(int);
			if (num_received!=last_recived) {
				sent_count++;
			}
			for (int i = 0; i < num_received; i++) {
				printf("index of success: %u\n", ntohl(recv_ids[i]));
				printf("Semt message counter = %d\n", sent_count);
				sent_messages[ntohl(recv_ids[i])] = 1;
			}
		}
	}
	else
	{
		sock_err("select", s);
		//return 0;
	}
	return 1;
}
unsigned int get_host_ipn(const char* name)
{
	struct addrinfo* addr = 0;
	unsigned int ip4addr = 0;
	// ������� ���������� ��� ������ ���������� �����
	// � ���� ������������� ����������������� ������
	if (0 == getaddrinfo(name, 0, 0, &addr))
	{
		struct addrinfo* cur = addr;
		while (cur)
		{
			// ���������� ������ IPv4 �����, ���� �� ��������� - �� ������
			if (cur->ai_family == AF_INET)
			{
				ip4addr = ((struct sockaddr_in*)cur->ai_addr)->sin_addr.s_addr;
				break;
			}
			cur = cur->ai_next;
		}
		freeaddrinfo(addr);
	}
	return ip4addr;
}

int main(int argc, char* argv[])
{
	int s;
	struct sockaddr_in addr;
	// �������������� ������� ����������
	init();

	// �������� UDP-������
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		return sock_err("socket", s);

	char* server_ip = strtok(argv[1], ":");
	char* server_port = strtok(NULL, ":");
	int port = atoi(server_port);
	char* filename = argv[2];

	// ���������� ��������� � ������� ���������� ����
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port); // ���� DNS - 53
	addr.sin_addr.s_addr = inet_addr(argv[1]);

	// �������� ����� ���������
	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		printf("Error opening file: %s\n", filename);
		return 1;
	}

	// ������ ��� �������� ������� ��� ������������ ��������� (����. 20)
	int received_nums[20];
	memset(received_nums, -1, sizeof(received_nums));
	int num_received = 0; // ���������� ���������� �������������

	// ������ � �������� ��������� �� �����
	get_lines(file);
	int i = 0;
	memset(sent_messages, 0, sizeof(sent_messages)); // ������������� ������
	while (1) {
		while (i < lines_cnt) {
			if (!sent_messages[i + 1]) {
				// 8. �������� ��������� �� ������
				send_message(s, &file_lines[i], &addr);
				//sent_messages[i+1] = 0; // �������� ��������� ��� ������������
			}
			i++;
		}

		int recv_resp=recv_response(s);

		for (int i = 1; i <= lines_cnt; i++) {
			if (sent_messages[i] != 1) {
				send_message(s, &file_lines[i-1], &addr);
			}
		}

		// 12. �����, ���� ���������� 20 ���������
		if (sent_count >= 20 || sent_count== lines_cnt) {
			printf("All messages successfully sent\n");
			break;
		}
	}
	// �������� ������
	s_close(s);
	deinit();
	//Sleep(10000000000);
	return 0;
	
}