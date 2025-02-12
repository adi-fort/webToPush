#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <iostream>
#include <unistd.h>
#include <vector>
#include "FileInfo.hpp"


class HttpRequest
{
	public:
		std::string				method;
		std::string				path;
		std::string				protocol;
		std::map<std::string, std::string>	headers;
		std::vector<unsigned char>		body;		
		std::string 				host;
    		int 					port;
    		int 					fd;
		size_t					content_length;	
		

		std::vector<FileInfo>			files;
		
		HttpRequest();
		~HttpRequest();
			
};

#endif
