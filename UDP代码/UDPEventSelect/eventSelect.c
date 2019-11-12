#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib,"ws2_32.lib")

SOCKET serverSocket;
WSAEVENT serverEvent;

BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		closesocket(serverSocket);
		WSACloseEvent(serverEvent);
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
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		printf("版本校验失败\n");
		WSACleanup();
		system("pause");
		return 0;
	}
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == serverSocket)
	{
		printf("socket error:%d\n", WSAGetLastError());
		WSACleanup();
		system("pause");
		return 0;
	}
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("10.100.151.6");
	serverAddr.sin_port = htons(12345);
	if (SOCKET_ERROR == bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
	{
		printf("bind error:%d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		system("pause");
		return 0;
	}
	//创建事件
	serverEvent = WSACreateEvent();
	if (serverEvent == WSA_INVALID_EVENT)
	{
		int errorCode = WSAGetLastError();
		printf("WSACreateEvent error : %d\n", errorCode);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//绑定事件对象
	if( WSAEventSelect(serverSocket, serverEvent, FD_READ | FD_WRITE) == SOCKET_ERROR)
	{
		printf("绑定事件失败：%d\n", WSAGetLastError());
		WSACloseEvent(serverEvent);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}

	while (1)
	{
		DWORD Nres = WSAWaitForMultipleEvents(1, &serverEvent, FALSE, WSA_INFINITE, FALSE);
		if (Nres == WSA_WAIT_FAILED)
		{
			printf("wait  error :%d\n", WSAGetLastError());
			break;
		}
		WSANETWORKEVENTS networkEvent;
		if (SOCKET_ERROR == WSAEnumNetworkEvents(serverSocket, serverEvent, &networkEvent))
		{
			printf("错误码：%d\n", WSAGetLastError());
			break;
		}
		if (networkEvent.lNetworkEvents & FD_READ)
		{
			if (networkEvent.iErrorCode[FD_READ_BIT] == 0)
			{
				char strbuf[548] = { 0 };
				SOCKADDR_IN clientAddr;
				int fromlen = sizeof(serverAddr);
				int num = recvfrom(serverSocket, strbuf, sizeof(strbuf), 0, (struct sockaddr*)&clientAddr, &fromlen);
				if (num == SOCKET_ERROR)
				{
					printf("recvfrom error : %d\n", WSAGetLastError());
					continue;
				}
				printf("recvfrom data :%s\n", strbuf);
				num = sendto(serverSocket, "服务器收到", sizeof("服务器收到"), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
				if (num == SOCKET_ERROR)
				{
					printf("sendto error:%d\n", WSAGetLastError());
				}
			}
			else
			{
				printf("FD_READ  iErrorCode :%d\n", WSAGetLastError());
			}
		}
		if (networkEvent.lNetworkEvents & FD_WRITE)
		{
			if (networkEvent.iErrorCode[FD_WRITE_BIT] == 0)
			{
				printf("FD_WRITE\n");
			}
			else
			{
				printf("FD_WRITE  iErrorCode :%d\n", WSAGetLastError());
			}
		}
	}

	WSACloseEvent(serverEvent);
	closesocket(serverSocket);
	WSACleanup();
	return 0;
}
