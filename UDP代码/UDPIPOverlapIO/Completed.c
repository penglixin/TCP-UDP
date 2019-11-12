//#define _WINSOCK_DEPRECATED_NO_WARNINGS
//#include <WinSock2.h>
//#include <stdio.h>
//#include <stdlib.h>
//
//#pragma comment(lib,"ws2_32.lib")
//
//SOCKET serverSocket;
//WSAOVERLAPPED serverLap;
//SOCKADDR_IN clientAddr;
//char recvBuf[548];
//int PostRecvFrom(void);
//int PostSendTo(struct sockaddr_in* sa);
//
//BOOL WINAPI fun(DWORD dwCtrlType)
//{
//	switch (dwCtrlType)
//	{
//	case CTRL_CLOSE_EVENT:
//		closesocket(serverSocket);
//		WSACloseEvent(serverLap.hEvent);
//		WSACleanup();
//		break;
//	}
//	return TRUE;
//}
//
//int main(void)
//{
//	SetConsoleCtrlHandler(fun, TRUE);
//	//打开网络库
//	WSADATA wsaData;
//	int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
//	if (nRes != 0)
//	{
//		printf("网络库打开失败，错误码：%d\n", WSAGetLastError());
//		system("pause");
//		return 0;
//	}
//	//版本校验
//	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
//	{
//		//打开网络库失败
//		printf("版本校验失败\n");
//		WSACleanup();
//		system("pause");
//		return 0;
//	}
//	//创建socket
//	serverSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
//	if (serverSocket == INVALID_SOCKET)
//	{
//		printf("创建socket失败，错误码：%d\n", WSAGetLastError());
//		WSACleanup();
//		system("pause");
//		return 0;
//	}
//	//绑定
//	SOCKADDR_IN serverAddr;
//	serverAddr.sin_family = AF_INET;
//	serverAddr.sin_addr.S_un.S_addr = inet_addr("10.100.151.6");
//	serverAddr.sin_port = htons(12345);
//	if (SOCKET_ERROR == bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
//	{
//		printf("绑定socket失败，错误码：%d\n", WSAGetLastError());
//		closesocket(serverSocket);
//		WSACleanup();
//		system("pause");
//		return 0;
//	}
//	printf("******(UDP)完成例程******\n");
//	//创建事件
//	serverLap.hEvent = WSACreateEvent();
//	if (serverLap.hEvent == WSA_INVALID_EVENT)
//	{
//		int errorCode = WSAGetLastError();
//		printf("WSACreateEvent error : %d\n", errorCode);
//		closesocket(serverSocket);
//		WSACleanup();
//		return 0;
//	}
//	if (0 == PostRecvFrom())
//	{
//		int errorCode = WSAGetLastError();
//		printf("PostRecvFrom error : %d\n", errorCode);
//		WSACloseEvent(serverLap.hEvent);
//		closesocket(serverSocket);
//		WSACleanup();
//		return 0;
//	}
//	while (1)
//	{
//		/*参数6改成TRUE（只用在重叠IO里的完成例程模型）:
//		实现等待事件函数和完成例程函数的异步执行，当完成例程函数执行完会给等待事件函数一个信号 WSA_WAIT_IO_COMPLETION ,
//		一个完成例程完成了就会返回这个宏*/
//		int nRes = WSAWaitForMultipleEvents(1, &serverLap.hEvent, FALSE, WSA_INFINITE, TRUE);
//		if (WSA_WAIT_FAILED == nRes || WSA_WAIT_IO_COMPLETION == nRes)
//		{
//			int a = WSAGetLastError();
//			continue;
//		}
//	}
//	closesocket(serverSocket);
//	WSACloseEvent(serverLap.hEvent);
//	WSACleanup();
//	return 0;
//}
//
////回调函数是在异步事件完成之后才会去调用
//void CALLBACK RecvfromCallBck(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
//{
//	WSAResetEvent(lpOverlapped->hEvent);
//	if (dwError != 0)
//	{
//		printf("recv failed!\n");
//		return;
//	}
//	if (cbTransferred>0)
//	{
//		//接收到了
//		printf("WSArecvfrom data:%s\n",recvBuf);
//		recvBuf[0] = 0;
//		PostRecvFrom();
//		PostSendTo(&clientAddr);
//	}
//}
//
//int PostRecvFrom(void)
//{
//	WSABUF  wsaBuf;
//	wsaBuf.buf = recvBuf;
//	wsaBuf.len = 547;
//	DWORD dwRecvByte = 0;
//	DWORD dwFlag = 0;
//	int nLen = sizeof(clientAddr);
//	int nRes = WSARecvFrom(serverSocket, &wsaBuf, 1, &dwRecvByte, &dwFlag, (struct sockaddr*)&clientAddr, &nLen, &serverLap, RecvfromCallBck);
//	if (nRes == 0)
//	{
//		//立即完成    同步
//		printf("WSArecvfrom data:%s\n", wsaBuf.buf);
//		recvBuf[0] = 0;
//		//memset(recvBuf, 0, 548);
//		PostSendTo(&clientAddr);
//		PostRecvFrom();
//	}
//	else
//	{
//		int a = WSAGetLastError();
//		if (a == WSA_IO_PENDING)
//		{
//			//异步
//			return 1;
//		}
//	}
//	return 0;
//}
//
//void CALLBACK SendToCallBck(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
//{
//	if (dwError != 0)
//	{
//		printf("send failed\n");
//		return;
//	}
//	printf("send over\n");
//}
//
//int PostSendTo(struct sockaddr_in* sa)
//{
//	WSABUF  wsaBuf;
//	wsaBuf.buf = "ok";
//	wsaBuf.len = 547;
//	DWORD dwSendByte = 0;
//	int nlen = sizeof(struct sockaddr_in);
//	int nRes = WSASendTo(serverSocket, &wsaBuf, 1, &dwSendByte, 0, (struct sockaddr*)sa, nlen, &serverLap, SendToCallBck);
//	if (nRes == 0)
//	{
//		//立即完成
//		printf("PostSendTo Success!\n");
//	}
//	else
//	{
//		int a = WSAGetLastError();
//		if (WSA_IO_PENDING == a)
//		{
//			//printf("PostSendTo later!\n");
//			return 1;
//		}
//	}
//	return 0;
//}
