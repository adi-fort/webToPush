NAME = webserv

SRC = src/main.cpp src/Server.cpp src/HttpRequest.cpp src/RequestHandler.cpp src/ConfigParser.cpp

OBJ = $(SRC:.cpp=.o)

GCC = c++

FLAGS = -Wall -Werror -Wextra -std=c++98


all: $(NAME)

$(NAME): $(OBJ)
	$(GCC) $(FLAGS) -o $(NAME) $(OBJ)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all
