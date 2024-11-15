#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "webserv.hpp"

class Host;

class Request {
	public:
		Request(Host & host, struct epoll_event & event, std::string const & raw);
		Request(Request const & src);
		~Request(void);

		void								parse();
		void								append(const char * buffer, int valread);
		void								pnc_request_line(std::istringstream & iss);
		void								pnc_headers(std::istringstream & iss);
		void								pnc_body(void);
		void								pnc_check_chunk();
		std::vector<char>					pnc_clean_chunk();

		Host &								_host;
		struct epoll_event &				_event;
		std::string							_raw;
		std::string							_body_info;
		std::string							_boundary;
		std::string							_body;
		std::string							_filename;
		std::map<std::string, std::string>	_request_line;
		std::map<std::string, std::string>	_headers;
		std::vector<char>					_binary_body;
		int									_stage;
		int									_eof;
		bool								_chunked;
};

#endif