#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>


std::string ConfigParser::trim(const std::string& str) 
{
    	size_t first = str.find_first_not_of(" \t\r\n;");
    	size_t last = str.find_last_not_of(" \t\r\n;");
    	return (first == std::string::npos || last == std::string::npos) ? "" : str.substr(first, (last - first + 1));
}


ConfigParser::ConfigParser(const std::string& configFile) : configFile(configFile) {}


std::vector<std::string> ConfigParser::readFile() 
{
    	std::ifstream file(configFile);
    	if (!file.is_open())
        	throw std::runtime_error("Unable to open config file: " + configFile);

    	std::vector<std::string> lines;
    	std::string line;
    	while (std::getline(file, line)) 
	{
        	if (!line.empty() && line[0] != '#') 
            		std::cout << line << std::endl;
		lines.push_back(line);
    	}

	return lines;
}


void ConfigParser::parseServerBlock(const std::vector<std::string>& lines, size_t& index, std::vector<ServerConfig>& servers) 
{
    	ServerConfig server;
    	while (index < lines.size()) 
	{
        	std::string line = trim(lines[index++]);
        	if (line == "}") 
			break;

        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "host")
            iss >> server.host;
        else if (key == "port")
            iss >> server.port;
        else if (key == "server_name")
            iss >> server.server_name;
        else if (key == "client_body_size") {
    std::string value;
    iss >> value;
    server.client_body_size = std::stoul(value);
}

	else if (key == "error_page") 
	{
            int code;
            std::string path;
            iss >> code >> path;
            server.routes["/"].error_pages[std::to_string(code)] = path;
        } 
	else if (key == "location")
            parseLocationBlock(lines, index, server);
    	}
    	servers.push_back(server);
}


void ConfigParser::parseLocationBlock(const std::vector<std::string>& lines, size_t& index, ServerConfig& server) 
{
    	RouteConfig route;
    	std::string path;
    	std::istringstream iss(lines[index - 1]);
    	iss >> path >> path;
    	path = trim(path);

    	while (index < lines.size()) 
	{
        	std::string line = trim(lines[index++]);
        	if (line == "}") 
			break;

        	std::istringstream iss(line);
        	std::string key;
        	iss >> key;
		if (key == "upload_path") 
		{
            		std::string value;
            		iss >> value;
            		route.upload_path = trim(value);
        	}
        	else if (key == "root") 
		{
            		std::string value;
            		iss >> value;
            		route.root = trim(value);
        	} 
		else if (key == "methods") 
		{
            		std::string method;
            		while (iss >> method) 
                		route.methods.push_back(trim(method));
      		}
		else if (key == "default_file") 
		{
            		std::string value;
            		iss >> value;
            		route.default_file = trim(value);
        	}
		else if (key == "http_redirection") 
		{
    			std::string value;
    			iss >> value;
    			route.http_redirection = trim(value);
		}

    	}

    	server.routes[path] = route;
}	



std::vector<ServerConfig> ConfigParser::parse() 
{
    	std::vector<std::string> lines = readFile();
    	std::vector<ServerConfig> servers;

    	std::cout << "Debug: Reading configuration lines:\n";
    	for (std::string& line : lines) 
	{
        	line = trim(line);
        	std::cout << line << std::endl;
   	}

    	for (size_t i = 0; i < lines.size(); ++i) 
	{
        	if (lines[i] == "server {") 
		{
            		parseServerBlock(lines, ++i, servers);
        	}
    	}

    	std::cout << "Debug: Parsed server configurations:\n";
    	for (const ServerConfig& server : servers) 
	{
        	std::cout << "Host: " << server.host << ", Port: " << server.port << "\n";
    	}

    	return servers;
}

