#include "Server.hpp"
#include "FileInfo.hpp"
#include <cerrno>
#include "HttpRequest.hpp"
#include "ConfigParser.hpp"
#include "RequestHandler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <algorithm> 



std::map<std::string, std::string> parseHeaders(const std::string &headers) 
{
    	std::map<std::string, std::string> header_map;
    	std::istringstream stream(headers);
    	std::string line;

    	while (std::getline(stream, line)) 
	{
        	if (!line.empty() && line.back() == '\r') 
            		line.pop_back();

        	size_t delimiter = line.find(": ");
        	if (delimiter != std::string::npos) 
		{
            		std::string key = line.substr(0, delimiter);
            		std::string value = line.substr(delimiter + 2);

            		std::transform(key.begin(), key.end(), key.begin(), ::tolower);

            		header_map[key] = value;
        	}
    	}
	return header_map;
}

/*
bool recvDataFromClient(int fd, HttpRequest &request) {
    char buffer[4096];  // Buffer temporaneo da 4KB
    int bytes_read = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes_read < 0) {
        std::cerr << "ERROR: recv() fallito con errore: " << strerror(errno) << std::endl;
        return false;
    } 
    else if (bytes_read == 0) {
        std::cout << "Client closed the connection." << std::endl;
        return false;
    }

    // Aggiungiamo i dati letti al body (anche se sono header per ora)
    request.body.insert(request.body.end(), buffer, buffer + bytes_read);
    return true;
}*/

bool parseHttpRequest(HttpRequest &request) {
    std::string request_string(request.body.begin(), request.body.end());

    // Trova la fine degli header
    size_t header_end = request_string.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        std::cerr << "ERROR: Header HTTP non completo!" << std::endl;
        return false;
    }

    // Estrarre gli header
    std::string headers = request_string.substr(0, header_end);
    request.headers = parseHeaders(headers); // Funzione che estrae header in una mappa

    // Trova Content-Length
    if (request.headers.find("Content-Length") != request.headers.end()) {
        request.content_length = std::stoi(request.headers["Content-Length"]);
    }

    // Inizio del body
    size_t body_start = header_end + 4;
    if (body_start < request.body.size()) {
        request.body.erase(request.body.begin(), request.body.begin() + body_start);
    } else {
        request.body.clear();
    }

    return true;
}

/*
bool recvRemainingBody(int fd, HttpRequest &request) {
    size_t missing = request.content_length - request.body.size();

    while (request.body.size() < request.content_length) {
        char buffer[4096];
        int bytes_read = recv(fd, buffer, std::min(missing, sizeof(buffer)), 0);

        if (bytes_read <= 0) {
            std::cerr << "ERROR: recv() fallito o connessione chiusa." << std::endl;
            return false;
        }

        request.body.insert(request.body.end(), buffer, buffer + bytes_read);
        missing -= bytes_read;

        // Debug per mostrare il progresso della lettura
        std::cout << "DEBUG: Bytes letti finora: " << request.body.size() << " / " << request.content_length << std::endl;
    }

    std::cout << "DEBUG: Lettura completa del body. Byte totali: " << request.body.size() << std::endl;
    return true;
}*/


std::string vectorToString(const std::vector<unsigned char>& vec) 
{
   	return std::string(vec.begin(), vec.end());
}


bool is_numeric(const std::string& str)
{
    	for (std::string::const_iterator it = str.begin(); it != str.end(); ++it)
    	{
        	if (!std::isdigit(static_cast<unsigned char>(*it)))
            	return false;
    	}
    	return true;
}


std::string Server::extractHost(const std::string& request) 
{
    	std::istringstream stream(request);
    	std::string line;
    	std::string host = "";

    	while (std::getline(stream, line)) 
	{
        	std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        	if (line.find("host:") == 0) 
		{
            		size_t pos = line.find(":");
            		if (pos != std::string::npos) 
			{
                		host = line.substr(pos + 1);
                		host.erase(0, host.find_first_not_of(" \t"));
                		host.erase(host.find_last_not_of(" \t\r\n") + 1);
                		size_t pos = host.find("localhost");
    				if (pos != std::string::npos)
        				host.replace(pos, 9, "127.0.0.1");
				return host;
            		}
        	}
    }

    return "127.0.0.1:8080";
}


