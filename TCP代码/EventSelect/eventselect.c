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
	//�������
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
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		//��ȡ������
		int errorCode = WSAGetLastError();
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
		//��ȡ������
		int errorCode = WSAGetLastError();
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	/*����*/
	if (SOCKET_ERROR == listen(serverSocket, SOMAXCONN))
	{
		/*��ȡ������*/
		int errorCode = WSAGetLastError();
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	printf("�ȴ��ͻ������ӡ�����������\n");
	

	//�����¼�
	WSAEVENT serverEvent = WSACreateEvent();
	if (serverEvent == WSA_INVALID_EVENT)
	{
		int errorCode = WSAGetLastError();
		printf("�����¼�ʧ�ܣ�%d\n", errorCode);
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//���¼�����
	if (WSAEventSelect(serverSocket, serverEvent, FD_ACCEPT) == SOCKET_ERROR)
	{
		printf("���¼�ʧ�ܣ�%d\n", WSAGetLastError());
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
		//��ȡ�����źŵ��¼�
		//����4�����룩: 0������¼�����״̬���������ز�����û���źţ�WSA_INFINITE��һֱ�ȴ�ֱ�����¼�����
		DWORD nRes = WSAWaitForMultipleEvents(esSet.count, esSet.allEvents, FALSE, WSA_INFINITE, FALSE);
		if (nRes == WSA_WAIT_FAILED)  //��������������˾���ȴ�ʱ�䣬���ҳ�ʱ��nIndex == WSA_WAIT_TIMEOUT,continue;����
		{
			printf("�����룺%d\n", WSAGetLastError());
			break;
		}
		DWORD nIndex = nRes - WSA_WAIT_EVENT_0;
		//��ȡ�¼����ͣ�accept��recv��send��close�ȣ��������¼��ϵ��ź�����
		WSANETWORKEVENTS networkEvent;
		if (WSAEnumNetworkEvents(esSet.allSockets[nIndex], esSet.allEvents[nIndex], &networkEvent) == SOCKET_ERROR)
		{
			printf("�����룺%d\n", WSAGetLastError());
			break;
		}
		//����accept�¼�
		if (networkEvent.lNetworkEvents & FD_ACCEPT) 
		{
			if (networkEvent.iErrorCode[FD_ACCEPT_BIT] == 0)  //�¼�accept����
			{
				//����
				SOCKET clientSocket = accept(esSet.allSockets[nIndex], NULL, NULL);
				if(INVALID_SOCKET == clientSocket)
				{
					continue;
				}
				//Ϊ��socket�����¼�����
				WSAEVENT clientEvent = WSACreateEvent();
				if (clientEvent == WSA_INVALID_EVENT)
				{
					int errorCode = WSAGetLastError();
					closesocket(clientSocket);
					continue;
				}
				//���¼�����--�ѿͻ����¼�Ͷ�ݸ�ϵͳ����
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
		//����write�¼�  send
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
		//����read�¼� recv
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
		//����close�¼� �ͻ��˶Ͽ�����
		if (networkEvent.lNetworkEvents & FD_CLOSE)
		{
			printf("*******client close*******\n");
			//����ͻ����׽��֣��¼�
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
		//�ͷ��¼�����ռ�ں��ڴ棩
		closesocket(esSet.allSockets[i]);
		WSACloseEvent(esSet.allEvents[i]);
	}
	WSACleanup();
	return 0;
}