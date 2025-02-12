#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <map>
#include "RouteConfig.hpp"

struct ServerConfig 
{
    	std::string host;
    
	int port;
    
	std::string server_name;
    
	size_t client_body_size;
    
	std::map<std::string, RouteConfig> routes;
};

#endif

