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
//����ϵͳ�������߳���
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
		//�ر���ɶ˿�
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
	//�������
	WSADATA wsaData;
	int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRes != 0)
	{
		printf("������ʧ�ܣ������룺%d\n", WSAGetLastError());
		system("pause");
		return 0;
	}
	//�汾У��
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		//�������ʧ��
		printf("�汾У��ʧ��\n");
		WSACleanup();
		system("pause");
		return 0;
	}
	//����socket
	serverSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (serverSocket == INVALID_SOCKET)
	{
		printf("����socketʧ�ܣ������룺%d\n", WSAGetLastError());
		WSACleanup();
		system("pause");
		return 0;
	}
	//��
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("192.168.137.110");
	serverAddr.sin_port = htons(12345);
	if (SOCKET_ERROR == bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
	{
		printf("��socketʧ�ܣ������룺%d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		system("pause");
		return 0;
	}
	
	//�����¼�
	serverLap.hEvent = WSACreateEvent();
	if (serverLap.hEvent == WSA_INVALID_EVENT)
	{
		int errorCode = WSAGetLastError();
		printf("WSACreateEvent error : %d\n", errorCode);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//������ɶ˿�
	hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hPort == 0)
	{
		printf("������ɶ˿�ʧ��:%d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//����ɶ˿�
	hPort = CreateIoCompletionPort((HANDLE)serverSocket, hPort, serverSocket, 0);
	if (hPort == 0)
	{
		printf("����ɶ˿�ʧ��:%d", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//��ȡ��Ҫ�����߳���
	SYSTEM_INFO systemProcessCount;
	GetSystemInfo(&systemProcessCount);
	nProcesscount = systemProcessCount.dwNumberOfProcessors;
	/*�����߳�
	����2��λΪ�ֽڣ���0:��ʾĬ���߳�ջ��С 1M��
	����5��0:�������߳�����ִ��serverThread;CREATE_SUSPENDED:�������̹߳��𣬵���ResumeThread()��������*/
	for (int i = 0;i < nProcesscount;i++)
	{
		DWORD ThreadID;
		if (NULL == CreateThread(NULL, 0, ServerThread, hPort, 0, &ThreadID))
		{
			int a = WSAGetLastError();
			printf("�����߳�ʧ��:%d\n", a);
			i--;
		}
	}
	printf("******(UDP)��ɶ˿�******\n");
	//Ͷ��recvfrom�¼�
	if (0 == PostRecvFrom())
	{
		int errorCode = WSAGetLastError();
		printf("PostRecvFrom error : %d\n", errorCode);
		//�ر���ɶ˿�
		CloseHandle(hPort);
		WSACloseEvent(serverLap.hEvent);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//���߳�����
	while (1)
	{
		Sleep(1000);
	}
	g_flag = FALSE;
	//�ر���ɶ˿�
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
		//�첽
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
		//���¼��ź�����
		WSAResetEvent(serverLap.hEvent);
		//�ɹ�
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
