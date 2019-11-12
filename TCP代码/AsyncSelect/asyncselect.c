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
	//�������ڽṹ��
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
	//ע��ṹ��
	RegisterClassEx(&wc);
	//��������
	HWND hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, "PLXWindows", "PLX", WS_OVERLAPPEDWINDOW, 200, 200, 600, 400, NULL, NULL, hInstance, NULL);
	if (NULL == hWnd)
	{
		return 0;
	}
	//��ʾ����
	ShowWindow(hWnd, nShowCmd);  //1��������ʾ��0����С����ʾ
	//���´���
	UpdateWindow(hWnd);

	/***************�������***************/
	WSADATA wsaData;
	int nRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(nRes != 0)
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
	if (2!=HIBYTE(wsaData.wVersion) || 2!=LOBYTE(wsaData.wVersion))
	{
		WSACleanup();
		return 0;
	}
	//����socket
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(INVALID_SOCKET == serverSocket)
	{
		printf("socket error:%d\n", WSAGetLastError());
		WSACleanup();
		return 0;
	}
	//��socket
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
	//����
	if (SOCKET_ERROR == listen(serverSocket,SOMAXCONN))
	{
		printf("listen error:%d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	//�첽
	if (SOCKET_ERROR == WSAAsyncSelect(serverSocket, hWnd, UM_ASYNCSELECTMSG, FD_ACCEPT))
	{
		printf("WSAAsyncSelect error:%d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	FD_ZERO(&allSockets);
	FD_SET(serverSocket, &allSockets);
	
	//��Ϣѭ��
	MSG msg;
	while (GetMessage(&msg,NULL,0,0)) // һ��ȡһ����Ϣ��Ȼ�󴫵ݸ��ص��������ص�����Ҳ��һ�δ���һ����Ϣ
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//�ر�����socket
	for (u_int i=0;i<allSockets.fd_count;i++)
	{
		closesocket(allSockets.fd_array[i]);
	}
	//�رո�socket����Ϣ
	//WSAAsyncSelect(serverSocket, hWnd, 0, 0);
	WSACleanup();
	return 0;
}

int Y = 0;

//�ص�����
LRESULT CALLBACK WINBackProc(HWND hWnd,UINT msgID,WPARAM wParam,LPARAM lParam)
{
	HDC hdc = GetDC(hWnd);
	
	switch (msgID)
	{
	case UM_ASYNCSELECTMSG:
		//MessageBox(NULL, TEXT("�пͻ�������!"), TEXT("������ʾ"), MB_OKCANCEL);
	{
		//��ȡsocket,��ĳ���ͻ��˸�������ͨ���� ��Windows�ͻ�Ѷ�Ӧ��socket����WPARAM������
		SOCKET sock = (SOCKET)(wParam); // ���пͻ������Ӵ�ʱsockΪserversock�����ͻ��˷�����Ϣ����������ʱsockΪ����Ϣ�Ŀͻ��ˣ�
		//��ȡ��Ϣ
		if (0!=HIWORD(lParam))  
		{
			//�д�
			if(WSAECONNABORTED == HIWORD(lParam))
			{
				TextOut(hdc, 0, Y, TEXT("*****client closed*****"), strlen(TEXT("*****client closed*****")));
				Y += 20;
				//�رո�socket����Ϣ
				WSAAsyncSelect(sock, hWnd, 0, 0);
				closesocket(sock);
				FD_CLR(sock, &allSockets);
				break;
			}
			break;
		}
		//������Ϣ
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
				//�첽�����ͻ���Ͷ�ݵ���Ϣ���У�
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
			//�رո�socket����Ϣ
			WSAAsyncSelect(sock, hWnd, 0, 0);
			closesocket(sock);
			FD_CLR(sock, &allSockets);
			break;
		}
	}
		break;
	case WM_CREATE:      //���ڳ�ʼ��
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	
	ReleaseDC(hWnd, hdc);
	return DefWindowProc(hWnd,msgID,wParam,lParam);
}