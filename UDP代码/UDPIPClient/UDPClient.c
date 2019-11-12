#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>

#pragma comment(lib,"ws2_32.lib")

int main(void)
{
	WSADATA wsaData;
	int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRes != 0)
	{
		printf("ÍøÂç¿â´ò¿ªÊ§°Ü£¬´íÎóÂë£º%d\n", WSAGetLastError());
		system("pause");
		return 0;
	}
	if (2!=HIBYTE(wsaData.wVersion) || 2!=LOBYTE(wsaData.wVersion))
	{
		//´ò¿ªÍøÂç¿âÊ§°Ü
		printf("°æ±¾Ð£ÑéÊ§°Ü\n");
		WSACleanup();
		system("pause");
		return 0;
	}
	SOCKET clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clientSocket == INVALID_SOCKET)
	{
		printf("´´½¨socketÊ§°Ü£¬´íÎóÂë£º%d\n", WSAGetLastError());
		WSACleanup();
		system("pause");
		return 0;
	}
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("192.168.137.110");
	serverAddr.sin_port = htons(12345);

	while (1)
	{

		char strbuf[548] = { 0 };
		//gets(strbuf);
		scanf("%s", strbuf);
		/*if (strbuf[0] == '0')
		{
			continue;
		}*/
		int num = sendto(clientSocket, strbuf, 547, 0, (const struct sockaddr*)&serverAddr, sizeof(serverAddr));
		if (num == SOCKET_ERROR)
		{
			printf("sendto  error :%d\n", WSAGetLastError());
		}
		int nLen = sizeof(serverAddr);
		num = recvfrom(clientSocket, strbuf, 547, 0, (struct sockaddr*)&serverAddr, &nLen);
		if (num == SOCKET_ERROR)
		{
			printf("recvfrom  error :%d\n", WSAGetLastError());
		}
		printf("recvfrom data :%s\n", strbuf);
	}
	

	closesocket(clientSocket);
	WSACleanup();
	system("pause");
	return 0;
}