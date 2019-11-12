#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#pragma comment (lib,"ws2_32.lib")


int main()
{
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
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion)) //HIBYTE�Ǹ�8λ�����汾�ţ�LOBYTE�ǵ�8λ�����汾��
	{
		WSACleanup();
		return 0;
	}
	//���������socket
	SOCKET socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketServer == INVALID_SOCKET)
	{
		int errorcode = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//���ӷ����
	struct sockaddr_in Serveraddr;
	Serveraddr.sin_family = AF_INET;
	Serveraddr.sin_addr.S_un.S_addr = inet_addr("10.100.145.239");
	Serveraddr.sin_port = htons(12345);
	if (connect(socketServer, (const struct sockaddr*)&Serveraddr, sizeof(Serveraddr)) == SOCKET_ERROR) //���ӷ��������Ұѷ�������Ϣ�������socket�󶨵�һ��
	{
		int errorcode = WSAGetLastError();
		closesocket(socketServer);
		WSACleanup();
		return 0;
	}
	printf("���ӳɹ�\n");
	char buf[1024] = { 0 };
	recv(socketServer, buf, 1023, 0);
	printf("recv data : %s\n", buf);
	while (1)
	{
		////����Ϣ
		char buf[1500] = { 0 };
		//int res = recv(socketServer, buf, 1499, 0);
		//if (res == 0)
		//{
		//	printf("�ͻ��������жϡ�����");
		//}
		//else if (res == SOCKET_ERROR)
		//{
		//	int errorcode = WSAGetLastError();
		//	printf("client���ճ�������");
		//}
		//else
		//{
		//	printf("��Ϣ���ȣ�%d�����ݣ�%s\n", res, buf);
		//}
		//����Ϣ
		scanf("%s", buf);
		if (buf[0] == 'E' || buf[0] == 'Q')
		{
			break;
		}
		if (send(socketServer, buf, strlen(buf), 0) == SOCKET_ERROR)
		{
			int errorcode = WSAGetLastError();
		}
	}

	closesocket(socketServer);
	WSACleanup();
	system("pause");
	return 0;
}