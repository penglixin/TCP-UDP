#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <MSWSock.h>
#include <stdio.h>
#include <stdlib.h>
#pragma  comment(lib,"ws2_32.lib")
#pragma  comment(lib,"mswsock.lib")


#define MAX_COUNT 1024
#define MAX_RECV_COUNT 1024
SOCKET g_allSock[MAX_COUNT];
OVERLAPPED g_Overlpd[MAX_COUNT];
int g_count;
char g_strRecv[MAX_RECV_COUNT];
HANDLE hPort;
HANDLE *pThread;
int nProcessorCount;
BOOL g_flag = TRUE;

int PostAccept(void);
int PostRecv(int index);
int PostSend(int index);

void Clear(void)
{
	for (int i = 0;i < g_count;i++)
	{
		if (g_allSock[i]==0)
			continue;
		closesocket(g_allSock[i]);
		WSACloseEvent(g_Overlpd[i].hEvent);
	}
	WSACleanup();
}

BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		g_flag = FALSE;
		//释放句柄
		for (int i = 0;i < nProcessorCount;i++)
		{
			CloseHandle(pThread[i]);
		}
		free(pThread);
		CloseHandle(hPort);
		Clear();
		break;
	}

	return TRUE;
}

DWORD WINAPI serverThread(LPVOID lpParameter);

