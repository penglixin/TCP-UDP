#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <MSWSock.h>
#include <stdlib.h>
#include <stdio.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Mswsock.lib")

#define  MAX_COUNT  1024
#define  MAX_RECV_COUNT  1024
SOCKET  g_allSock[MAX_COUNT];
OVERLAPPED g_Overlpd[MAX_COUNT];
int g_count;

char g_strRecv[MAX_RECV_COUNT];

int PostAccept();
int PostRecv(int count);
int PostSend(int index);

void Clear()
{
	for (int i = 0;i < g_count;i++)
	{
		closesocket(g_allSock[i]);
		WSACloseEvent(g_Overlpd[i].hEvent);
	}
}

BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		Clear();
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
	SOCKET serverSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == serverSocket)
	{
		printf("WSASocket error:%d", WSAGetLastError());
		WSACleanup();
		return 0;
	}
	//绑定socket
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("10.100.145.239");
	serverAddr.sin_port = htons(12345);
	if (bind(serverSocket,(SOCKADDR*)&serverAddr,sizeof(serverAddr)) == SOCKET_ERROR)
	{
		printf("bind error:%d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//监听
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("listen error:%d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	printf("******等待客户端连接(event通知)******\n");
	g_allSock[g_count] = serverSocket;
	g_Overlpd[g_count].hEvent = WSACreateEvent();
	g_count++;
	//创建客户端连接AcceptEx
	if (0 != PostAccept())
	{
		printf("PostAccept error:%d\n", PostAccept());
		Clear();
		WSACleanup();
		return 0;
	}
	while (1)
	{
		for (int i = 0; i < g_count;i++)
		{
			int nRes = WSAWaitForMultipleEvents(1, &(g_Overlpd[i].hEvent), FALSE, 0, FALSE);
			if (nRes == WSA_WAIT_FAILED || nRes == WSA_WAIT_TIMEOUT)
			{
				continue;
			}
			//有信号了
			DWORD dwStatus;
			DWORD dwFlag;
			BOOL bFlag = WSAGetOverlappedResult(g_allSock[i], &g_Overlpd[i], &dwStatus, TRUE, &dwFlag);
			//重置信号
			WSAResetEvent(g_Overlpd[i].hEvent);
			if (!bFlag)
			{
				int a = WSAGetLastError();
				if (a == 10054)
				{
					printf("******client force closed******\n");
					//客户端下线
					//关闭socket，event
					closesocket(g_allSock[i]);
					WSACloseEvent(g_Overlpd[i].hEvent);
					g_allSock[i] = g_allSock[g_count - 1];
					g_Overlpd[i] = g_Overlpd[g_count - 1];
					i--;
					g_count--;
				}
				continue;
			}
			if (0 == i)
			{
				PostSend(g_count);
				printf("******accept******\n");
				//接受连接
				//投递recv
				PostRecv(g_count);
				g_count++;
				//投递Accept
				PostAccept();
				continue;
			}
			if (0 == dwStatus)
			{
				printf("******client closed******\n");
				//客户端下线
				//关闭socket，event
				closesocket(g_allSock[i]);
				WSACloseEvent(g_Overlpd[i].hEvent);
				g_allSock[i] = g_allSock[g_count - 1];
				g_Overlpd[i] = g_Overlpd[g_count - 1];
				i--;
				g_count--;
				continue;
			}
			if (0 != dwStatus)
			{
				//发送或者接收成功
				if (g_strRecv[0]!=0)
				{
					//recv
					printf("WSARecv Data:%s\n",g_strRecv);
					//清空buff
					memset(g_strRecv, 0, MAX_RECV_COUNT);
					PostRecv(i);
				}
				else
				{
					//send
				}

			}
		}
	}


	Clear();
	WSACleanup();
	return 0;
}

int PostAccept()
{
	g_allSock[g_count] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_Overlpd[g_count].hEvent = WSACreateEvent();
	char str[1024] = { 0 };  
	//参数3（不能设置成NULL）功能：客户端第一次send的数据由AcceptEx函数接收，存放在str内；第二次send的数据由WSARecv接收
	//如果参数四设置成0那么取消了参数3的功能；如果填写参数3的1024那么就会接收第一次数据
	DWORD dwRecvcount;
	if (TRUE == AcceptEx(g_allSock[0], g_allSock[g_count], str, 0, sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16, &dwRecvcount, &g_Overlpd[0]))
	{
		//立即完成
		//投递recv
		PostRecv(g_count);
		g_count++;
		//投递Accept
		PostAccept();
		return 0;
	}
	else
	{
		int a = WSAGetLastError();
		if (ERROR_IO_PENDING == a)
		{
			//稍后处理
			return 0;
		}
		else
		{
			return a;
		}
	}
}

int PostRecv(int index)
{
	WSABUF wsabuf;
	wsabuf.buf = g_strRecv;
	wsabuf.len = MAX_RECV_COUNT;

	DWORD dwRecvCount;
	DWORD dwFlag = 0;
	int nRes = WSARecv(g_allSock[index], &wsabuf, 1, &dwRecvCount, &dwFlag, &g_Overlpd[index], NULL);
	if (nRes == 0)
	{
		//立即执行
		printf("WSARecv Data:%s\n", wsabuf.buf);
		//清空buff
		memset(g_strRecv, 0, MAX_RECV_COUNT);
		PostRecv(index);
		return 0;
	} 
	else
	{
		int a = WSAGetLastError();
		if (ERROR_IO_PENDING == a)
		{
			//稍后处理
			return 0;
		}
		else
		{
			return a;
		}
	}
}

int PostSend(int index)
{
	WSABUF wsabuf;
	wsabuf.buf = "你好，这里是服务器";
	wsabuf.len = MAX_RECV_COUNT;

	DWORD dwSendCount;
	DWORD dwFlag = 0;
	int nRes = WSASend(g_allSock[index], &wsabuf, 1, &dwSendCount, dwFlag, &g_Overlpd[index], NULL);
	if (nRes == 0)
	{
		//立即执行
		printf("WSASend Success\n");
		return 0;
	}
	else
	{
		int a = WSAGetLastError();
		if (ERROR_IO_PENDING == a)
		{
			//稍后处理
			return 0;
		}
		else
		{
			return a;
		}
	}
}