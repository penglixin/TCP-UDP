#define _WINSOCK_DEPRECATED_NO_WARNINGS 
//#include <Windows.h>
#include <WinSock2.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib,"ws2_32.lib")

#define UM_ASYNCSELECTMSG WM_USER + 1

LRESULT CALLBACK WINBackProc(HWND hWnd, UINT msgID, WPARAM wParam, LPARAM lParam);

//SOCKADDR_IN g_clientAddr[10];
//int g_count;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInstance, LPSTR lpCmdLine, int nShowCmd)
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
	if (nRes != 0)
	{
		return 0;
	}
	//�汾У��
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		WSACleanup();
		return 0;
	}
	SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == serverSocket)
	{
		WSACleanup();
		return 0;
	}
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("10.100.151.6");
	serverAddr.sin_port = htons(12345);
	if (SOCKET_ERROR == bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)))
	{
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	if (SOCKET_ERROR == WSAAsyncSelect(serverSocket, hWnd, UM_ASYNCSELECTMSG, FD_READ | FD_WRITE))
	{
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}

	//��Ϣѭ��
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	closesocket(serverSocket);
	WSACleanup();
	return 0;
}

int Y = 0;

//BOOL clientAddrIsExist(SOCKADDR_IN clientAddr)
//{
//	BOOL isExist = FALSE;
//	if (g_count == 0)
//	{
//		isExist = FALSE;
//	}
//	else
//	{
//		for (int i = 0;i < g_count;i++)
//		{
//			if (&g_clientAddr[i] == &clientAddr)
//			{
//				isExist = TRUE;
//				return isExist;
//			}
//			else
//			{
//				isExist = FALSE;
//			}
//		}
//	}
//	return isExist;
//}

LRESULT CALLBACK WINBackProc(HWND hWnd, UINT msgID, WPARAM wParam, LPARAM lParam)
{
	HDC hdc = GetDC(hWnd);
	switch (msgID)
	{
	case UM_ASYNCSELECTMSG:
	{
		//��ȡsocket,��ĳ���ͻ��˸�������ͨ���� ��Windows�ͻ�Ѷ�Ӧ��socket����WPARAM������
		SOCKET sock = (SOCKET)(wParam); // ���пͻ������Ӵ�ʱsockΪserversock�����ͻ��˷�����Ϣ����������ʱsockΪ����Ϣ�Ŀͻ��ˣ�
		//��ȡ��Ϣ
		if (HIWORD(lParam) != 0)
		{
			TextOut(hdc, 0, Y, TEXT("error"), strlen(TEXT("error")));
			Y += 20;
			//�رո�socket����Ϣ
			WSAAsyncSelect(sock, hWnd, 0, 0);
			closesocket(sock);
			break;
		}
		//������Ϣ
		switch (LOWORD(lParam))
		{
			case  FD_WRITE:
				TextOut(hdc, 0, Y, TEXT("write"), strlen(TEXT("write")));
				Y += 20;
				break;
			case FD_READ:
				{
					TextOut(hdc, 0, Y, TEXT("read:"), strlen(TEXT("read:")));
					char strbuf[548] = { 0 };
					SOCKADDR_IN clientAddr;
					int fromlen = sizeof(clientAddr);
					int num = recvfrom(sock, strbuf, sizeof(strbuf), 0, (struct sockaddr*)&clientAddr, &fromlen);
					if (num == SOCKET_ERROR)
					{
						break;
					}
					TextOut(hdc, 35, Y, strbuf, strlen(strbuf));
					Y += 20;
					/*if (!clientAddrIsExist(clientAddr))
					{
						g_clientAddr[g_count] = clientAddr;
						g_count++;
					}*/
					//for (int i=0;i<g_count;i++)
					//{
						num = sendto(sock, "�������յ�", sizeof("�������յ�"), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
						if (num == SOCKET_ERROR)
						{
							TextOut(hdc, 0, Y, TEXT("sendto error"), strlen(TEXT("sendto error")));
							Y += 20;
						}
					//}
					
				}
				break;
		}
	}
	break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	ReleaseDC(hWnd, hdc);
	return DefWindowProc(hWnd, msgID, wParam, lParam);
}
