#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <MSWSock.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Mswsock.lib")

#define MAX_COUNT 1024
#define MAX_RECV_COUNT 1024
SOCKET g_allSock[MAX_COUNT];
OVERLAPPED g_Overlpd[MAX_COUNT];
int g_count;
char g_strRecv[MAX_RECV_COUNT];

int PostAccept();
int PostRecv(int index);
int PostSend(int index);

void Clear()
{
	for (int i = 0;i < g_count;i++)
	{
		closesocket(g_allSock[i]);
		WSACloseEvent(g_Overlpd[i].hEvent);
	}
}


int main()
{
	WSADATA wsaData;
	int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRes!=0)
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
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
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
	//��socket
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("10.100.145.239");
	serverAddr.sin_port = htons(12345);
	if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		printf("bind error:%d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//����
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("listen error:%d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	printf("******�ȴ��ͻ�������(�������)******\n");
	g_allSock[g_count] = serverSocket;
	g_Overlpd[g_count].hEvent = WSACreateEvent();
	g_count++;
	//�����ͻ�������AcceptEx
	if (PostAccept() != 0)
	{
		printf("PostAccept error:%d\n", PostAccept());
		Clear();
		WSACleanup();
		return 0;
	}

	while (1)
	{
		/*����6�ĳ�TRUE��ֻ�����ص�IO����������ģ�ͣ�:
		ʵ�ֵȴ��¼�������������̺������첽ִ�У���������̺���ִ�������ȴ��¼�����һ���ź� WSA_WAIT_IO_COMPLETION */
		int nRes = WSAWaitForMultipleEvents(1, &(g_Overlpd[0].hEvent), FALSE, WSA_INFINITE, TRUE);
		if (nRes == WSA_WAIT_FAILED || nRes == WSA_WAIT_IO_COMPLETION)
		{
			continue;
		}
		//�����ź�
		WSAResetEvent(g_Overlpd[0].hEvent);
		PostSend(g_count);
		printf("******accept******\n");
		//��������
		//Ͷ��recv
		PostRecv(g_count);
		g_count++;
		//Ͷ��Accept
		PostAccept();
	}
	Clear();
	WSACleanup();
	return 0;
}

int PostAccept()
{
	g_allSock[g_count] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_Overlpd[g_count].hEvent = WSACreateEvent();
	char str[1024] = { 0 };  //����3���������ó�NULL�����ܣ��ͻ��˵�һ��send��������AcceptEx�������գ������str�ڣ��ڶ���send��������WSARecv����
	//������������ó�0��ôȡ���˲���3�Ĺ��ܣ������д����3��1024��ô�ͻ���յ�һ������
	DWORD dwRecvCount;
	if (AcceptEx(g_allSock[0], g_allSock[g_count], str, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwRecvCount, &g_Overlpd[0]) == TRUE)
	{
		//����Ͷ��
		//recv
		PostRecv(g_count);
		g_count++;
		//Ͷ��accept
		PostAccept();
		return 0;
	}
	else
	{
		int a = WSAGetLastError();
		if (ERROR_IO_PENDING == a)
		{
			//�Ժ���
			return 0;
		}
		else
		{
			return a;
		}
	}
}

//�ص����������첽�¼����֮��Ż�ȥ����
void CALLBACK RecvCallBck(DWORD dwError,DWORD cbTransferred,LPWSAOVERLAPPED lpOverlapped,DWORD dwFlags)
{
	int i = 0;
	for (i;i<g_count;i++)
	{
		if (lpOverlapped->hEvent == g_Overlpd[i].hEvent)
		{
			break;
		}
	}
	/*int i = lpOverlapped - &g_Overlpd[0];*/

	if (dwError == 10054 || 0 == cbTransferred)
	{
		//ɾ���ͻ���
		printf("******client closed******\n");
		closesocket(g_allSock[i]);
		WSACloseEvent(g_Overlpd[i].hEvent);
		g_allSock[i] = g_allSock[g_count - 1];
		g_Overlpd[i] = g_Overlpd[g_count - 1];
		g_count--;
	}
	else
	{
		printf("WSARecv Data:%s\n", g_strRecv);
		//���buff
		memset(g_strRecv, 0, MAX_RECV_COUNT);
		//���Լ�Ͷ��recv:��Ϊÿ�ε���WSARecvֻ����һ����Ϣ������Ҫ�ظ�����
		PostRecv(i);
	}
}

int PostRecv(int index)
{
	WSABUF wsabuf;
	wsabuf.buf = g_strRecv;
	wsabuf.len = MAX_RECV_COUNT;

	DWORD dwRecvCount;
	DWORD dwFlag = 0;
	int nRes = WSARecv(g_allSock[index], &wsabuf, 1, &dwRecvCount, &dwFlag, &g_Overlpd[index], RecvCallBck);
	if (nRes == 0)
	{
		//����ִ��
		printf("WSARecv Data:%s\n", wsabuf.buf);
		//���buff
		memset(g_strRecv, 0, MAX_RECV_COUNT);
		PostRecv(index);
		return 0;
	}
	else
	{
		int a = WSAGetLastError();
		if (ERROR_IO_PENDING == a)
		{
			//�Ժ���
			return 0;
		}
		else
		{
			return a;
		}
	}
}

void CALLBACK SendCallBck(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	printf("send over\n");
}

int PostSend(int index)
{
	WSABUF wsabuf;
	wsabuf.buf = "��ã������Ƿ�����";
	wsabuf.len = MAX_RECV_COUNT;

	DWORD dwSendCount;
	DWORD dwFlag = 0;
	int nRes = WSASend(g_allSock[index], &wsabuf, 1, &dwSendCount, dwFlag, &g_Overlpd[index], SendCallBck);
	if (nRes == 0)
	{
		//����ִ��
		printf("WSASend Success\n");
		return 0;
	}
	else
	{
		int a = WSAGetLastError();
		if (ERROR_IO_PENDING == a)
		{
			//�Ժ���
			return 0;
		}
		else
		{
			return a;
		}
	}
}