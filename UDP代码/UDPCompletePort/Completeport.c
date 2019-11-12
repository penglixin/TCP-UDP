#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib,"ws2_32.lib")

SOCKET serverSocket;
WSAOVERLAPPED serverLap;
SOCKADDR_IN clientAddr;
char recvBuf[548];
HANDLE hPort;
//根据系统创建的线程数
int nProcesscount;
BOOL g_flag = TRUE;

int PostRecvFrom(void);
int PostSendTo(struct sockaddr_in* sa);
DWORD WINAPI  ServerThread(LPVOID lpParameter);

BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		g_flag = FALSE;
		//关闭完成端口
		CloseHandle(hPort);
		closesocket(serverSocket);
		WSACloseEvent(serverLap.hEvent);
		WSACleanup();
		break;
	}
	return TRUE;
}


int main()
{
	SetConsoleCtrlHandler(fun, TRUE);
	//打开网络库
	WSADATA wsaData;
	int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRes != 0)
	{
		printf("网络库打开失败，错误码：%d\n", WSAGetLastError());
		system("pause");
		return 0;
	}
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
	serverAddr.sin_addr.S_un.S_addr = inet_addr("192.168.137.110");
	serverAddr.sin_port = htons(12345);
	if (SOCKET_ERROR == bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
	{
		printf("绑定socket失败，错误码：%d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		system("pause");
		return 0;
	}
	
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
	//创建完成端口
	hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hPort == 0)
	{
		printf("创建完成端口失败:%d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//绑定完成端口
	hPort = CreateIoCompletionPort((HANDLE)serverSocket, hPort, serverSocket, 0);
	if (hPort == 0)
	{
		printf("绑定完成端口失败:%d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//获取需要创建线程数
	SYSTEM_INFO systemProcessCount;
	GetSystemInfo(&systemProcessCount);
	nProcesscount = systemProcessCount.dwNumberOfProcessors;
	/*创建线程
	参数2单位为字节：填0:表示默认线程栈大小 1M，
	参数5：0:创建完线程立即执行serverThread;CREATE_SUSPENDED:创建完线程挂起，调用ResumeThread()启动函数*/
	for (int i = 0;i < nProcesscount;i++)
	{
		DWORD ThreadID;
		if (NULL == CreateThread(NULL, 0, ServerThread, hPort, 0, &ThreadID))
		{
			int a = WSAGetLastError();
			printf("创建线程失败:%d\n", a);
			i--;
		}
	}
	printf("******(UDP)完成端口******\n");
	//投递recvfrom事件
	if (0 == PostRecvFrom())
	{
		int errorCode = WSAGetLastError();
		printf("PostRecvFrom error : %d\n", errorCode);
		//关闭完成端口
		CloseHandle(hPort);
		WSACloseEvent(serverLap.hEvent);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//主线程阻塞
	while (1)
	{
		Sleep(1000);
	}
	g_flag = FALSE;
	//关闭完成端口
	CloseHandle(hPort);
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
	int nlen = sizeof(clientAddr);
	int nRes = WSARecvFrom(serverSocket, &wsaBuf, 1, &dwRecvByte, &dwFlag, (struct sockaddr*)&clientAddr, &nlen, &serverLap, NULL);
	int a = WSAGetLastError();
	if (a == WSA_IO_PENDING)
	{
		//异步
		return 1;
	}
	return 0;
}

int PostSendTo(struct sockaddr_in* sa)
{
	WSABUF  wsaBuf;
	wsaBuf.buf = "OK";
	wsaBuf.len = 547;
	DWORD dwSendByte = 0;
	DWORD dwFlag = 0;
	int iTolen = sizeof(SOCKADDR);
	int nRes = WSASendTo(serverSocket, &wsaBuf, 1, &dwSendByte, dwFlag, (struct sockaddr*)sa, iTolen, &serverLap, NULL);
	int a = WSAGetLastError();
	if (WSA_IO_PENDING == a)
	{
		return 1;
	}
	return 0;
}

DWORD WINAPI ServerThread(LPVOID lpParameter)
{
	DWORD dwNumberofByte;
	ULONG_PTR index;
	WSAOVERLAPPED *lpOverlpd;
	while (g_flag)
	{
		BOOL bFlag = GetQueuedCompletionStatus(hPort, &dwNumberofByte, &index, &lpOverlpd, INFINITE);
		if (!bFlag)
		{
			continue;
		}
		//将事件信号重置
		WSAResetEvent(serverLap.hEvent);
		//成功
		if (0 != recvBuf[0])
		{
			//recvfrom
			printf("Thread WSArecvfrom data:%s\n", recvBuf);
			recvBuf[0] = 0;
			//memset(recvBuf, 0, 548);
			PostSendTo(&clientAddr);
			PostRecvFrom();
		}
		else
		{
			//sendto
			printf("Thread PostSendTo Success!\n");
		}
	}
	return 0;
}
