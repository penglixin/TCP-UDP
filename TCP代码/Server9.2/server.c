#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib,"ws2_32.lib")


int main()
{
	WORD wdVersion = MAKEWORD(2, 2); //主版本号存在低8位，副版本号存在高8位
	//int a = *((char*)&wdVersion);
	//int b = *((char*)&wdVersion + 1);
	WSADATA wsaData;
	int nRes = WSAStartup(wdVersion,&wsaData);
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
	//创建socket
	SOCKET socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketServer == INVALID_SOCKET)
	{
		int errorCode = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//绑定socket
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
	//监听
	if (listen(socketServer, SOMAXCONN) == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		closesocket(socketServer);
		WSACleanup();
		return 0;
	}
	//创建客户端链接accept
	printf("等待客户端连接......\n");
	SOCKADDR_IN Clientaddr;
	int len = sizeof(Clientaddr);
	SOCKET socketClient = accept(socketServer,(SOCKADDR*)&Clientaddr, &len); //这是一个阻塞函数，如果没有客户端连接，那么程序一直等待客户端连接，不会往下执行
	if (socketClient == INVALID_SOCKET)
	{
		printf("客户端连接失败！！！\n");
		int errorCode = WSAGetLastError();
		closesocket(socketServer);
		WSACleanup();
		return 0;
	}
	printf("客户端连接成功！\n");

	if (send(socketClient, "我是服务器,收到了你的连接\n", sizeof("我是服务器,收到了你的连接"), 0) == SOCKET_ERROR)
	{
		int errorcode = WSAGetLastError();
	}
	while (1)
	{
		//接收消息
		char buf[1500] = { 0 };
		int res = recv(socketClient, buf, 1499, 0);
		if (res == 0)  //这也是一个阻塞函数，当客户端没有send消息的时候会一直阻塞在这行代码，当客户端掉线则返回0;正常接收则返回接收到的字符数组的长度；执行失败返回SOCKET_ERROR;
		{
			printf("客户端连接中断。。。");
		}
		else if (res == SOCKET_ERROR)
		{
			int errorcode = WSAGetLastError();
			printf("server接收出错。。。");
		}
		else
		{
			printf("消息长度：%d，消息内容：%s\n", res, buf);
		}
		//发送消息
		scanf("%s", buf);
		if (send(socketClient,buf, strlen(buf), 0) == SOCKET_ERROR)  //执行成功返回值>0，发送字符的长度；失败返回SOCKET_ERROR
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