//#define _WINSOCK_DEPRECATED_NO_WARNINGS 
//#include <WinSock2.h>
//#include <stdio.h>
//#include <stdlib.h>
//#pragma comment(lib,"ws2_32.lib")
//
//
//struct fd_es_set
//{
//	unsigned short count;
//	SOCKET allSockets[WSA_MAXIMUM_WAIT_EVENTS];
//	WSAEVENT allEvents[WSA_MAXIMUM_WAIT_EVENTS];
//};
//
//BOOL WINAPI fun(DWORD dwCtrlType)
//{
//	switch (dwCtrlType)
//	{
//	case CTRL_CLOSE_EVENT:
//
//		break;
//	}
//
//	return TRUE;
//}
//
//
//int main()
//{
//	SetConsoleCtrlHandler(fun, TRUE);
//	WSADATA wsaData;
//	int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
//	if (nRes != 0)
//	{
//		//网络库加载失败
//		switch (nRes)
//		{
//		case WSASYSNOTREADY:
//			printf("重启电脑,或者检查网络库");
//			break;
//		case WSAVERNOTSUPPORTED:
//			printf("请更新网络库");
//			break;
//		case WSAEINPROGRESS:
//			printf("重新启动程序");
//			break;
//		case WSAEPROCLIM:
//			printf("请尝试关闭不必要的软件,为当前程序运行提供充足资源");
//			break;
//		case WSAEFAULT:
//			printf("函数第二个参数错误");
//			break;
//		}
//		return 0;
//	}
//	//版本校验
//	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
//	{
//		WSACleanup();
//		return 0;
//	}
//	//创建socket
//	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if (serverSocket == INVALID_SOCKET)
//	{
//		//获取错误码
//		int errorCode = WSAGetLastError();
//		WSACleanup();
//		return 0;
//	}
//	//绑定socket
//	SOCKADDR_IN serverAddr;
//	serverAddr.sin_family = AF_INET;
//	serverAddr.sin_addr.S_un.S_addr = inet_addr("192.168.137.1");
//	serverAddr.sin_port = htons(12345);
//	if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
//	{
//		//获取错误码
//		int errorCode = WSAGetLastError();
//		closesocket(serverSocket);
//		WSACleanup();
//		return 0;
//	}
//	//监听
//	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
//	{
//		/*获取错误码*/
//		int errorCode = WSAGetLastError();
//		closesocket(serverSocket);
//		WSACleanup();
//		return 0;
//	}
//	printf("等待客户端连接(优化eventselect)......\n");
//	struct fd_es_set esSet = { 0,{0},{NULL} };
//	//创建事件
//	WSAEVENT serverEvent = WSACreateEvent();
//	if (serverEvent == WSA_INVALID_EVENT)
//	{
//		printf("创建事件失败：%d\n", WSAGetLastError());
//		closesocket(serverSocket);
//		WSACleanup();
//		return 0;
//	}
//	//绑定事件对象
//	if (WSAEventSelect(serverSocket, serverEvent, FD_ACCEPT) == SOCKET_ERROR)
//	{
//		printf("绑定事件失败：%d\n", WSAGetLastError());
//		WSACloseEvent(serverEvent);
//		closesocket(serverSocket);
//		WSACleanup();
//		return 0;
//	}
//	//绑定成功，把socket，事件添加进数组
//	esSet.allSockets[esSet.count] = serverSocket;
//	esSet.allEvents[esSet.count] = serverEvent;
//	esSet.count++;
//	while (1)
//	{
//		DWORD nRes = WSAWaitForMultipleEvents(esSet.count, esSet.allEvents, FALSE, WSA_INFINITE, FALSE);
//		if (nRes == WSA_WAIT_FAILED)
//		{
//			printf("错误码：%d\n", WSAGetLastError());
//			break;
//		}
//		DWORD nIndex = nRes - WSA_WAIT_EVENT_0;
//
//		for (int i = nIndex;i < esSet.count;i++)
//		{
//			//获取发生信号的事件
//			//参数4（毫秒）: 0：检查事件对象状态并立即返回不管有没有信号；WSA_INFINITE：一直等待直到有事件发生
//			DWORD nRes = WSAWaitForMultipleEvents(1, &esSet.allEvents[i], FALSE, 0, FALSE);
//			if (nRes == WSA_WAIT_FAILED)
//			{
//				printf("WSAWaitForMultipleEvents错误码：%d\n", WSAGetLastError());
//				continue;
//			}
//			if (nRes == WSA_WAIT_TIMEOUT)
//			{
//				continue;
//			}
//			//获取事件类型（accept、recv、send、close等），并将事件上的信号重置
//			WSANETWORKEVENTS networkEvent;
//			if (SOCKET_ERROR == WSAEnumNetworkEvents(esSet.allSockets[i], esSet.allEvents[i], &networkEvent))
//			{
//				printf("WSAEnumNetworkEvents错误码：%d\n", WSAGetLastError());
//				break;
//			}
//			//产生accept事件
//			if(networkEvent.lNetworkEvents & FD_ACCEPT)
//			{
//				//accept事件正常
//				if (networkEvent.iErrorCode[FD_ACCEPT_BIT] == 0)
//				{
//					SOCKET clientSocket = accept(esSet.allSockets[i], NULL, NULL);
//					if (INVALID_SOCKET == clientSocket)
//					{
//						continue;
//					}
//					//为新socket创建新事件
//					WSAEVENT clientEvent = WSACreateEvent();
//					if(clientEvent == WSA_INVALID_EVENT)
//					{
//						int errorCode = WSAGetLastError();
//						closesocket(clientSocket);
//						continue;
//					}
//					//绑定事件对象--把客户端事件投递给系统监视
//					if (SOCKET_ERROR == WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_WRITE | FD_CLOSE))
//					{
//						int errorCode = WSAGetLastError();
//						WSACloseEvent(clientEvent);
//						closesocket(clientSocket);
//						continue;
//					}
//					esSet.allSockets[esSet.count] = clientSocket;
//					esSet.allEvents[esSet.count] = clientEvent;
//					esSet.count++;
//					printf("accept event\n");
//				} 
//				else
//				{
//					printf("socket accept error,error code:%d\n", networkEvent.iErrorCode[FD_ACCEPT_BIT]);
//					continue;
//				}
//			}
//			//产生write事件
//			if (networkEvent.lNetworkEvents & FD_WRITE)
//			{
//				if (networkEvent.iErrorCode[FD_WRITE_BIT] == 0)
//				{
//					if (SOCKET_ERROR == send(esSet.allSockets[i], "connect success", strlen("connect success"), 0))
//					{
//						printf("send failed,error code:%d", WSAGetLastError());
//						continue;
//					}
//					printf("write event\n");
//				}
//				else
//				{
//					printf("socket write error,error code:%d\n", networkEvent.iErrorCode[FD_WRITE_BIT]);
//					continue;
//				}
//			}
//			//产生read事件
//			if (networkEvent.lNetworkEvents & FD_READ)
//			{
//				if (networkEvent.iErrorCode[FD_READ_BIT] == 0)
//				{
//					char recvbuf[1500] = { 0 };
//					if (recv(esSet.allSockets[i], recvbuf, 1499, 0) == SOCKET_ERROR)
//					{
//						printf("recv failed,error code:%d", WSAGetLastError());
//						continue;
//					}
//					printf("recv data:%s\n", recvbuf);
//				}
//				else
//				{
//					printf("socket read error,error code:%d\n", networkEvent.iErrorCode[FD_READ_BIT]);
//					continue;
//				}
//
//			}
//			//产生close事件
//			if (networkEvent.lNetworkEvents & FD_CLOSE)
//			{
//				printf("*******client close*******\n");
//				//清理客户端套接字，事件
//				closesocket(esSet.allSockets[i]);
//				esSet.allSockets[i] = esSet.allSockets[esSet.count - 1];
//				WSACloseEvent(esSet.allEvents[i]);
//				esSet.allEvents[i] = esSet.allEvents[esSet.count - 1];
//				esSet.count--;
//				//printf("socket close error,error code:%d\n", networkEvent.iErrorCode[FD_CLOSE_BIT]);
//			}
//		}
//	}
//
//
//	WSACloseEvent(serverEvent);
//	closesocket(serverSocket);
//	WSACleanup();
//	return 0;
//}