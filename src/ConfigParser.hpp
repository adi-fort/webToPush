#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <map>
#include "ServerConfig.hpp"
#include "RouteConfig.hpp"

class ConfigParser {	
	
	public:
    		ConfigParser(const std::string& configFile);
  		std::vector<ServerConfig> parse();
		static std::string trim(const std::string& str);
			
	private:
    
		std::string configFile;
    		std::vector<std::string> readFile();
    		
		void parseServerBlock(const std::vector<std::string>& lines, size_t& index, std::vector<ServerConfig>& servers);
    		
		void parseLocationBlock(const std::vector<std::string>& lines, size_t& index, ServerConfig& server);
};

#endif

