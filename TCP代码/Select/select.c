#define _WINSOCK_DEPRECATED_NO_WARNINGS 

#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib,"ws2_32.lib")


//客户端socket数组
fd_set allSockets;

BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{

	case CTRL_CLOSE_EVENT:
		//释放所有socket
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
	//打开网络库
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
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		WSACleanup();
		return 0;
	}
	//创建socket
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		int errorCode = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//绑定socket
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
	//监听
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		int errorCode = WSAGetLastError();
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	
	//清零
	FD_ZERO(&allSockets);
	//向集合中添加一个服务器socket
	FD_SET(serverSocket, &allSockets);
	
	while (1)
	{
		fd_set readSockets = allSockets;
		fd_set writeSockets = allSockets;   //检查是否有可写的客户端，返回可以被send消息的客户端，只要连接建立成功那么该客户端socket就是可写的
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
			//有响应
			//处理错误
			for (u_int i=0;i<errorSockets.fd_count;i++) 
			{
				char str[100] = { 0 };
				int len = 99;
				if (SOCKET_ERROR == getsockopt(errorSockets.fd_array[i], SOL_SOCKET, SO_ERROR, str, &len))
				{
					printf("无法得到错误信息！");
				}
				printf("%s\n", str);
			}
			//有可写的socket
			for (u_int i = 0;i < writeSockets.fd_count;i++)
			{
				if (send(writeSockets.fd_array[i], "这是服务器。。。", sizeof("这是服务器。。。"), 0) == SOCKET_ERROR)
				{
					WSAGetLastError();
				}
			}
			//recv accept
			for (u_int i = 0;i < readSockets.fd_count;i++)
			{
				if (readSockets.fd_array[i] == serverSocket)  //服务器socket有响应  accept
				{
					SOCKET clientSocket = accept(serverSocket, NULL, NULL);
					if (clientSocket == INVALID_SOCKET)
					{
						continue;
					}
					//添加进数组里
					FD_SET(clientSocket, &allSockets);
					printf("# %d 号客户端连接成功！\n",allSockets.fd_count-1);
				}
				else  //客户端socket有响应  recv
				{
					char buf[1500] = { 0 };
					int nRecv = recv(readSockets.fd_array[i], buf, 1500, 0);
					if (nRecv == 0)
					{
						//客户端下线,把该客户端socket从数组中删除
						SOCKET tempsocket = readSockets.fd_array[i];
						FD_CLR(readSockets.fd_array[i], &allSockets);
						//释放
						closesocket(tempsocket);
					}
					else if (nRecv > 0)
					{
						//收到消息
						printf("收到消息:%s\n",buf);

					}
					else //SOCK_ERROR 强制下线(直接关闭窗口)也是执行这段代码
					{
						//接收出错
						int a = WSAGetLastError();
						if (a == 10054)
						{
							//客户端下线,把该客户端socket从数组中删除
							SOCKET tempsocket = readSockets.fd_array[i];
							FD_CLR(readSockets.fd_array[i], &allSockets);
							//释放
							closesocket(tempsocket);
						}
					}
				}
			}
		}
		else
		{
			//发生错误
			WSAGetLastError();
			break;
		}
	}


	////集合中删除指定socket，删除后一定要closesocket（）；
	//FD_CLR(serverSocket, &clientSocket);
	////判断一个socket是否在集合中。存在返回非零，不存在返回零
	//FD_ISSET(serverSocket, &clientSocket);

	//释放所有socket
	for (u_int i = 0;i < allSockets.fd_count;i++)
	{
		closesocket(allSockets.fd_array[i]);
	}
	WSACleanup();
	return 0;
}