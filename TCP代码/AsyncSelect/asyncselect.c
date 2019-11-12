#define _WINSOCK_DEPRECATED_NO_WARNINGS 
//#include <windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib,"ws2_32.lib")

#define UM_ASYNCSELECTMSG WM_USER + 1

LRESULT CALLBACK WINBackProc(HWND hWnd, UINT msgID, WPARAM wParam, LPARAM lParam);


fd_set allSockets;

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPreInstance,LPSTR lpCmdLine,int nShowCmd)
{
	//创建窗口结构体
	WNDCLASSEX wc;
	wc.cbClsExtra = 0;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.cbWndExtra = 0;
	wc.hbrBackground = NULL;
	wc.hCursor = NULL;
	wc.hIcon = NULL;
	wc.hIconSm = NULL;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WINBackProc;
	wc.lpszClassName = "PLXWindows";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	//注册结构体
	RegisterClassEx(&wc);
	//创建窗口
	HWND hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, "PLXWindows", "PLX", WS_OVERLAPPEDWINDOW, 200, 200, 600, 400, NULL, NULL, hInstance, NULL);
	if (NULL == hWnd)
	{
		return 0;
	}
	//显示窗口
	ShowWindow(hWnd, nShowCmd);  //1：正常显示；0：最小化显示
	//更新窗口
	UpdateWindow(hWnd);

	/***************网络代码***************/
	WSADATA wsaData;
	int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(nRes != 0)
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
	if (2!=HIBYTE(wsaData.wVersion) || 2!=LOBYTE(wsaData.wVersion))
	{
		WSACleanup();
		return 0;
	}
	//创建socket
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(INVALID_SOCKET == serverSocket)
	{
		printf("socket error:%d\n", WSAGetLastError());
		WSACleanup();
		return 0;
	}
	//绑定socket
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("10.100.145.239");
	serverAddr.sin_port = htons(12345);
	if (SOCKET_ERROR == bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
	{
		printf("bind error:%d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//监听
	if (SOCKET_ERROR == listen(serverSocket,SOMAXCONN))
	{
		printf("listen error:%d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//异步
	if (SOCKET_ERROR == WSAAsyncSelect(serverSocket, hWnd, UM_ASYNCSELECTMSG, FD_ACCEPT))
	{
		printf("WSAAsyncSelect error:%d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	FD_ZERO(&allSockets);
	FD_SET(serverSocket, &allSockets);
	
	//消息循环
	MSG msg;
	while (GetMessage(&msg,NULL,0,0)) // 一次取一个消息，然后传递给回调函数；回调函数也是一次处理一个消息
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//关闭所有socket
	for (u_int i=0;i<allSockets.fd_count;i++)
	{
		closesocket(allSockets.fd_array[i]);
	}
	//关闭该socket的消息
	//WSAAsyncSelect(serverSocket, hWnd, 0, 0);
	WSACleanup();
	return 0;
}

int Y = 0;

//回调函数
LRESULT CALLBACK WINBackProc(HWND hWnd,UINT msgID,WPARAM wParam,LPARAM lParam)
{
	HDC hdc = GetDC(hWnd);
	
	switch (msgID)
	{
	case UM_ASYNCSELECTMSG:
		//MessageBox(NULL, TEXT("有客户端连接!"), TEXT("连接提示"), MB_OKCANCEL);
	{
		//获取socket,当某个客户端跟服务器通信了 ，Windows就会把对应的socket传到WPARAM参数里
		SOCKET sock = (SOCKET)(wParam); // 当有客户端连接此时sock为serversock；当客户端发送消息给服务器此时sock为发消息的客户端；
		//获取消息
		if (0!=HIWORD(lParam))  
		{
			//有错
			if(WSAECONNABORTED == HIWORD(lParam))
			{
				TextOut(hdc, 0, Y, TEXT("*****client closed*****"), strlen(TEXT("*****client closed*****")));
				Y += 20;
				//关闭该socket的消息
				WSAAsyncSelect(sock, hWnd, 0, 0);
				closesocket(sock);
				FD_CLR(sock, &allSockets);
				break;
			}
			break;
		}
		//具体消息
		switch (LOWORD(lParam))
		{
		case FD_ACCEPT: 
			{
				TextOut(hdc, 0, Y, TEXT("accept"), strlen(TEXT("accept")));
				Y+=20;
				SOCKET clientSocket = accept(sock, NULL, NULL);
				if (INVALID_SOCKET == clientSocket)
				{
					printf("callback accept error:%d\n", WSAGetLastError());
					break;
				}
				//异步（将客户端投递到消息队列）
				if (SOCKET_ERROR == WSAAsyncSelect(clientSocket, hWnd, UM_ASYNCSELECTMSG, FD_CLOSE | FD_READ | FD_WRITE))
				{
					printf("callback WSAAsyncSelect error:%d\n", WSAGetLastError());
					closesocket(clientSocket);
					break;
				}
				FD_SET(clientSocket, &allSockets);
				send(clientSocket, "connect success", sizeof("connect success"), 0);
			}
			break;
		case FD_READ:
			{
				TextOut(hdc, 0, Y, TEXT("read:"), strlen(TEXT("read:")));
				char buf[1500] = { 0 };
				if (SOCKET_ERROR == recv(sock, buf, 1499, 0))
				{
					break;
				}
				TextOut(hdc, 35, Y, buf, strlen(buf));
				Y += 20;
			}
			break;
		case FD_WRITE:
			if (send(sock, "connect success", sizeof("connect success"), 0) == SOCKET_ERROR)
			{
				TextOut(hdc, 0, Y, TEXT("send error"), strlen(TEXT("send error")));
				Y += 20;
			}
			TextOut(hdc, 0, Y, TEXT("write"), strlen(TEXT("write")));
			Y += 20;
			break;
		case FD_CLOSE:
			TextOut(hdc, 0, Y, TEXT("*****client closed*****"), strlen(TEXT("*****client closed*****")));
			Y += 20;
			//关闭该socket的消息
			WSAAsyncSelect(sock, hWnd, 0, 0);
			closesocket(sock);
			FD_CLR(sock, &allSockets);
			break;
		}
	}
		break;
	case WM_CREATE:      //用于初始化
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	
	ReleaseDC(hWnd, hdc);
	return DefWindowProc(hWnd,msgID,wParam,lParam);
}