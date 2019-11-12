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
		//网络库加载失败
		switch (nRes)
		{
		case WSASYSNOTREADY:
			printf("重启电脑,或者检查网络库");
			break;
		case WSAVERNOTSUPPORTED:
			printf("请更新网络库");
			break;
		case WSAEINPROGRESS:
			printf("重新启动程序");
			break;
		case WSAEPROCLIM:
			printf("请尝试关闭不必要的软件,为当前程序运行提供充足资源");
			break;
		case WSAEFAULT:
			printf("函数第二个参数错误");
			break;
		}

		return 0;
	}
	//版本校验
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion)) //HIBYTE是高8位，副版本号；LOBYTE是低8位，主版本号
	{
		WSACleanup();
		return 0;
	}
	//创建服务端socket
	SOCKET socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketServer == INVALID_SOCKET)
	{
		int errorcode = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//连接服务端
	struct sockaddr_in Serveraddr;
	Serveraddr.sin_family = AF_INET;
	Serveraddr.sin_addr.S_un.S_addr = inet_addr("10.100.145.239");
	Serveraddr.sin_port = htons(12345);
	if (connect(socketServer, (const struct sockaddr*)&Serveraddr, sizeof(Serveraddr)) == SOCKET_ERROR) //连接服务器并且把服务器信息与服务器socket绑定到一起
	{
		int errorcode = WSAGetLastError();
		closesocket(socketServer);
		WSACleanup();
		return 0;
	}
	printf("连接成功\n");
	char buf[1024] = { 0 };
	recv(socketServer, buf, 1023, 0);
	printf("recv data : %s\n", buf);
	while (1)
	{
		////收消息
		char buf[1500] = { 0 };
		//int res = recv(socketServer, buf, 1499, 0);
		//if (res == 0)
		//{
		//	printf("客户端连接中断。。。");
		//}
		//else if (res == SOCKET_ERROR)
		//{
		//	int errorcode = WSAGetLastError();
		//	printf("client接收出错。。。");
		//}
		//else
		//{
		//	printf("消息长度：%d，内容：%s\n", res, buf);
		//}
		//发消息
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