Server::Server(const std::string& configFile) 
{
    	ConfigParser parser(configFile);
    	std::vector<ServerConfig> parsedConfigs = parser.parse();
    	if (parsedConfigs.empty()) 
	{
        	std::cerr << "Error: No server configurations found in config file!" << std::endl;
        	exit(EXIT_FAILURE);
    	}
    	_port = parsedConfigs[0].port;
    	_address.sin_family = AF_INET;
    	_address.sin_addr.s_addr = INADDR_ANY;
    	_address.sin_port = htons(_port);
    	for (const auto& server : parsedConfigs) 
	{
		std::string host = server.host;
		if (host == "localhost") 
			host = "127.0.0.1";
		std::string key = host + ":" + std::to_string(server.port);
		server_configs[key] = server;
		server_configs[key] = server;
    	}
	for (const auto& entry : server_configs) 
	{
    		std::cout << "Key: " << entry.first << " | Port: " << entry.second.port << std::endl;
	}
}



Server::~Server()
{
    	if (_server_fd != -1)
    	{
        	close(_server_fd);
    	}
}



void Server::init() 
{
   	_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    	if (_server_fd == 0) 
	{
        	perror("Socket creation failed");
        	exit(EXIT_FAILURE);
    	}
    	if (bind(_server_fd, (struct sockaddr*)&_address, sizeof(_address)) < 0) 
	{
        	perror("Bind failed");
        	exit(EXIT_FAILURE);
    	}
    	if (fcntl(_server_fd, F_SETFL, O_NONBLOCK) == -1) 
	{
        	perror("Failed to set non-blocking mode");
        	exit(EXIT_FAILURE);
    	}
    	if (listen(_server_fd, 10) < 0) 
	{
        	perror("Listen failed");
        	exit(EXIT_FAILURE);
    	}
}


void Server::parseHeaders(HttpRequest& request, std::istringstream& request_stream) 
{
    	std::string line;
    	std::string buffer;

    	std::getline(request_stream, line);

    	while (std::getline(request_stream, line)) 
	{
        	if (!line.empty() && line.back() == '\r')
            		line.pop_back();
        
        	buffer += line + "\n";
        	if (line.empty()) 
            		break;
        	size_t delimiter_pos = line.find(":");
        	if (delimiter_pos != std::string::npos) 
		{
            		std::string header_key = line.substr(0, delimiter_pos);
            		std::string header_value = line.substr(delimiter_pos + 1);

            		header_value.erase(0, header_value.find_first_not_of(" \t"));
            		header_value.erase(header_value.find_last_not_of(" \t") + 1);

            		request.headers[header_key] = header_value;
        	}
    	}
	auto it = request.headers.find("Content-Length");
    	if (it == request.headers.end()) 
	{
        	if(request.method == "POST")
		{
			std::cerr << "ERROR: Header Content-Length mancante!" << std::endl;
        		request.headers["Content-Length"] = "0";
    		}
	} 
	else if (it->second.empty()) 
	{
        	std::cerr << "ERROR: Header Content-Length è vuoto!" << std::endl;
        	request.headers["Content-Length"] = "0";
    	}
}



void Server::parseBody(HttpRequest& request, std::vector<unsigned char>& body) 
{
    	std::string contentType;
	std::string boundary;


	auto it = request.headers.find("Content-Type");
    	
	size_t boundary_pos = it->second.find("boundary=");

    	boundary = "--" + it->second.substr(boundary_pos + 9);
    	std::string boundaryFinal = boundary + "--";

	auto boundary_start = std::search(body.begin(), body.end(), boundary.begin(), boundary.end());
    	auto boundary_end = std::search(boundary_start + boundary.size(), body.end(), boundaryFinal.begin(), boundaryFinal.end());
    
    	auto content_start = boundary_start + boundary.size();
    	auto content_end = boundary_end - 2;

    	
    	std::string header_section(body.begin() + std::distance(body.begin(), content_start), 
                               body.begin() + std::distance(body.begin(), content_start) + 500);

    	size_t filename_pos = header_section.find("filename=\"");
    	if (filename_pos == std::string::npos) 
    	{
        	std::cerr << "ERROR! filename non found." << std::endl;
        	return;
    	}

    	filename_pos += 10;
    	size_t filename_end = header_section.find("\"", filename_pos);
    	std::string filename = header_section.substr(filename_pos, filename_end - filename_pos);

    	auto data_start = body.begin() + header_section.size();
	
	std::vector<unsigned char> file_data(content_start, content_end);
    
	FileInfo fileInfo;
    	fileInfo.filename = filename;
    	fileInfo.file_start = file_data.begin();
    	fileInfo.file_end = file_data.end();
    	
	request.files.push_back(fileInfo);
}



