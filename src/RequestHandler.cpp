#include "RequestHandler.hpp"
#include "ServerConfig.hpp"
#include "RouteConfig.hpp"
#include "Server.hpp"
#include "FileInfo.hpp"

#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <cstring>
#include <cstdlib> 
#include <cstdlib>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <algorithm> 
#include <filesystem> 


RequestHandler::~RequestHandler() {}

RequestHandler::RequestHandler(const std::map<std::string, ServerConfig>& server_configs) : server_configs(server_configs) {}


void RequestHandler::logServerConfig() const 
{
    	for (const auto& [key, server] : server_configs) 
	{
        	for (const auto& [route, config] : server.routes) 
		{
            		std::cout << " - Route " << route << " has upload_path: " << config.upload_path << "\n";
        	}
    	}
}


const ServerConfig* RequestHandler::findServerConfig(const std::string& host, int port) const 
{
    	std::string normalized_host = host;
    	if (host == "localhost") 
	{
        	normalized_host = "127.0.0.1";
    	}

    	std::string search_key = normalized_host;
    	if (normalized_host.find(":") == std::string::npos) 
	{
        	search_key += ":" + std::to_string(port);
	}

    	auto it = server_configs.find(search_key);
    	if (it != server_configs.end()) 
        	return &it->second;

    	return nullptr;
}


const RouteConfig* RequestHandler::findRouteConfig(const ServerConfig& server, const std::string& path) const 
{
    	auto it = server.routes.find(path);
    	if (it != server.routes.end()) 
        	return &it->second;

    	return nullptr;
}


std::string RequestHandler::handleRequest(const HttpRequest& request, int client_fd) 
{
	const ServerConfig* serverConfig = findServerConfig(request.host, request.port);
	if (serverConfig == nullptr) 
    		return generateErrorResponse(502);
	if (request.path.empty()) 
        	return generateErrorResponse(500);

    	//
	const RouteConfig* routeConfig = findRouteConfig(*serverConfig, request.path);

    	if (routeConfig) 
    	{
        	if (!routeConfig->http_redirection.empty()) 
        	{
            		return "HTTP/1.1 301 Moved Permanently\r\n"
                   	"Location: " + routeConfig->http_redirection + "\r\n"
                   	"Content-Length: 0\r\n\r\n";
        	}
    	}	
	//	
	std::string response;
	
	if (request.method == "POST" && request.headers.find("Content-Type") != request.headers.end()) 
    	{
        	if (request.headers.at("Content-Type") == "application/x-www-form-urlencoded") 
        	{
            		std::string body(request.body.begin(), request.body.end());
            		if (body.find("_method=DELETE") != std::string::npos) 
            		{
                		return handleDeleteRequest(request);
            		}
        	}
    	}
    	
	if (request.path == "/upload" && request.method == "POST") 
	{
        	response = handleUploadRequest(client_fd, request);
	}
	else if (request.path == "/upload.html" && request.method == "GET") 
	{
        	response = serveStaticFile("/upload.html");
    	} 
	else if (request.path == "/uploads" && request.method == "GET") 
	{
        	response = listUploadedFiles();
    	} 
	else if (request.path.find("/cgi-bin/") == 0) 
	{
        	response = handleCgiRequest(request);
    	} 
	else if (request.method == "GET") 
	{
		response = serveStaticFile(request.path);
    	} 
	else if (request.method == "POST") 
	{
        	response = handlePostRequest(request);
    	} 
	else if (request.method == "DELETE") 
	{
        	response = handleDeleteRequest(request);
    	} 
	else 
	{
        	response = generateErrorResponse(405);
    	}

    	if (response.empty()) 
	{
        	std::cerr << "ERROR: Response is empty for path: " << request.path << std::endl;
        	response = generateErrorResponse(500);
    	}

	return response;
}


std::string RequestHandler::decodeUrl(const std::string &url)
{
    	std::string result;
    	char hex[3] = {0};
    	for (size_t i = 0; i < url.length(); i++) 
    	{
        	if (url[i] == '%') 
        	{
            		if (i + 2 < url.length()) 
            		{
                		hex[0] = url[i + 1];
                		hex[1] = url[i + 2];
                		result += static_cast<char>(strtol(hex, nullptr, 16));
                		i += 2;
            		}
        	} 
        	else if (url[i] == '+') 
        	{
            		result += ' ';
        	} 
        	else 
        	{
            		result += url[i];
        	}
    	}
    	return result;
}




