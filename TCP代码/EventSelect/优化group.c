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
//		//��������ʧ��
//		switch (nRes)
//		{
//		case WSASYSNOTREADY:
//			printf("��������,���߼�������");
//			break;
//		case WSAVERNOTSUPPORTED:
//			printf("����������");
//			break;
//		case WSAEINPROGRESS:
//			printf("������������");
//			break;
//		case WSAEPROCLIM:
//			printf("�볢�Թرղ���Ҫ�����,Ϊ��ǰ���������ṩ������Դ");
//			break;
//		case WSAEFAULT:
//			printf("�����ڶ�����������");
//			break;
//		}
//		return 0;
//	}
//	//�汾У��
//	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
//	{
//		WSACleanup();
//		return 0;
//	}
//	//����socket
//	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if (serverSocket == INVALID_SOCKET)
//	{
//		//��ȡ������
//		int errorCode = WSAGetLastError();
//		WSACleanup();
//		return 0;
//	}
//	//��socket
//	SOCKADDR_IN serverAddr;
//	serverAddr.sin_family = AF_INET;
//	serverAddr.sin_addr.S_un.S_addr = inet_addr("192.168.137.1");
//	//serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
//	serverAddr.sin_port = htons(12345);
//	if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
//	{
//		//��ȡ������
//		int errorCode = WSAGetLastError();
//		closesocket(serverSocket);
//		WSACleanup();
//		return 0;
//	}
//	//����
//	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
//	{
//		/*��ȡ������*/
//		int errorCode = WSAGetLastError();
//		closesocket(serverSocket);
//		WSACleanup();
//		return 0;
//	}
//	printf("�ȴ��ͻ�������(�Ż�eventselect)......\n");
//
//	struct fd_es_set esSet[20] = { 0,{0},{NULL} };
//	memset(esSet, 0, sizeof(struct fd_es_set) * 20);
//
//
//	//�����¼�
//	WSAEVENT serverEvent = WSACreateEvent();
//	if (serverEvent == WSA_INVALID_EVENT)
//	{
//		printf("�����¼�ʧ�ܣ�%d\n", WSAGetLastError());
//		closesocket(serverSocket);
//		WSACleanup();
//		return 0;
//	}
//	//���¼�����
//	if (WSAEventSelect(serverSocket, serverEvent, FD_ACCEPT) == SOCKET_ERROR)
//	{
//		printf("���¼�ʧ�ܣ�%d\n", WSAGetLastError());
//		WSACloseEvent(serverEvent);
//		closesocket(serverSocket);
//		WSACleanup();
//		return 0;
//	}
//	//�󶨳ɹ�����socket���¼���ӽ�����
//	esSet[0].allSockets[esSet[0].count] = serverSocket;
//	esSet[0].allEvents[esSet[0].count] = serverEvent;
//	esSet[0].count++;
//	while (1)
//	{
//		for (int j = 0;j < 20;j++)
//		{
//			if (esSet[j].count == 0)
//			{
//				continue;
//			}
//			DWORD nRes = WSAWaitForMultipleEvents(esSet[j].count, esSet[j].allEvents, FALSE, 0, FALSE);
//			if (nRes == WSA_WAIT_FAILED)
//			{
//				printf("�����룺%d\n", WSAGetLastError());
//				break;
//			}
//			if (WSA_WAIT_TIMEOUT == nRes)
//			{
//				continue;
//			}
//			DWORD nIndex = nRes - WSA_WAIT_EVENT_0;
//
//			for (int i = nIndex;i < esSet[j].count;i++)
//			{
//				//��ȡ�����źŵ��¼�
//				//����4�����룩: 0������¼�����״̬���������ز�����û���źţ�WSA_INFINITE��һֱ�ȴ�ֱ�����¼�����
//				DWORD nRes = WSAWaitForMultipleEvents(1, &esSet[j].allEvents[i], FALSE, 0, FALSE);
//				if (nRes == WSA_WAIT_FAILED)
//				{
//					printf("WSAWaitForMultipleEvents�����룺%d\n", WSAGetLastError());
//					continue;
//				}
//				if (nRes == WSA_WAIT_TIMEOUT)
//				{
//					continue;
//				}
//				//��ȡ�¼����ͣ�accept��recv��send��close�ȣ��������¼��ϵ��ź�����
//				WSANETWORKEVENTS networkEvent;
//				if (SOCKET_ERROR == WSAEnumNetworkEvents(esSet[j].allSockets[i], esSet[j].allEvents[i], &networkEvent))
//				{
//					printf("WSAEnumNetworkEvents�����룺%d\n", WSAGetLastError());
//					break;
//				}
//				//����accept�¼�
//				if (networkEvent.lNetworkEvents & FD_ACCEPT)
//				{
//					//accept�¼�����
//					if (networkEvent.iErrorCode[FD_ACCEPT_BIT] == 0)
//					{
//						SOCKET clientSocket = accept(esSet[j].allSockets[i], NULL, NULL);
//						if (INVALID_SOCKET == clientSocket)
//						{
//							continue;
//						}
//						//Ϊ��socket�������¼�
//						WSAEVENT clientEvent = WSACreateEvent();
//						if (clientEvent == WSA_INVALID_EVENT)
//						{
//							int errorCode = WSAGetLastError();
//							closesocket(clientSocket);
//							continue;
//						}
//						//���¼�����--�ѿͻ����¼�Ͷ�ݸ�ϵͳ����
//						if (SOCKET_ERROR == WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_WRITE | FD_CLOSE))
//						{
//							int errorCode = WSAGetLastError();
//							WSACloseEvent(clientEvent);
//							closesocket(clientSocket);
//							continue;
//						}
//						for (int m=0;m<20;m++)
//						{
//							if (esSet[m].count<64)
//							{
//								esSet[m].allSockets[esSet[m].count] = clientSocket;
//								esSet[m].allEvents[esSet[m].count] = clientEvent;
//								esSet[m].count++;
//								break;
//							}
//						}
//						printf("accept event\n");
//					}
//					else
//					{
//						printf("socket accept error,error code:%d\n", networkEvent.iErrorCode[FD_ACCEPT_BIT]);
//						continue;
//					}
//				}
//				//����write�¼�
//				if (networkEvent.lNetworkEvents & FD_WRITE)
//				{
//					if (networkEvent.iErrorCode[FD_WRITE_BIT] == 0)
//					{
//						if (SOCKET_ERROR == send(esSet[j].allSockets[i], "connect success", strlen("connect success"), 0))
//						{
//							printf("send failed,error code:%d", WSAGetLastError());
//							continue;
//						}
//						printf("write event\n");
//					}
//					else
//					{
//						printf("socket write error,error code:%d\n", networkEvent.iErrorCode[FD_WRITE_BIT]);
//						continue;
//					}
//				}
//				//����read�¼�
//				if (networkEvent.lNetworkEvents & FD_READ)
//				{
//					if (networkEvent.iErrorCode[FD_READ_BIT] == 0)
//					{
//						char recvbuf[1500] = { 0 };
//						if (recv(esSet[j].allSockets[i], recvbuf, 1499, 0) == SOCKET_ERROR)
//						{
//							printf("recv failed,error code:%d", WSAGetLastError());
//							continue;
//						}
//						printf("recv data:%s\n", recvbuf);
//					}
//					else
//					{
//						printf("socket read error,error code:%d\n", networkEvent.iErrorCode[FD_READ_BIT]);
//						continue;
//					}
//
//				}
//				//����close�¼�
//				if (networkEvent.lNetworkEvents & FD_CLOSE)
//				{
//					printf("*******client close*******\n");
//					//����ͻ����׽��֣��¼�
//					closesocket(esSet[j].allSockets[i]);
//					esSet[j].allSockets[i] = esSet[j].allSockets[esSet[j].count - 1];
//					WSACloseEvent(esSet[j].allEvents[i]);
//					esSet[j].allEvents[i] = esSet[j].allEvents[esSet[j].count - 1];
//					esSet[j].count--;
//					//printf("socket close error,error code:%d\n", networkEvent.iErrorCode[FD_CLOSE_BIT]);
//				}
//			}
//		}
//		
//	}
//
//
//	WSACloseEvent(serverEvent);
//	closesocket(serverSocket);
//	WSACleanup();
//	return 0;
//}