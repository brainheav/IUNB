#pragma once
#include <thread>
#include <iostream>
#include "sock_helpers.h"
#include "xml_pref.h"
namespace Brainheav
{
	class iunb
	{
	private:
		Brainheav::sock_heplers::SmSock sock; // RAII wrapper around winsock

		Brainheav::xml_pref xml_par;	// Parameters from iunb.xml file. 
										// May throw exception xml_err if not ok.
										// Check xml_pref::err if you dont't want to have exception.
		std::string cookie;
		size_t max_tcp;
	public:

		// Connect to server with password and login and
		// save parsed reply to cookie, to be auth.
		iunb(std::string login, std::string password, const char * filename):xml_par(filename)
		{
			// Initial cookie string. Will be appended to GET request. "Cookie: " - for default
			cookie+=	xml_par.get("<cookie>", "</cookie>").get_string();

			auto addr=	xml_par.get("<addr>", "</addr>").get_string();
			auto port=	xml_par.get("<port>", "</port>").get_string();
			max_tcp=	xml_par.get("<max_tcp>", "</max_tcp>").get_ll();
	
			// Get from xml param file POST request and replace password and login
			// with user input, if any
			auto post_req=xml_par.get("<post_auth>", "</post_auth>").get_string();
			auto pos=post_req.find("$login");
			if (pos!=post_req.npos)
			{
				post_req.replace(pos, strlen("$login"), login);
				pos=post_req.find("$password", pos);
				if (pos!=post_req.npos) post_req.replace(pos, strlen("$password"), password);
			}
			
			// Get from xml param file GET request and replace $Content-Length
			// with size of post request, if any
			auto get_req=xml_par.get("<get_auth>", "</get_auth>").get_string();
			pos=get_req.find("$Content-Length");
			if (pos!=get_req.npos)
			{
				get_req.replace(pos, strlen("$Content-Length"), std::to_string(post_req.size()));
			}
			
			get_req+=post_req;

			// Init addr structure with def IP4 parameters
			Brainheav::sock_heplers::SmAdInf sai;
			sai.hints.ai_family=AF_INET;
			sai.hints.ai_socktype=SOCK_STREAM;      
			sai.hints.ai_protocol=IPPROTO_TCP;
			
			// Get IP and port in native machine way
			if (getaddrinfo(addr.c_str(), port.c_str(), &sai.hints, &sai.result)) 
			{
				throw Brainheav::sock_heplers::WSErr("getaddrinfo() in "__FUNCTION__, WSAGetLastError());
			}
			
			// Init socket to IP and port with parameters from above
			sock.data()=socket(	sai.result->ai_family, sai.result->ai_socktype, sai.result->ai_protocol);
			if (INVALID_SOCKET==sock.data())
			{
				throw Brainheav::sock_heplers::WSErr("socket() in "__FUNCTION__, WSAGetLastError());
			}
			
			// Connect to 	IP and port from above
			if (connect(sock.data(), sai.result->ai_addr, sai.result->ai_addrlen)) 
			{
				throw Brainheav::sock_heplers::WSErr("connect() in "__FUNCTION__, WSAGetLastError());
			}
			
			// Send GET request with POST data to auth	
			if (SOCKET_ERROR==send(sock.data(), get_req.c_str(), get_req.size(), 0)) 
			{
				throw Brainheav::sock_heplers::WSErr("send() in "__FUNCTION__,WSAGetLastError());
			}
			
			// Receive reply with user_id, user_hash and PHPSESSID
			get_req.resize(max_tcp);
			if (SOCKET_ERROR==recv(sock.data(), &get_req[0], get_req.size(), 0))
			{
				throw Brainheav::sock_heplers::WSErr("recv() in "__FUNCTION__,WSAGetLastError());
			}
	
			size_t end(0); // End of some sequences below

			// Append to cookie user_id, if any
			pos=get_req.find(xml_par.get("<user_id>", "</user_id>").get_string());
			if (pos!=get_req.npos)
			{
				end=get_req.find(';', pos); ++end;
				cookie.append(get_req, pos, end-pos);
				cookie+=' ';
			}
			// Append to cookie user_id, if any
			pos=get_req.find(xml_par.get("<user_hash>", "</user_hash>").get_string());
			if (pos!=get_req.npos)
			{
				end=get_req.find(';', pos); ++end;
				cookie.append(get_req, pos, end-pos);
				cookie+=' ';
			}
			// Append to cookie PHPSESSID, if any	
			pos=get_req.find(xml_par.get("<PHPSID>", "</PHPSID>").get_string());
			if (pos!=get_req.npos)
			{
				end=get_req.find(';', pos);
				cookie.append(get_req, pos, end-pos);
			}	
		}// !iunb(std::string, std::string, const char *)