std::string RequestHandler::handleCgiRequest(const HttpRequest& request) 
{
    	std::string path = request.path;
	if (path.find("/cgi-bin/") == 0)
        	path = path.substr(9);
    	if (access(("cgi-bin/" + path).c_str(), F_OK) == -1)
        	return generateErrorResponse(404);
    	if (access(("cgi-bin/" + path).c_str(), X_OK) == -1)
        	return generateErrorResponse(403);
    	FILE* pipe = popen(("cgi-bin/" + path).c_str(), "r");
    	if (!pipe)
        	return generateErrorResponse(500);
    	std::ostringstream output;
    	char buffer[128];
    	while (fgets(buffer, sizeof(buffer), pipe) != nullptr) 
	{
        	output << buffer;
    	}
    	fclose(pipe);
    	
	std::string response = "HTTP/1.1 200 OK\r\n";
    	response += "Content-Type: text/html\r\n";
    	response += "Content-Length: " + std::to_string(output.str().size()) + "\r\n";
    	response += "\r\n";
    	response += output.str();

    	return response;
}

std::vector<char *> RequestHandler::buildCgi(const HttpRequest& request)
{
	std::vector<std::string> env_strings;

    	env_strings.push_back("REQUEST_METHOD=" + request.method);
    	env_strings.push_back("PATH_INFO=" + request.path);
    	env_strings.push_back("CONTENT_LENGTH=" + std::to_string(request.body.size()));
    	env_strings.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
    	env_strings.push_back("SERVER_PROTOCOL=" + request.protocol);

    	for (const auto& header : request.headers) 
    	{
        	std::string env_name = "HTTP_" + header.first;
        	for (size_t i = 0; i < env_name.size(); ++i) 
        	{
            		if (env_name[i] == '-') 
                	env_name[i] = '_';
            	else 
                	env_name[i] = toupper(env_name[i]);
        	}
        	env_strings.push_back(env_name + "=" + header.second);
    	}

    	std::vector<char*> envp;
    	for (size_t i = 0; i < env_strings.size(); ++i) 
   	{
        	envp.push_back(const_cast<char*>(env_strings[i].c_str()));
    	}
    	envp.push_back(NULL);

    	return envp;
}


typedef std::vector<unsigned char> ByteVector;

ByteVector::const_iterator findInVector(const ByteVector& haystack, const std::string& needle, ByteVector::const_iterator start) 
{
    	return std::search(start, haystack.end(), needle.begin(), needle.end());
}

ByteVector extractSubVector(const ByteVector& vec, ByteVector::const_iterator start, ByteVector::const_iterator end) 
{
    	return ByteVector(start, end);
}


std::string RequestHandler::handlePostRequest(const HttpRequest& request) 
{
    	if (request.body.empty()) 
    	{
        	std::cerr << "Request body is empty!" << std::endl;
        	return generateErrorResponse(400);
    	}

    	std::string content_disposition;
    	ByteVector::const_iterator content_disposition_pos = findInVector(request.body, "Content-Disposition: form-data;", request.body.begin());
    	if (content_disposition_pos != request.body.end()) 
    	{
        	ByteVector::const_iterator start_pos = findInVector(request.body, "filename=\"", content_disposition_pos);
        	if (start_pos != request.body.end()) 
		{
            		start_pos += std::strlen("filename=\"");
            		ByteVector::const_iterator end_pos = std::find(start_pos, request.body.end(), '\"');
            		if (end_pos != request.body.end()) 
			{
                		ByteVector content_disposition_vec = extractSubVector(request.body, start_pos, end_pos);
                		content_disposition = std::string(content_disposition_vec.begin(), content_disposition_vec.end());
            		}
        	}
    	}

    	if (content_disposition.empty()) 
        	return generateErrorResponse(400);

    	std::string file_name = content_disposition;
    	std::string file_path = "uploads/" + file_name;

    	std::ofstream out_file(file_path.c_str(), std::ios::binary);
    	if (out_file.is_open()) 
    	{
        	out_file.write(reinterpret_cast<const char*>(request.body.data()), request.body.size());
        	out_file.close();
    	} 
    	else 
        {	
		return generateErrorResponse(500);
	}
    	
	std::string response = "HTTP/1.1 200 OK\r\n";
    	response += "Content-Length: " + std::to_string(file_name.size()) + "\r\n";
    	response += "Content-Type: text/plain\r\n\r\n";
    	response += "Received file: " + file_name;

    	return response;
}


