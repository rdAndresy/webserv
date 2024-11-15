# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: bdelamea <bdelamea@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/01/04 16:22:18 by npaolett          #+#    #+#              #
#    Updated: 2024/11/15 13:14:02 by bdelamea         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #


NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -MMD -std=c++98 -g

INC_PATH = ./incl/
INC = -I $(INC_PATH)

SRCS_PATH = ./src/
SRC := \
	cgi.cpp \
	errors.cpp \
	Hosts.cpp \
	Hosts_utils.cpp \
	Location.cpp \
	main.cpp \
	Request.cpp \
	Request_utils.cpp \
	Response.cpp \
	ServerConf.cpp \
	utils.cpp \

SRCS = $(addprefix $(SRCS_PATH), $(SRC))

OBJS_PATH = ./.build/
OBJ = $(SRC:.cpp=.o)
OBJS = $(addprefix $(OBJS_PATH), $(OBJ))

DEPS = $(OBJS:.o=.d)

MAKEFLAGS   += --silent --no-print-directory # remove if you want to see the compilation

all: $(OBJS_PATH) $(NAME)

$(OBJS_PATH):
	mkdir -p $(OBJS_PATH)

$(OBJS_PATH)%.o: $(SRCS_PATH)%.cpp
		${CXX} ${CXXFLAGS} -c $< -o $@ $(INC)

$(NAME) : $(OBJS)
		$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(INC)
		$(info CREATED $(NAME))

-include $(DEPS)

clean:
		rm -rf $(OBJS_PATH)

fclean:
		rm -rf $(NAME) $(OBJS_PATH)
	
re: fclean
		make all

.PHONY: all clean fclean re
