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
		//�ͷž��
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
	//�����ϵͳͶ��һ����������رմ���ʱ���ú���fun
	SetConsoleCtrlHandler(fun, TRUE);
	WSADATA wsaData;
	int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRes != 0)
	{
		//��������ʧ��
		switch (nRes)
		{
		case WSASYSNOTREADY:
			printf("��������,���߼�������");
			break;
		case WSAVERNOTSUPPORTED:
			printf("����������");
			break;
		case WSAEINPROGRESS:
			printf("������������");
			break;
		case WSAEPROCLIM:
			printf("�볢�Թرղ���Ҫ�����,Ϊ��ǰ���������ṩ������Դ");
			break;
		case WSAEFAULT:
			printf("�����ڶ�����������");
			break;
		}
		return 0;
	}
	//�汾У��
	if (2!=HIBYTE(wsaData.wVersion) || 2!=LOBYTE(wsaData.wVersion))
	{
		WSACleanup();
		return 0;
	}
	//����socket
	SOCKET serverSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == serverSocket)
	{
		printf("WSASocket error:%d", WSAGetLastError());
		WSACleanup();
		return 0;
	}
	//��
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
	//������ɶ˿�
	hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0); //������0��ʾ�Զ���ȡCPU�ĺ������Զ��������ʵ��߳���
	if (hPort == 0)
	{
		printf("������ɶ˿� error:%d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//����ɶ˿�
	HANDLE hPort2 = CreateIoCompletionPort((HANDLE)serverSocket, hPort, 0, 0);//������0�����������0���岻һ���������ʾ��ȫ���ԣ���ɶ����������
	if (hPort != hPort2)
	{
		printf("����ɶ˿� error:%d", WSAGetLastError());
		CloseHandle(hPort);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//����
	if (SOCKET_ERROR == listen(serverSocket, SOMAXCONN))
	{
		printf("listen error:%d\n", WSAGetLastError());
		CloseHandle(hPort);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	printf("******�ȴ��ͻ�������(��ɶ˿�)******\n");
	
	if (PostAccept()!=0)
	{
		printf("PostAccept error:%d\n", PostAccept());
		CloseHandle(hPort);
		Clear();
		return 0;
	}
	
	//��ȡ��Ҫ�����߳���
	SYSTEM_INFO systemProcessorsCount;
	GetSystemInfo(&systemProcessorsCount);
	nProcessorCount = systemProcessorsCount.dwNumberOfProcessors;
	//�����߳�
	pThread = (HANDLE*)malloc(sizeof(HANDLE)*nProcessorCount);
	for (int i=0;i<nProcessorCount;i++)
	{
		//����2��λΪ�ֽڣ���0:��ʾĬ���߳�ջ��С 1M��
		//����5��0:�������߳�����ִ��serverThread;CREATE_SUSPENDED:�������̹߳��𣬵���ResumeThread()��������
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
	//�������߳�
	while (1)
	{
		Sleep(1000);
	}
	//�ͷž��
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
	//����3���������ó�NULL�����ܣ��ͻ��˵�һ��send��������AcceptEx�������գ������str�ڣ��ڶ���send��������WSARecv����
	//������������ó�0��ôȡ���˲���3�Ĺ��ܣ������д����3��1024��ô�ͻ���յ�һ������
	DWORD dwRecvcount;
	BOOL bRes = AcceptEx(g_allSock[0], g_allSock[g_count], str, 0, sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16, &dwRecvcount, &g_Overlpd[0]);
	int a = WSAGetLastError();
	if (ERROR_IO_PENDING != a)
	{
		//�Ժ���
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
		//�Ժ���
		return 1;
	}
	return a;
}

int PostSend(int index)
{
	WSABUF wsabuf;
	wsabuf.buf = "��ã����Ƿ�����";
	wsabuf.len = MAX_RECV_COUNT;
	DWORD dwRecvCount;
	DWORD dwFlag = 0;
	int nRes = WSASend(g_allSock[index], &wsabuf, 1, &dwRecvCount, dwFlag, &g_Overlpd[index], NULL);
	int a = WSAGetLastError();
	if (ERROR_IO_PENDING != a)
	{
		//�Ժ���
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
				/*�����socket��������ɾ���������ص�IO����ֱ�Ӹ��������һ��Ԫ�ؽ���λ�ã�
				��Ϊ���socket����ɶ˿ڵ�ʱ���������������socket���±꣬���ֱ�ӽ���λ����ô�����Ǹ�socket���±�ͱ���*/
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
		//����
		//accept
		if (index == 0)
		{
			printf("accept\n");
			//����ɶ˿�
			HANDLE hPort2 = CreateIoCompletionPort((HANDLE)g_allSock[g_count], hPort, g_count, 0);
			if (hPort != hPort2)
			{
				printf("����ɶ˿� error:%d,index:%d\n", WSAGetLastError(), g_count);
				closesocket(g_allSock[g_count]);
				continue;
			}
			PostSend(g_count);
			//Ͷ��recv
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
				/*�����socket��������ɾ���������ص�IO����ֱ�Ӹ��������һ��Ԫ�ؽ���λ�ã�
				��Ϊ���socket����ɶ˿ڵ�ʱ���������������socket���±꣬���ֱ�ӽ���λ����ô�����Ǹ�socket���±�ͱ���*/
				g_allSock[index] = 0;
				g_Overlpd[index].hEvent = NULL;
			}
			else
			{
				if (g_strRecv[0] != 0)
				{
					//recv
					printf("WSARecv Data:%s\n", g_strRecv);
					//���buff
					memset(g_strRecv, 0, MAX_RECV_COUNT);
					//���Լ�Ͷ��recv:��Ϊÿ�ε���WSARecvֻ����һ����Ϣ������Ҫ�ظ�����
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
