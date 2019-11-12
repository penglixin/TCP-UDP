#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <Stdio.h>
#include <stdlib.h>

#pragma comment(lib,"ws2_32.lib")

SOCKET serverSocket;

BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
		closesocket(serverSocket);
		WSACleanup();
		break;
	}
	return TRUE;
}

int main(void)
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
	if (2!=HIBYTE(wsaData.wVersion) || 2!=LOBYTE(wsaData.wVersion))
	{
		//�������ʧ��
		printf("�汾У��ʧ��\n");
		WSACleanup();
		system("pause");
		return 0;
	}
	//����socket
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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
	serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	serverAddr.sin_port = htons(12345);
	if( SOCKET_ERROR == bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
	{
		printf("��socketʧ�ܣ������룺%d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		system("pause");
		return 0;
	}
	while (1)
	{
		char buf[548] = { 0 };
		struct sockaddr clientAddr;
		//SOCKADDR_IN clientAddr;
		int fromlen = sizeof(serverAddr);
		int num = recvfrom(serverSocket, buf, sizeof(buf), 0, &clientAddr, &fromlen);
		if (SOCKET_ERROR == num )
		{
			printf("recvfrom  error:%d\n", WSAGetLastError());
		}
		printf("recv data:%s\n", buf);
		num = sendto(serverSocket, "this is Server", sizeof("this is Server"), 0, &clientAddr, sizeof(clientAddr));
		if (num == SOCKET_ERROR)
		{
			printf("sendto  error:%d\n", WSAGetLastError());
		}
	}



	WSACleanup();
	system("pause");
	return 0;
}