int main()
{
	//向操作系统投递一个句柄，当关闭窗口时调用函数fun
	SetConsoleCtrlHandler(fun, TRUE);
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
	//绑定
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("10.100.145.239");
	serverAddr.sin_port = htons(12345);
	if(bind(serverSocket,(SOCKADDR*)&serverAddr,sizeof(serverAddr)) == SOCKET_ERROR)
	{
		printf("bind error:%d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	g_allSock[g_count] = serverSocket;
	g_Overlpd[g_count].hEvent = WSACreateEvent();
	g_count++;
	//创建完成端口
	hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); //参数四0表示自动获取CPU的核数，自动创建合适的线程数
	if (hPort == 0)
	{
		printf("创建完成端口 error:%d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//绑定完成端口
	HANDLE hPort2 = CreateIoCompletionPort((HANDLE)serverSocket, hPort, 0, 0);//参数四0与上面参数四0意义不一样，这里表示完全忽略，填啥都不起作用
	if (hPort != hPort2)
	{
		printf("绑定完成端口 error:%d", WSAGetLastError());
		CloseHandle(hPort);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//监听
	if (SOCKET_ERROR == listen(serverSocket, SOMAXCONN))
	{
		printf("listen error:%d\n", WSAGetLastError());
		CloseHandle(hPort);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	printf("******等待客户端连接(完成端口)******\n");
	
	if (PostAccept()!=0)
	{
		printf("PostAccept error:%d\n", PostAccept());
		CloseHandle(hPort);
		Clear();
		return 0;
	}
	
	//获取需要创建线程数
	SYSTEM_INFO systemProcessorsCount;
	GetSystemInfo(&systemProcessorsCount);
	nProcessorCount = systemProcessorsCount.dwNumberOfProcessors;
	//创建线程
	pThread = (HANDLE*)malloc(sizeof(HANDLE)*nProcessorCount);
	for (int i=0;i<nProcessorCount;i++)
	{
		//参数2单位为字节：填0:表示默认线程栈大小 1M，
		//参数5：0:创建完线程立即执行serverThread;CREATE_SUSPENDED:创建完线程挂起，调用ResumeThread()启动函数
		pThread[i] = CreateThread(NULL, 0, serverThread, hPort, 0, NULL);
		if (NULL == pThread[i])
		{
			printf("CreateThread error:%d,index:%d\n", WSAGetLastError(),i);
			CloseHandle(hPort);
			closesocket(serverSocket);
			WSACleanup();
			return 0;
		}
	}
	//阻塞主线程
	while (1)
	{
		Sleep(1000);
	}
	//释放句柄
	for (int i=0;i<nProcessorCount;i++)
	{
		CloseHandle(pThread[i]);
	}
	free(pThread);
	CloseHandle(hPort);
	Clear();
	return 0;
}

int PostAccept(void)
{
	g_allSock[g_count] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_Overlpd[g_count].hEvent = WSACreateEvent();
	char str[MAX_RECV_COUNT] = { 0 };
	//参数3（不能设置成NULL）功能：客户端第一次send的数据由AcceptEx函数接收，存放在str内；第二次send的数据由WSARecv接收
	//如果参数四设置成0那么取消了参数3的功能；如果填写参数3的1024那么就会接收第一次数据
	DWORD dwRecvcount;
	BOOL bRes = AcceptEx(g_allSock[0], g_allSock[g_count], str, 0, sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16, &dwRecvcount, &g_Overlpd[0]);
	int a = WSAGetLastError();
	if (ERROR_IO_PENDING != a)
	{
		//稍后处理
		return 1;
	}
	return 0;
}

int PostRecv(int index)
{
	WSABUF wsabuf;
	wsabuf.buf = g_strRecv;
	wsabuf.len = MAX_RECV_COUNT;
	DWORD dwRecvCount;
	DWORD dwFlag = 0;
	int nRes = WSARecv(g_allSock[index], &wsabuf, 1, &dwRecvCount, &dwFlag, &g_Overlpd[index], NULL);
	int a = WSAGetLastError();
	if (ERROR_IO_PENDING != a)
	{
		//稍后处理
		return 1;
	}
	return a;
}

int PostSend(int index)
{
	WSABUF wsabuf;
	wsabuf.buf = "你好，我是服务器";
	wsabuf.len = MAX_RECV_COUNT;
	DWORD dwRecvCount;
	DWORD dwFlag = 0;
	int nRes = WSASend(g_allSock[index], &wsabuf, 1, &dwRecvCount, dwFlag, &g_Overlpd[index], NULL);
	int a = WSAGetLastError();
	if (ERROR_IO_PENDING != a)
	{
		//稍后处理
		return 1;
	}
	return a;
}

DWORD WINAPI serverThread(LPVOID lpParameter)
{
	HANDLE port = (HANDLE)lpParameter;
	DWORD dwNumberOfBytes;
	ULONG_PTR index;
	LPOVERLAPPED lpOverlapped;
	while (g_flag)
	{
		BOOL bFlag = GetQueuedCompletionStatus(port, &dwNumberOfBytes, &index, &lpOverlapped, INFINITE);
		if (!bFlag)
		{
			if (64 == GetLastError())
			{
				//close
				printf("*****client force closed*****\n");
				closesocket(g_allSock[index]);
				WSACloseEvent(g_Overlpd[index].hEvent);
				/*这里把socket从数组中删除不能像重叠IO那样直接跟数组最后一个元素交换位置，
				因为这个socket绑定完成端口的时候第三个参数就是socket的下标，如果直接交换位置那么最后的那个socket的下标就变了*/
				g_allSock[index] = 0;
				g_Overlpd[index].hEvent = NULL;
				continue;
			}
			else
			{
				printf("GetQueuedCompletionStatus error:%d\n", WSAGetLastError());
				continue;
			}
		}
		//处理
		//accept
		if (index == 0)
		{
			printf("accept\n");
			//绑定完成端口
			HANDLE hPort2 = CreateIoCompletionPort((HANDLE)g_allSock[g_count], hPort, g_count, 0);
			if (hPort != hPort2)
			{
				printf("绑定完成端口 error:%d,index:%d\n", WSAGetLastError(), g_count);
				closesocket(g_allSock[g_count]);
				continue;
			}
			PostSend(g_count);
			//投递recv
			PostRecv(g_count);
			g_count++;
			PostAccept();
		}
		else
		{
			if (dwNumberOfBytes == 0)
			{
				//close
				printf("*****client closed*****\n");
				closesocket(g_allSock[index]);
				WSACloseEvent(g_Overlpd[index].hEvent);
				/*这里把socket从数组中删除不能像重叠IO那样直接跟数组最后一个元素交换位置，
				因为这个socket绑定完成端口的时候第三个参数就是socket的下标，如果直接交换位置那么最后的那个socket的下标就变了*/
				g_allSock[index] = 0;
				g_Overlpd[index].hEvent = NULL;
			}
			else
			{
				if (g_strRecv[0] != 0)
				{
					//recv
					printf("WSARecv Data:%s\n", g_strRecv);
					//清空buff
					memset(g_strRecv, 0, MAX_RECV_COUNT);
					//对自己投递recv:因为每次调用WSARecv只接收一次信息，所以要重复调用
					PostRecv(index);
				}
				else
				{
					//send
					printf("Send Success\n");
				}
			}
		}
	}


	return 0;
}
