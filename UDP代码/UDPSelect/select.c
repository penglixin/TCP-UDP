#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib,"ws2_32.lib")

SOCKET serverSocket;

BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		closesocket(serverSocket);
		WSACleanup();
		break;
	}

	return TRUE;
}

int main(void)
{
	SetConsoleCtrlHandler(fun, TRUE);
	WSADATA wsaData;
	int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRes != 0)
	{
		printf("网络库打开失败，错误码：%d\n", WSAGetLastError());
		system("pause");
		return 0;
	}
	if (2!=HIBYTE(wsaData.wVersion) || 2!=LOBYTE(wsaData.wVersion))
	{
		printf("版本校验失败\n");
		WSACleanup();
		system("pause");
		return 0;
	}
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == serverSocket)
	{
		printf("socket error:%d\n",WSAGetLastError());
		WSACleanup();
		system("pause");
		return 0;
	}
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("10.100.151.6");
	serverAddr.sin_port = htons(12345);
	if (SOCKET_ERROR == bind(serverSocket,(SOCKADDR*)&serverAddr,sizeof(serverAddr)))
	{
		printf("bind error:%d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		system("pause");
		return 0;
	}
	while (1)
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(serverSocket, &fds);
		struct timeval tvl;
		tvl.tv_sec = 3;
		tvl.tv_usec = 0;
		int nRes = select(0, &fds, NULL, NULL, &tvl);
		if (nRes == 0)
		{
			continue;
		}
		else if (nRes > 0)
		{
			//接收消息
			char strbuf[548] = { 0 };
			SOCKADDR_IN clientAddr;
			int fromlen = sizeof(serverAddr);
			int num = recvfrom(serverSocket, strbuf, sizeof(strbuf), 0, (struct sockaddr*)&clientAddr, &fromlen);
			if (num == SOCKET_ERROR)
			{
				printf("recvfrom error:%d\n", WSAGetLastError());
			}
			printf("recvfrom data:%s\n", strbuf);
			//收到消息后给客户端一个回复
			num = sendto(serverSocket, "服务端已收到", sizeof("服务端已收到"), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
			if (num == SOCKET_ERROR)
			{
				printf("sendto error:%d\n", WSAGetLastError());
			}
		}
		else
		{
			printf("select error :%d\n", WSAGetLastError());
			break;
		}
	}

	closesocket(serverSocket);
	WSACleanup();
	return 0;
}