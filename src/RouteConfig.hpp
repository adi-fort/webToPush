#ifndef ROUTECONFIG_HPP
#define ROUTECONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct RouteConfig 
{
    	std::string 				root;
    	
	std::vector<std::string> 		methods;
    	
	std::string 				default_file;
    	
	bool 					directory_listing;
    	
	std::string 				upload_path;
    	
	std::map<std::string, std::string> 	error_pages;
    	
	std::map<std::string, std::string> 	cgi_extensions;

	std::string				http_redirection;
};

#endif

