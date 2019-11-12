#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib,"ws2_32.lib")


struct fd_es_set 
{
	unsigned short count;
	SOCKET allSockets[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT allEvents[WSA_MAXIMUM_WAIT_EVENTS];
};

struct fd_es_set esSet = { 0,{0},{NULL} };

BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		for (int i = 0;i < esSet.count;i++)
		{
			closesocket(esSet.allSockets[i]);
			WSACloseEvent(esSet.allEvents[i]);
		}
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
		//网络库加载失败
		switch (nRes)
		{
		case WSASYSNOTREADY:
			printf("重启电脑,或者检查网络库");
			break;
		case WSAVERNOTSUPPORTED:
			printf("请更新网络库");
			break;
		case WSAEINPROGRESS:
			printf("重新启动程序");
			break;
		case WSAEPROCLIM:
			printf("请尝试关闭不必要的软件,为当前程序运行提供充足资源");
			break;
		case WSAEFAULT:
			printf("函数第二个参数错误");
			break;
		}
		return 0;
	}
	//版本校验
	if (2!=HIBYTE(wsaData.wVersion) || 2!=LOBYTE(wsaData.wVersion))
	{
		WSACleanup();
		return 0;
	}
	//创建socket
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		//获取错误码
		int errorCode = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//绑定socket
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("10.100.145.239");
	serverAddr.sin_port = htons(12345);
	if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		//获取错误码
		int errorCode = WSAGetLastError();
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	/*监听*/
	if (SOCKET_ERROR == listen(serverSocket, SOMAXCONN))
	{
		/*获取错误码*/
		int errorCode = WSAGetLastError();
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	printf("等待客户端连接。。。。。。\n");
	

	//创建事件
	WSAEVENT serverEvent = WSACreateEvent();
	if (serverEvent == WSA_INVALID_EVENT)
	{
		int errorCode = WSAGetLastError();
		printf("创建事件失败：%d\n", errorCode);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//绑定事件对象
	if (WSAEventSelect(serverSocket, serverEvent, FD_ACCEPT) == SOCKET_ERROR)
	{
		printf("绑定事件失败：%d\n", WSAGetLastError());
		WSACloseEvent(serverEvent);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	esSet.allEvents[esSet.count] = serverEvent;
	esSet.allSockets[esSet.count] = serverSocket;
	esSet.count++; 
	while (1)
	{
		//获取发生信号的事件
		//参数4（毫秒）: 0：检查事件对象状态并立即返回不管有没有信号；WSA_INFINITE：一直等待直到有事件发生
		DWORD nRes = WSAWaitForMultipleEvents(esSet.count, esSet.allEvents, FALSE, WSA_INFINITE, FALSE);
		if (nRes == WSA_WAIT_FAILED)  //如果参数四设置了具体等待时间，并且超时则nIndex == WSA_WAIT_TIMEOUT,continue;即可
		{
			printf("错误码：%d\n", WSAGetLastError());
			break;
		}
		DWORD nIndex = nRes - WSA_WAIT_EVENT_0;
		//获取事件类型（accept、recv、send、close等），并将事件上的信号重置
		WSANETWORKEVENTS networkEvent;
		if (WSAEnumNetworkEvents(esSet.allSockets[nIndex], esSet.allEvents[nIndex], &networkEvent) == SOCKET_ERROR)
		{
			printf("错误码：%d\n", WSAGetLastError());
			break;
		}
		//产生accept事件
		if (networkEvent.lNetworkEvents & FD_ACCEPT) 
		{
			if (networkEvent.iErrorCode[FD_ACCEPT_BIT] == 0)  //事件accept正常
			{
				//正常
				SOCKET clientSocket = accept(esSet.allSockets[nIndex], NULL, NULL);
				if(INVALID_SOCKET == clientSocket)
				{
					continue;
				}
				//为新socket创建事件对象
				WSAEVENT clientEvent = WSACreateEvent();
				if (clientEvent == WSA_INVALID_EVENT)
				{
					int errorCode = WSAGetLastError();
					closesocket(clientSocket);
					continue;
				}
				//绑定事件对象--把客户端事件投递给系统监视
				if (SOCKET_ERROR == WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_WRITE | FD_CLOSE))
				{
					closesocket(clientSocket);
					WSACloseEvent(clientEvent);
					continue;
				}
				esSet.allSockets[esSet.count] = clientSocket;
				esSet.allEvents[esSet.count] = clientEvent;
				esSet.count++;
				printf("accept event\n");
			}
			else
			{
				printf("socket accept error,error code:%d\n", networkEvent.iErrorCode[FD_ACCEPT_BIT]);
				continue;
			}
		}
		//产生write事件  send
		if (networkEvent.lNetworkEvents & FD_WRITE)
		{
			if (networkEvent.iErrorCode[FD_WRITE_BIT] == 0)
			{
				//char sendbuf[1500] = { 0 };
				//scanf("%s", sendbuf);
				if (SOCKET_ERROR == send(esSet.allSockets[nIndex], "connect success", strlen("connect success"), 0))
				{
					printf("send failed,error code:%d",WSAGetLastError());
					continue;
				}
				printf("write event\n");
			}
			else
			{
				printf("socket write error,error code:%d\n", networkEvent.iErrorCode[FD_WRITE_BIT]);
				continue;
			}
		}
		//产生read事件 recv
		if (networkEvent.lNetworkEvents & FD_READ)
		{
			if (networkEvent.iErrorCode[FD_READ_BIT] == 0)
			{
				char recvbuf[1500] = { 0 };
				if (recv(esSet.allSockets[nIndex], recvbuf, 1499, 0) == SOCKET_ERROR)
				{
					printf("recv failed,error code:%d", WSAGetLastError());
					continue;
				}
				printf("recv data:%s\n", recvbuf);
			}
			else
			{
				printf("socket read error,error code:%d\n", networkEvent.iErrorCode[FD_READ_BIT]);
				continue;
			}
		}
		//产生close事件 客户端断开连接
		if (networkEvent.lNetworkEvents & FD_CLOSE)
		{
			printf("*******client close*******\n");
			//清理客户端套接字，事件
			closesocket(esSet.allSockets[nIndex]);
			esSet.allSockets[nIndex] = esSet.allSockets[esSet.count - 1];
			WSACloseEvent(esSet.allEvents[nIndex]);
			esSet.allEvents[nIndex] = esSet.allEvents[esSet.count - 1];
			esSet.count--;
			//printf("socket close error,error code:%d\n", networkEvent.iErrorCode[FD_CLOSE_BIT]);
		}
	}

	for (int i=0;i<esSet.count;i++)
	{
		//释放事件对象（占内核内存）
		closesocket(esSet.allSockets[i]);
		WSACloseEvent(esSet.allEvents[i]);
	}
	WSACleanup();
	return 0;
}