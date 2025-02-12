#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "HttpRequest.hpp"
#include "Server.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include "HttpRequest.hpp"
#include "ServerConfig.hpp"

class RequestHandler 
{
	public:
        	
		RequestHandler(const std::map<std::string, ServerConfig>& server_configs);
		RequestHandler();
        	~RequestHandler();

        	std::string	handleRequest(const HttpRequest& request, int client_fd);
		
		void logServerConfig() const;
		
		std::string 	decodeUrl(const std::string& url);

		std::string 	handleDeleteAllRequest(const HttpRequest& request);

		const ServerConfig* findServerConfig(const std::string& host, int port) const;

    		const RouteConfig* findRouteConfig(const ServerConfig& server, const std::string& path) const;

    	private:

        	std::map<std::string, ServerConfig> server_configs;

		std::string 	handlePostRequest(const HttpRequest& request);
		
		std::string 	handleDeleteRequest(const HttpRequest& request);
    		
		std::string 	serveStaticFile(const std::string& path);
    
		std::string 	handleCgiRequest(const HttpRequest& request);   		
	
		std::string	generateErrorResponse(int errorCode);
		
		std::string 	handleUploadRequest(int client_fd, const HttpRequest& request);
		
		std::string 	serveUploadedFile(const HttpRequest& request);
		
		std::string	serveUploadPage();
		
		std::vector<std::string>	getUploadedFiles();
		
		std::string			listUploadedFiles();

		
		void	logRequest(const HttpRequest& request, const std::string& response);

		std::vector<char*>		buildCgi(const HttpRequest& request);
};

#endif