		// Send GET request with cookie, that contains auth.
		// Get reply from server and parse it to find unread books.
		// Repeat until count(unread books) < num
		// num - integer from user pref file iunb.xml
		std::string get_undread	()
		{
			// Start search from this page
			auto page_num=	xml_par.get("<start_page>", "</start_page>").get_ll();
			// Num of unread books, that user wish to find
			auto num=		xml_par.get("<unread_num>", "</unread_num>").get_ll();
			// GET request without cookie and page_num yet
			auto get_req=	xml_par.get("<get_unread>", "</get_unread>").get_string();
			
			// Replace $Cookie with cookie string processed in constructor
			auto pos=get_req.find("$Cookie");
			if (pos!=get_req.npos)
			{
				get_req.replace(pos, strlen("$Cookie"), cookie);
			}
			
			// Find pos for replacing $pagenumber with 1-...N+1 in for() below
			// This allow to visit next page
			size_t len(0);
			char c_page_num[50];
			pos=get_req.find("$pagenumber");
			if (pos!=get_req.npos)
			{
				len=strlen("$pagenumber");
			}
			
			// Don't block my socket!
			u_long mode(1);
			ioctlsocket(sock.data(), FIONBIO, &mode);
			
			// This string will contain HTML-like list of unread_books
			std::string result =	"\xEF\xBB\xBF" // UTF-8 BOM
									"<html><head><meta charset charset=\"utf-8\"></head><body>";
			
			std::string buf(max_tcp, '\0'); // buffer for receiving from server
	
			// size_t var	for searching books
			size_t	beg_search(0), unread_book_pos(0), beg_pos(0), end_pos(0),
			//				for waiting response from server till wait_count < 
					wait_count(0),
			//				for managing buf
					size(0), offs(0), offs_before(0);
			
			// count - count unread books. num - desired.
			for (size_t count(0); count<num; wait_count=0)
			{
				std::cout << '\n' << page_num << " page processing, total books found: ";
				
				// integer page_num to c-style string c_page_num
				// replace $pagenumber with it
				// (pos was found above)
				if (pos!=get_req.npos)
				{
					_itoa_s(page_num++, c_page_num, 10);
					get_req.replace(pos, len, c_page_num);
					len=strlen(c_page_num);
				}
				
				// Send GET with auth inf
				if (SOCKET_ERROR==send(sock.data(), get_req.c_str(), get_req.size(), 0)) 
				{
					throw Brainheav::sock_heplers::WSErr("send() in "__FUNCTION__,WSAGetLastError());
				}
				
				// Receive reply. offs - Offset - size of data, that buffer already contain.
				while (size=recv(sock.data(), &buf[offs], buf.size()-offs, 0))
				{
					if (size==SOCKET_ERROR) 
					{
						// Time-out. wait_count reset here at every success.
						// And in for (above). for (...wait_count=0)
						if (WSAEWOULDBLOCK==WSAGetLastError())
						{
							if (++wait_count>10) break;
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
							continue;
						} else throw Brainheav::sock_heplers::WSErr("recv() in "__FUNCTION__,WSAGetLastError());
					} else wait_count=0;
					offs+=size;
	
					unread_book_pos=buf.find("data-rate=\"\"", beg_search);

					while (unread_book_pos!=buf.npos && count<num)
					{
						beg_pos=buf.rfind("<a href=\"", unread_book_pos);
						end_pos=buf.find("</a>", beg_pos);
	
						result.append(buf, beg_pos, end_pos-beg_pos);
						result+="</a><br>";
	
						++count;
	
						unread_book_pos=buf.find("data-rate=\"\"", ++unread_book_pos);
					}
					
					// 14==strlen("data-rate=\"\"")
					beg_search=offs-14;

					if (buf.size()-offs<max_tcp) buf.resize(buf.size()*2);
				}
	
				std::cout << count << '\n';

				// Have I received any?
				if (offs_before!=offs) offs_before=offs;
				else break;
			}
			result+="</body></html>";
			return std::move(result);
		}
	};// !class iunb
}// !namespace Brainheav