ByteVector::const_iterator findBoundary(const ByteVector& body, const std::string& boundary) 
{
    	return std::search(body.begin(), body.end(), boundary.begin(), boundary.end());
}


FileInfo extractFileInfo(const ByteVector& body, const std::string& boundary) 
{
    	FileInfo fileInfo;

    	ByteVector::const_iterator boundary_pos = findBoundary(body, boundary);
    	if (boundary_pos == body.end()) 
	{
        	throw std::runtime_error("Boundary not found in body!");
    	}

    	ByteVector::const_iterator filename_pos = findBoundary(body, "filename=\"");
    	if (filename_pos == body.end()) 
	{
        	throw std::runtime_error("Filename not found!");
    	}
    	filename_pos += 10;

    	ByteVector::const_iterator filename_end = std::find(filename_pos, body.end(), '\"');
    	fileInfo.filename = std::string(filename_pos, filename_end);

    	ByteVector::const_iterator content_start = findBoundary(body, "\r\n\r\n");
    	if (content_start == body.end()) 
	{
        	throw std::runtime_error("File start offset not found!");
    	}
    	content_start += 4;

    	ByteVector::const_iterator content_end = findBoundary(body, boundary);
	if (content_end == body.end()) 
	{
        	throw std::runtime_error("File end offset not found!!");
    	}
    	
	content_end -= 2;

    	fileInfo.file_start = content_start;
    	fileInfo.file_end = content_end;

    	return fileInfo;
}


std::string RequestHandler::handleUploadRequest(int client_fd, const HttpRequest& request) 
{
    	try 
	{
        	if (request.method != "POST")
            		return generateErrorResponse(405);

        	auto it = request.headers.find("Content-Type");
        	if (it == request.headers.end()) 
            		return generateErrorResponse(400);
        
       		size_t boundary_pos = it->second.find("boundary=");
        	if (boundary_pos == std::string::npos) 
            		return generateErrorResponse(400);
        
        	std::string boundary = "--" + it->second.substr(boundary_pos + 9);

        	ByteVector FiletoDisplay;

        	ByteVector::const_iterator file_start = std::search(request.body.begin(), request.body.end(), "\r\n\r\n", "\r\n\r\n" + 4);
        	
		if (file_start == request.body.end()) 
		{
            		throw std::runtime_error("Error file start not found!");
        	}
        	file_start += 4;

        	ByteVector::const_iterator file_end = std::search(file_start, request.body.end(), boundary.begin(), boundary.end());
        	if (file_end == request.body.end()) 
            		throw std::runtime_error("Error: file end not found!");
        	file_end -= 2;  

        	FiletoDisplay.assign(file_start, file_end);

        	std::string upload_dir = "/home/andrea/Scrivania/lastweb-main/uploads";
        	if (!std::filesystem::exists(upload_dir)) 
		{
            		std::filesystem::create_directory(upload_dir);
        	}
	
		const FileInfo& fileInfo = request.files[0];
		std::string filename = fileInfo.filename;
	
        	std::string file_path = upload_dir + "/" + filename;
        	std::ofstream out_file(file_path, std::ios::binary);
        	if (!out_file.is_open()) 
		{
            		return generateErrorResponse(500);
        	}

        	out_file.write(reinterpret_cast<const char*>(FiletoDisplay.data()), FiletoDisplay.size());
        	out_file.close();

       		std::string response_body = "<html><body><h2>File uploaded successfully!</h2></body></html>";


		std::string response = "HTTP/1.1 200 OK\r\n";
		response += "Content-Length: " + std::to_string(response_body.size()) + "\r\n";
		response += "Content-Type: text/html\r\n";
		response += "\r\n";
		response += response_body;
        
		return response;

    	} 
	catch (const std::exception& e) 
	{
        	std::cerr << "ERROR: " << e.what() << std::endl;
        	return generateErrorResponse(400);
    	}
}


std::string RequestHandler::listUploadedFiles() 
{
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";

    std::string body = "<html><body><h1>Uploaded Files</h1>";

    std::vector<std::string> files = getUploadedFiles();
    for (const auto& file : files) 
    {
        body += "<a href='/uploads/" + file + "'>" + file + "</a><br>";
        body += "<form action='/uploads/" + file + "' method='POST'>";
        body += "<input type='hidden' name='_method' value='DELETE'>";
        body += "<input type='submit' value='Delete this file'>";
        body += "</form><br>";
    }

    body += "</body></html>";

    response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    response += "\r\n";
    response += body;

    return response;
}