void Server::run() 
{
    	struct pollfd fds[100];
    	int nfds = 1;
    
    	fds[0].fd = _server_fd;
    	fds[0].events = POLLIN;

    	std::cout << "Server is running and listening on port " << _port << std::endl;

    	while (true) 
    	{
        	int ret = poll(fds, nfds, -1);
        	if (ret < 0) 
        	{
            		std::cerr << "ERROR: poll() failed" << std::endl;
            		break;
        	}	
        	for (int i = 0; i < nfds; ++i) 
        	{
            		if (fds[i].revents & POLLIN) 
            		{
                		if (fds[i].fd == _server_fd) 
                		{
                    			socklen_t addrlen = sizeof(_address);
                    			int new_socket = accept(_server_fd, reinterpret_cast<struct sockaddr *>(&_address), &addrlen);
                    			if (new_socket < 0)
                        			continue;

                    			fcntl(new_socket, F_SETFL, O_NONBLOCK);
                    			fds[nfds].fd = new_socket;
                    			fds[nfds].events = POLLIN | POLLOUT;
                    			++nfds;

                    			std::cout << "New connection accepted. Total connections: " << nfds - 1 << std::endl;
                		} 
                		else 
                		{ 
                    			std::vector<unsigned char> request_data;
                    			char buffer[8192];
                    			int bytes_read;

                    			while ((bytes_read = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0)) > 0) 
                    			{
						request_data.insert(request_data.end(), buffer, buffer + bytes_read);
                        			if (request_data.size() >= 8192) 
							break;
					}
					if (request_data.empty())
					{
						close(fds[i].fd);
						fds[i].fd = -1;
						continue;
                    			}

                    			std::string request_string(request_data.begin(), request_data.end());
                    			std::istringstream request_stream(request_string);

                    			std::string method, path, protocol;
                    
					request_stream >> method >> path >> protocol;

                    			HttpRequest request;
                    			request.method = method;
                    			request.path = path;
                    			request.protocol = protocol;
                    			request.host = extractHost(request_string);

                    			std::istringstream header_stream(request_string.substr(0, request_string.find("\r\n\r\n")));
                    			parseHeaders(request, header_stream);
			
                    			bool keep_alive = false;
                    			if (request.headers.find("Connection") != request.headers.end()) 
					{
                        			std::string conn_value = request.headers["Connection"];
                        			std::transform(conn_value.begin(), conn_value.end(), conn_value.begin(), ::tolower);
                        			if (conn_value == "keep-alive") 
                            				keep_alive = true;
                    			}

                    			size_t body_length = 0;
                    			if (request.method == "POST" && request.headers.find("Content-Length") != request.headers.end()) 
                    			{
                        			body_length = std::stoul(request.headers["Content-Length"]);
                    				std::vector<unsigned char> request_body;
                    				request_body.reserve(body_length);

                    				size_t header_end_pos = request_string.find("\r\n\r\n") + 4;
                    				size_t body_start_pos = header_end_pos;
                    				size_t bytes_in_request = request_string.size() - body_start_pos;

                    				if (bytes_in_request > 0) 
						{
                        				request_body.insert(request_body.end(), request_data.begin() + body_start_pos, request_data.end());
                    				}

                    				while (request_body.size() < body_length) 
                    				{
                        				poll(fds, nfds, -1);
                        
                        				if (fds[i].revents & POLLIN)
                        				{
                            					int bytes_to_read = std::min(sizeof(buffer), body_length - request_body.size());
                            					bytes_read = recv(fds[i].fd, buffer, bytes_to_read, 0);
                            
                            					if (bytes_read > 0) 
								{
                                					request_body.insert(request_body.end(), buffer, buffer + bytes_read);
                            					}
                        				}
                    				}
						request.body = request_body;
                    				parseBody(request, request.body);
					}
			
                    			if (server_configs.find(request.host) == server_configs.end()) 
                    			{
                        			std::string response = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\n\r\n";
                        			send(fds[i].fd, response.c_str(), response.length(), 0);
                        				continue;
                    			}

                    			RequestHandler handler(server_configs);
                    			std::string response = handler.handleRequest(request, fds[i].fd);

                    			poll(fds, nfds, -1);
                    			if (fds[i].revents & POLLOUT) 
					{
                        			send(fds[i].fd, response.c_str(), response.length(), 0);
                    			}

                    			if (!keep_alive) 
					{
                        			close(fds[i].fd);
                        			fds[i].fd = -1;
                    			}
                		}
            		}
        	}
    	}
}

