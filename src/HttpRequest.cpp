#include "HttpRequest.hpp"
#include <iostream>
#include <cstdlib>
#include <sys/socket.h>
#include <cctype>
#include <sstream>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <cstring>
#include <vector>


HttpRequest::HttpRequest()
    : method(""), path(""), protocol(""), body(), host("127.0.0.1"), port(8080) {}


HttpRequest::~HttpRequest() {}