std::vector<std::string> RequestHandler::getUploadedFiles() 
{
    	std::vector<std::string> files;
    	for (const auto& entry : std::filesystem::directory_iterator("uploads")) 
	{
        	files.push_back(entry.path().filename().string());
    	}
    	return files;
}

std::string RequestHandler::handleDeleteRequest(const HttpRequest& request) 
{
    	std::string file_path = request.path;

    	if (file_path.find("/uploads/") != 0) 
    	{
        	return "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
    	}

    	std::string filename = file_path.substr(9);
    	filename = decodeUrl(filename);

    	std::string full_path = "/home/andrea/Scrivania/lastweb-main/uploads/" + filename;

    	if (!std::filesystem::exists(full_path)) 
    	{
        	std::cerr << "ERROR: File to delete not found: " << full_path << std::endl;
        	return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    	}

    	if (!std::filesystem::remove(full_path)) 
    	{
        	std::cerr << "ERROR: Failed to remove file: " << full_path << std::endl;
        	return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
    	}

    	std::string message = "File deleted successfully";
	std::string response = "HTTP/1.1 200 OK\r\n";
	response += "Content-Length: " + std::to_string(message.size()) + "\r\n";
	response += "Content-Type: text/plain\r\n\r\n";
	response += message;
    	return response;
}


std::string RequestHandler::serveStaticFile(const std::string& path) 
{
	std::string basePath;
	struct stat buffer;
	
	std::cout << path << std::endl;
	
	if (path.find("/uploads", 0) == 0)
		basePath = "/home/andrea/Scrivania/lastweb-main";
	else
	{
		basePath = "/home/andrea/Scrivania/lastweb-main/static";
        }
	
	std::string fullPath = basePath + path;
	
	std::cout << "DEBUG: Attempting to serve file at: " << fullPath << std::endl;
	  	
	if (stat(fullPath.c_str(), &buffer) != 0) 
	{
        	std::cerr << "ERROR: File not found: " << fullPath << std::endl;
        	return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    	}

	std::ifstream file(fullPath, std::ios::binary);
	if (!file.is_open()) 
	{
        	std::cerr << "ERROR: Impossible to open file: " << fullPath << std::endl;
        	return generateErrorResponse(404);
    	}
  
	std::vector<char> file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	
	std::string content_type = "text/html";
    	if (path.find(".css") != std::string::npos) content_type = "text/css";
    	else if (path.find(".js") != std::string::npos) content_type = "application/javascript";
    	else if (path.find(".png") != std::string::npos) content_type = "image/png";
    	else if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos) content_type = "image/jpeg";
    	else if (path.find(".gif") != std::string::npos) content_type = "image/gif";
    	else if (path.find(".json") != std::string::npos) content_type = "application/json";
    	else if (path.find(".txt") != std::string::npos) content_type = "text/plain";

 	std::ostringstream response;
    	response << "HTTP/1.1 200 OK\r\n";
    	response << "Content-Length: " << file_content.size() << "\r\n";
    	response << "Content-Type: " << content_type << "\r\n";
    	response << "Connection: keep-alive\r\n";
    	response << "\r\n";
    	response.write(file_content.data(), file_content.size());

    	return response.str();
}

std::string RequestHandler::generateErrorResponse(int errorCode) 
{
    	std::string file_path;
    	if (errorCode == 404) 
	{
        	file_path = "static/404.html";
    	} 
	else if (errorCode == 500) 
	{
        	file_path = "static/500.html";
    	} 
	else 
	{
        	return "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\n500 Internal Server Error";
    	}

    	std::ifstream file(file_path.c_str());
    	if (!file.is_open()) 
	{
        	std::string errorMessage = std::to_string(errorCode) + " Error";
        	return "HTTP/1.1 " + std::to_string(errorCode) + " Error\r\n" + "Content-Length: " + std::to_string(errorMessage.size()) + "\r\n" + "Content-Type: text/plain\r\n\r\n" + errorMessage;
    	}

    	std::ostringstream file_content;
    	file_content << file.rdbuf();

    	std::string response = "HTTP/1.1 " + std::to_string(errorCode) + " Error\r\n";
    	response += "Content-Length: " + std::to_string(file_content.str().size()) + "\r\n";
    	response += "Content-Type: text/html\r\n\r\n";
    	response += file_content.str();
   
	return response;
}
