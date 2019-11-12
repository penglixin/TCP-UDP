#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <stdio.h>
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
	WSADATA wsaData;
	int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nRes != 0)
	{
		printf("������ʧ�ܣ������룺%d\n", WSAGetLastError());
		system("pause");
		return 0;
	}
	if (2!=HIBYTE(wsaData.wVersion) || 2!=LOBYTE(wsaData.wVersion))
	{
		printf("�汾У��ʧ��\n");
		WSACleanup();
		system("pause");
		return 0;
	}
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == serverSocket)
	{
		printf("socket error:%d\n",WSAGetLastError());
		WSACleanup();
		system("pause");
		return 0;
	}
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("10.100.151.6");
	serverAddr.sin_port = htons(12345);
	if (SOCKET_ERROR == bind(serverSocket,(SOCKADDR*)&serverAddr,sizeof(serverAddr)))
	{
		printf("bind error:%d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		system("pause");
		return 0;
	}
	while (1)
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(serverSocket, &fds);
		struct timeval tvl;
		tvl.tv_sec = 3;
		tvl.tv_usec = 0;
		int nRes = select(0, &fds, NULL, NULL, &tvl);
		if (nRes == 0)
		{
			continue;
		}
		else if (nRes > 0)
		{
			//������Ϣ
			char strbuf[548] = { 0 };
			SOCKADDR_IN clientAddr;
			int fromlen = sizeof(serverAddr);
			int num = recvfrom(serverSocket, strbuf, sizeof(strbuf), 0, (struct sockaddr*)&clientAddr, &fromlen);
			if (num == SOCKET_ERROR)
			{
				printf("recvfrom error:%d\n", WSAGetLastError());
			}
			printf("recvfrom data:%s\n", strbuf);
			//�յ���Ϣ����ͻ���һ���ظ�
			num = sendto(serverSocket, "��������յ�", sizeof("��������յ�"), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
			if (num == SOCKET_ERROR)
			{
				printf("sendto error:%d\n", WSAGetLastError());
			}
		}
		else
		{
			printf("select error :%d\n", WSAGetLastError());
			break;
		}
	}

	closesocket(serverSocket);
	WSACleanup();
	return 0;
}