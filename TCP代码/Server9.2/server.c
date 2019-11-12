#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib,"ws2_32.lib")


int main()
{
	WORD wdVersion = MAKEWORD(2, 2); //���汾�Ŵ��ڵ�8λ�����汾�Ŵ��ڸ�8λ
	//int a = *((char*)&wdVersion);
	//int b = *((char*)&wdVersion + 1);
	WSADATA wsaData;
	int nRes = WSAStartup(wdVersion,&wsaData);
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
	//����socket
	SOCKET socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketServer == INVALID_SOCKET)
	{
		int errorCode = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//��socket
	struct sockaddr_in Serveraddr;
	Serveraddr.sin_family = AF_INET;
	Serveraddr.sin_addr.S_un.S_addr = inet_addr("10.100.145.239");
	Serveraddr.sin_port = htons(12345);
	if (bind(socketServer, (const struct sockaddr*)&Serveraddr, sizeof(Serveraddr)) == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		closesocket(socketServer);
		WSACleanup();
		return 0;
	}
	//����
	if (listen(socketServer, SOMAXCONN) == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		closesocket(socketServer);
		WSACleanup();
		return 0;
	}
	//�����ͻ�������accept
	printf("�ȴ��ͻ�������......\n");
	SOCKADDR_IN Clientaddr;
	int len = sizeof(Clientaddr);
	SOCKET socketClient = accept(socketServer,(SOCKADDR*)&Clientaddr, &len); //����һ���������������û�пͻ������ӣ���ô����һֱ�ȴ��ͻ������ӣ���������ִ��
	if (socketClient == INVALID_SOCKET)
	{
		printf("�ͻ�������ʧ�ܣ�����\n");
		int errorCode = WSAGetLastError();
		closesocket(socketServer);
		WSACleanup();
		return 0;
	}
	printf("�ͻ������ӳɹ���\n");

	if (send(socketClient, "���Ƿ�����,�յ����������\n", sizeof("���Ƿ�����,�յ����������"), 0) == SOCKET_ERROR)
	{
		int errorcode = WSAGetLastError();
	}
	while (1)
	{
		//������Ϣ
		char buf[1500] = { 0 };
		int res = recv(socketClient, buf, 1499, 0);
		if (res == 0)  //��Ҳ��һ���������������ͻ���û��send��Ϣ��ʱ���һֱ���������д��룬���ͻ��˵����򷵻�0;���������򷵻ؽ��յ����ַ�����ĳ��ȣ�ִ��ʧ�ܷ���SOCKET_ERROR;
		{
			printf("�ͻ��������жϡ�����");
		}
		else if (res == SOCKET_ERROR)
		{
			int errorcode = WSAGetLastError();
			printf("server���ճ�������");
		}
		else
		{
			printf("��Ϣ���ȣ�%d����Ϣ���ݣ�%s\n", res, buf);
		}
		//������Ϣ
		scanf("%s", buf);
		if (send(socketClient,buf, strlen(buf), 0) == SOCKET_ERROR)  //ִ�гɹ�����ֵ>0�������ַ��ĳ��ȣ�ʧ�ܷ���SOCKET_ERROR
		{
			int errorcode = WSAGetLastError();
		}
	}


	closesocket(socketServer);
	closesocket(socketClient);
	WSACleanup();
	system("pause");
	return 0;
}