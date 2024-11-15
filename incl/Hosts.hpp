#ifndef HOSTS_HPP
# define HOSTS_HPP

# include "webserv.hpp"

class Request;
class Response;

class Host: public ServerConf {
	public:
		Host(void);
		Host(ServerConf & src);
		~Host(void);

		std::vector<struct epoll_event>	_events;
		std::vector<std::string>		_files;
		std::map<int, Request>			_requests;
		std::map<int, Response>			_responses;
		std::vector<int>				_partials;
		int								_nfds;
		time_t							_keep_alive_time;

		void							new_connection(void);
		void							parse_request(int fd);
		void							act_on_request(int fd);
		void							run_server(void);
		void							close_everything(void);
		void							json_update(void);
		void							manage_connection(int i, int fd, std::map<int, Request>::iterator	it_req);
		void							prepare_next_iteration(void);
};

#endif