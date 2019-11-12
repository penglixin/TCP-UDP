#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib,"ws2_32.lib")

SOCKET serverSocket;
WSAOVERLAPPED serverLap;
SOCKADDR_IN clientAddr;
char recvBuf[548];
int PostRecvFrom(void);
int PostSendTo(struct sockaddr_in* sa);

BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		closesocket(serverSocket);
		WSACloseEvent(serverLap.hEvent);
		WSACleanup();
		break;
	}
	return TRUE;
}

int main(void)
{
	SetConsoleCtrlHandler(fun, TRUE);
	//打开网络库
	WSADATA wsaData;
	int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRes != 0)
	{
		printf("网络库打开失败，错误码：%d\n", WSAGetLastError());
		system("pause");
		return 0;	}
	//版本校验
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		//打开网络库失败
		printf("版本校验失败\n");
		WSACleanup();
		system("pause");
		return 0;
	}
	//创建socket
	serverSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (serverSocket == INVALID_SOCKET)
	{
		printf("创建socket失败，错误码：%d\n", WSAGetLastError());
		WSACleanup();
		system("pause");
		return 0;
	}
	//绑定
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("10.100.151.6");
	serverAddr.sin_port = htons(12345);
	if (SOCKET_ERROR == bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
	{
		printf("绑定socket失败，错误码：%d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		system("pause");
		return 0;
	}
   printf("******(UDP)事件通知******\n");
	//创建事件
	serverLap.hEvent = WSACreateEvent();
	if (serverLap.hEvent == WSA_INVALID_EVENT)
	{
		int errorCode = WSAGetLastError();
		printf("WSACreateEvent error : %d\n", errorCode);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	if (0 == PostRecvFrom())
	{
		int errorCode = WSAGetLastError();
		printf("PostRecvFrom error : %d\n", errorCode);
		WSACloseEvent(serverLap.hEvent);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	while (1)
	{
		//循环等信号
		int nRes = WSAWaitForMultipleEvents(1, &serverLap.hEvent, FALSE, WSA_INFINITE, FALSE);
		if (WSA_WAIT_FAILED == nRes)
		{
			continue;
		}
		DWORD dwStatus;
		DWORD dwFlag;
		BOOL bFlag = WSAGetOverlappedResult(serverSocket, &serverLap, &dwStatus, FALSE, &dwFlag);
		//将信号置空
		WSAResetEvent(serverLap.hEvent);
		if (!bFlag)
		{
			//返回false 执行失败
			int a = WSAGetLastError();
			if (10054 == a)
			{
				//客户端强制退出 ,但是UDP协议是基于非连接的，所以不存在客户端退出
			}
			printf("WSAGetOverlappedResult  error :%d\n", a);
			continue;
		}
		//分类处理
		if(dwStatus > 0)
		{
			if (recvBuf[0] == 0)
			{
				printf("WSAsendto  later\n");
				//当客户端用gets(strBuf)接受输入时，直接按下回车就会执行到这个if，因为按下回车时发送字符'\0'
				//PostSendTo(&clientAddr);
				//PostRecvFrom();
			}
			else
			{
				printf("(循环)WSArecvfrom data:%s\n", recvBuf);
				recvBuf[0] = 0;
				//memset(recvBuf, 0, 548);
				PostSendTo(&clientAddr);
				PostRecvFrom();
			}

		}
	}
	closesocket(serverSocket);
	WSACloseEvent(serverLap.hEvent);
	WSACleanup();
	return 0;
}

int PostRecvFrom(void)
{
	WSABUF  wsaBuf;
	wsaBuf.buf = recvBuf;
	wsaBuf.len = 547;
	DWORD dwRecvByte = 0;
	DWORD dwFlag = 0;
	//SOCKADDR_IN clientAddr;
	int nLen = sizeof(clientAddr);
	int nRes = WSARecvFrom(serverSocket, &wsaBuf, 1, &dwRecvByte, &dwFlag, (struct sockaddr*)&clientAddr, &nLen, &serverLap, NULL);
	if (nRes == 0)
	{
		//立即完成    同步
		printf("(立即完成)WSArecvfrom data:%s\n", wsaBuf.buf);
		recvBuf[0] = 0;
		//memset(recvBuf, 0, 548);
		PostSendTo(&clientAddr);
		PostRecvFrom();
	}
	else
	{
		int a = WSAGetLastError();
		if (a == WSA_IO_PENDING)
		{
			//异步
			return 1;
		}
	}
	return 0;
}

int PostSendTo(struct sockaddr_in* sa)
{
	WSABUF  wsaBuf;
	wsaBuf.buf = "ok";
	wsaBuf.len = 547;
	DWORD dwSendByte = 0;
	int nlen = sizeof(struct sockaddr_in);
	int nRes = WSASendTo(serverSocket, &wsaBuf, 1, &dwSendByte, 0, (struct sockaddr*)sa, nlen, &serverLap, NULL);
	if (nRes == 0)
	{
		//立即完成
		printf("PostSendTo Success!\n");
	}
	else
	{
		int a = WSAGetLastError();
		if (WSA_IO_PENDING == a)
		{
			//printf("PostSendTo later!\n");
			return 1;
		}
	}
	return 0;
}
