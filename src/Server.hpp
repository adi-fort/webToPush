#ifndef SERVER_HPP
#define SERVER_HPP

#include "ServerConfig.hpp"
#include "HttpRequest.hpp"
#include "ConfigParser.hpp"
#include "FileInfo.hpp"
#include <string>
#include <netinet/in.h>
#include <map>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <iostream>
#include <algorithm>

class Server
{
	private:

		std::vector<int>		server_fds;
		std::vector<sockaddr_in>	server_addresses;	

		//int                 		_server_fd;
		struct sockaddr_in  		_address;
		int                 		_port;
		std::string			_host;
		
		std::string extractHost(const std::string& request);
		std::map<int, HttpRequest> client_requests;

	public:
		
		bool processCompleteRequest(int fd, const std::string &completeRequest);

		std::map<std::string, ServerConfig> server_configs;
		
		Server(const std::string& configFile);
		Server(int port);
		~Server();

		void init();
		void run();
	
		void processClient(int client_fd);
		bool parseHeaders(int client_fd);
		bool parseBody(int client_fd);
			
		void parseHeaders(HttpRequest& request, std::istringstream& request_stream);
		void parseBody(HttpRequest& request, std::vector<unsigned char>& body);
		
		std::string serveStaticFile(const std::string& path);
		const ServerConfig* getServerConfig(const std::string& host, int port) const;
};

#endif

