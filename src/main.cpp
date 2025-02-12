#include "Server.hpp"

int main(int argc, char **argv)
{
    std::string configFile = (argc > 1) ? argv[1] : "config.conf";
    Server server(configFile);
    server.init();
    server.run();
    return 0;
}

