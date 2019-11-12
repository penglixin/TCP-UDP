#define _WINSOCK_DEPRECATED_NO_WARNINGS 

#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib,"ws2_32.lib")


//�ͻ���socket����
fd_set allSockets;

BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{

	case CTRL_CLOSE_EVENT:
		//�ͷ�����socket
		for (u_int i = 0;i < allSockets.fd_count;i++)
		{
			closesocket(allSockets.fd_array[i]);
		}
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
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
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
		int errorCode = WSAGetLastError();
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//����
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	
	//����
	FD_ZERO(&allSockets);
	//�򼯺������һ��������socket
	FD_SET(serverSocket, &allSockets);
	
	while (1)
	{
		fd_set readSockets = allSockets;
		fd_set writeSockets = allSockets;   //����Ƿ��п�д�Ŀͻ��ˣ����ؿ��Ա�send��Ϣ�Ŀͻ��ˣ�ֻҪ���ӽ����ɹ���ô�ÿͻ���socket���ǿ�д��
		FD_CLR(serverSocket, &writeSockets);
		fd_set errorSockets = allSockets;

		struct timeval st;
		st.tv_sec = 3;
		st.tv_usec = 0;
		int nRes = select(0, &readSockets, &writeSockets, &errorSockets, &st);
		if (nRes == 0)
		{
			continue;
		}
		else if (nRes > 0)
		{
			//����Ӧ
			//�������
			for (u_int i=0;i<errorSockets.fd_count;i++) 
			{
				char str[100] = { 0 };
				int len = 99;
				if (SOCKET_ERROR == getsockopt(errorSockets.fd_array[i], SOL_SOCKET, SO_ERROR, str, &len))
				{
					printf("�޷��õ�������Ϣ��");
				}
				printf("%s\n", str);
			}
			//�п�д��socket
			for (u_int i = 0;i < writeSockets.fd_count;i++)
			{
				if (send(writeSockets.fd_array[i], "���Ƿ�����������", sizeof("���Ƿ�����������"), 0) == SOCKET_ERROR)
				{
					WSAGetLastError();
				}
			}
			//recv accept
			for (u_int i = 0;i < readSockets.fd_count;i++)
			{
				if (readSockets.fd_array[i] == serverSocket)  //������socket����Ӧ  accept
				{
					SOCKET clientSocket = accept(serverSocket, NULL, NULL);
					if (clientSocket == INVALID_SOCKET)
					{
						continue;
					}
					//��ӽ�������
					FD_SET(clientSocket, &allSockets);
					printf("# %d �ſͻ������ӳɹ���\n",allSockets.fd_count-1);
				}
				else  //�ͻ���socket����Ӧ  recv
				{
					char buf[1500] = { 0 };
					int nRecv = recv(readSockets.fd_array[i], buf, 1500, 0);
					if (nRecv == 0)
					{
						//�ͻ�������,�Ѹÿͻ���socket��������ɾ��
						SOCKET tempsocket = readSockets.fd_array[i];
						FD_CLR(readSockets.fd_array[i], &allSockets);
						//�ͷ�
						closesocket(tempsocket);
					}
					else if (nRecv > 0)
					{
						//�յ���Ϣ
						printf("�յ���Ϣ:%s\n",buf);

					}
					else //SOCK_ERROR ǿ������(ֱ�ӹرմ���)Ҳ��ִ����δ���
					{
						//���ճ���
						int a = WSAGetLastError();
						if (a == 10054)
						{
							//�ͻ�������,�Ѹÿͻ���socket��������ɾ��
							SOCKET tempsocket = readSockets.fd_array[i];
							FD_CLR(readSockets.fd_array[i], &allSockets);
							//�ͷ�
							closesocket(tempsocket);
						}
					}
				}
			}
		}
		else
		{
			//��������
			WSAGetLastError();
			break;
		}
	}


	////������ɾ��ָ��socket��ɾ����һ��Ҫclosesocket������
	//FD_CLR(serverSocket, &clientSocket);
	////�ж�һ��socket�Ƿ��ڼ����С����ڷ��ط��㣬�����ڷ�����
	//FD_ISSET(serverSocket, &clientSocket);

	//�ͷ�����socket
	for (u_int i = 0;i < allSockets.fd_count;i++)
	{
		closesocket(allSockets.fd_array[i]);
	}
	WSACleanup();
	return 0;
}