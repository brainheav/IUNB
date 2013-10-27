#pragma once
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
namespace Brainheav
{
	namespace sock_heplers
	{
		class SmSock // Smart Socket
			{
			public:
				explicit SmSock():socket(INVALID_SOCKET){}
				explicit SmSock(SOCKET sock):socket(sock){}
				SmSock(SmSock && rsock):socket(rsock.socket)
				{
					rsock.socket=INVALID_SOCKET;
				}
				SmSock & operator=(SmSock && rsock)
				{
					socket=rsock.socket;
					rsock.socket=INVALID_SOCKET;
				}
				SOCKET & data()
				{
					return socket;
				}
				SOCKET release()
				{
					SOCKET tmp(socket);
					socket=INVALID_SOCKET;
					return tmp;
				}
				~SmSock()
				{
					if (socket!=INVALID_SOCKET)
					{
						shutdown(socket, SD_BOTH);
						closesocket(socket);
					}
				}
			private:
				SOCKET socket;
			private:
				SmSock(const SmSock & rsock);
				SmSock & operator=(const SmSock & rsock);
			};// !class SmSock
		class WSErr:public std::exception // Win Socket Error
			{
			public:
				int err;
				WSErr(const char * p, int e):exception(p), err(e){};
			};
		class SmAdInf // Smart Address Info
			{
			public:
				addrinfo hints;
				addrinfo * result;
				SmAdInf():result(nullptr)
				{
					memset(&hints, 0x00,  sizeof(hints));
				}
				unsigned long * get_addr()
				{
					return reinterpret_cast<unsigned long *>(result->ai_addr->sa_data+2);
				}
				unsigned short * get_port()
				{
					return reinterpret_cast<unsigned short *>(result->ai_addr->sa_data);
				}
				void clear()
				{
					if(result) 
					{
						freeaddrinfo(result);
						result=nullptr;
					}
					memset(&hints, 0x00,  sizeof(hints));
				}
				~SmAdInf()
				{
					if(result) freeaddrinfo(result);
				}
			private:
				SmAdInf(const SmAdInf &);
				SmAdInf(SmAdInf &&);
				SmAdInf & operator=(const SmAdInf &);
				SmAdInf & operator=(SmAdInf &&);
			};// !class SmAdInf
		struct WinSockInit
			{
				WinSockInit(WORD version)
				{
					struct WSInit
					{
						WSADATA wsadata;
						int err;
						WSInit(WORD version):err(1)
						{
							if(err = WSAStartup(version, &wsadata))
							{
								WSACleanup();
								throw WSErr("WSAStartup in "__FUNCTION__, err);
							}
							else std::cout << "\nWinsock start at version " 
								<< static_cast<u_short>(LOBYTE(wsadata.wVersion)) << '.' 
								<< static_cast<u_short>(HIBYTE(wsadata.wVersion)) << '\n';
						}
						~WSInit()
						{
							WSACleanup();
						}
					};//!struct WSInit
					static WSInit WSInitObj(version);
				}
			};//!struct WinSockInit
	}// !sock_heplers
}// !Brainheav