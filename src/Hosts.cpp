#include "webserv.hpp"

Host::Host(ServerConf & src): ServerConf(src), _nfds(0) {

	struct epoll_event	event;
	std::ostringstream	oss;

	// Create listening socket
	if ((_fdSetSock = socket(_address.sin_family, SOCK_STREAM, 0)) <= 0) {
		oss << "In the socket creation for server " << _nbServer;
		throw ErrorFdManipulation(oss.str(), ERR_CODE_INTERNAL_ERROR);
	}
	
	// Set socket options for reusing the address
	int	opt = 1;
	if (setsockopt(_fdSetSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(int)) < 0) {
		ft_close(_fdSetSock);
		oss << "In the setsockopt for server " << _nbServer << " and socket " << _fdSetSock;
		throw ErrorFdManipulation(oss.str(), ERR_CODE_INTERNAL_ERROR);
	}
	
	// Set socket details (bind)
	if (bind(_fdSetSock, (struct sockaddr *)&_address, sizeof(_address)) < 0) {
		ft_close(_fdSetSock);
		oss << "In the bind for server " << _nbServer << "with port " << _port << " and socket " << _fdSetSock;
			throw ErrorFdManipulation(oss.str(), ERR_CODE_INTERNAL_ERROR);
	}
	
	// Listen on the socket
	if (listen(_fdSetSock, MAX_CONNECTIONS) < 0) {
		ft_close(_fdSetSock);
		throw ErrorFdManipulation("In the listen", ERR_CODE_INTERNAL_ERROR);
	}
	
	// Make the socket non-blocking
	if (fcntl(_fdSetSock, F_SETFL, O_NONBLOCK) < 0) {
		ft_close(_fdSetSock);
		oss << "In the listen for server " << _nbServer << " and socket " <<_fdSetSock;
			throw ErrorFdManipulation(oss.str(), ERR_CODE_INTERNAL_ERROR);
	}
	
	// Create epoll instance
	_fdEpoll = epoll_create(1);
	if (_fdEpoll < 0) {
		ft_close(_fdSetSock);
		oss << "In the epoll_create for server " << _nbServer << " and socket " <<_fdSetSock;
		throw ErrorFdManipulation(oss.str(), ERR_CODE_INTERNAL_ERROR);
	}
	event.events = EPOLLIN;
	event.data.fd = _fdSetSock;
	if (epoll_ctl(_fdEpoll, EPOLL_CTL_ADD, _fdSetSock, &event) < 0)
		throw ErrorFdManipulation("In epoll_ctl for listening socket: " + static_cast<std::string>(strerror(errno)), ERR_CODE_INTERNAL_ERROR);

	// Resize the events vector
	_events.resize(MAX_CONNECTIONS);

	// Add fds to the global list
	g_fds.push_back(_fdSetSock);
	g_fds.push_back(_fdEpoll);
}

void	Host::new_connection(void) {
	int					res;
	std::ostringstream	oss;
	std::cout << MAGENTA BOLD "New connection detected" RESET << std::endl;

	// Accept the connection if possible
	if (_connections.size() >= MAX_CONNECTIONS) {
		oss << "In the accept for server " << _nbServer << " and socket " << _fdSetSock << ": too many connections";
		throw ErrorFdManipulation(oss.str(), ERR_CODE_INTERNAL_ERROR);
	}
	res = accept(_fdSetSock, (struct sockaddr *)&_address, (socklen_t*)&_address_len);
	if (res < 0) {
		ft_close(_fdSetSock);
		oss << "In the accept for server " << _nbServer << " and socket " << _fdSetSock << ": " << strerror(errno);
		throw ErrorFdManipulation(oss.str(), ERR_CODE_INTERNAL_ERROR);
	}

	// Make the connection non-blocking
	if (fcntl(res, F_SETFL, O_NONBLOCK) < 0) {
		ft_close(_fdSetSock);
		ft_close(res);
		oss << "In the fcntl for server " << _nbServer << " and socket " << res << ": " << strerror(errno);
		throw ErrorFdManipulation(oss.str(), ERR_CODE_INTERNAL_ERROR);
	}

	// Add new connection to epoll if possible
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = res;
	if (epoll_ctl(_fdEpoll, EPOLL_CTL_ADD, res, &event) < 0)
		throw ErrorFdManipulation("In the epoll_ctl: " + static_cast<std::string>(strerror(errno)), ERR_CODE_INTERNAL_ERROR);

	// Add fd and time to the global list
	g_fds.push_back(event.data.fd);
	_connections.insert(std::pair<int, int>(event.data.fd, time(NULL)));
	_nb_keepalive++;
}

void	Host::parse_request(int i) {
	char								buffer[BUFFER_SIZE] = { 0 };
	int									valread, fd = _events[i].data.fd;
	std::map<int, Request>::iterator	it = _requests.find(fd);

	// Read data from client
	try {
		valread = read(fd, buffer, BUFFER_SIZE - 1);
		if (valread < 0 && it != _requests.end() && it->second._stage != HEADERS_DONE)
			throw ErrorFdManipulation("In the read", ERR_CODE_INTERNAL_ERROR);
		else if (valread < 0)
			valread = 0;
	} catch (const ErrorFdManipulation & e) {
		if (it != _requests.end())
			return send_error_page(*this, i, e, it->second._request_line["uri"]);
		else
			return send_error_page(*this, i, e, "");
	}
	if (it != _requests.end())
		it->second.append(buffer, valread);
	else {
		std::cout << YELLOW BOLD "New request detected" RESET << std::endl;
		Request tmp(*this, _events[i], static_cast<std::string>(buffer));
		_requests.insert(std::pair<int, Request>(fd, tmp));
		it = _requests.find(fd);
	}

	// Parse the partial read of the request
	try {
		if (valread == 0)
			it->second._eof = 0;
		else
			it->second._eof = valread;
		it->second.parse();
		if (it->second._chunked)
			_connections[fd] = time(NULL);
	} catch (const ErrorRequest & e) {
		send_error_page(*this, i, e, it->second._request_line["uri"]);
	}
}

void	Host::act_on_request(int i) {
	bool								done = false;
	int									fd = _events[i].data.fd;
	std::map<int, Response>::iterator	it_resp = _responses.find(fd);
	std::map<int, Request>::iterator	it_req = _requests.find(fd);

	// Build the response if it doesn't exist
	if (it_resp == _responses.end()) {
		Response tmp(it_req->second, *this);
		_responses.insert(std::pair<int, Response>(fd, tmp));
		it_resp = _responses.find(fd);
	}
	try {
		if (!it_resp->second._response_ready) {
			if ((it_resp->second._request_line["uri"].find(".py")!= std::string::npos 
			|| it_resp->second._request_line["uri"].find(".php")!= std::string::npos))
				it_resp->second.buildCgi();	
			else if (it_resp->second._request_line["method"] == "GET")
				it_resp->second.buildGet();
			else if (it_resp->second._request_line["method"] == "POST")
				it_resp->second.buildPost();
			else if (it_resp->second._request_line["method"] == "DELETE")
				it_resp->second.buildDelete();
			else
				throw ErrorResponse("In the request: method not implemented", ERR_CODE_FORBIDDEN);
			json_update();
		}

		// Send the response
		if (it_resp->second._response_ready) {
			it_resp->second.send_response(fd, &done);
			if (!done)
				return ;
		}

		// Close the connection if needed
		manage_connection(i, fd, it_req);

	} catch (const ErrorResponse & e) {
		send_error_page(*this, i, e, it_req->second._request_line["uri"]);
	}
	
	std::cout << YELLOW "---> Request answered" RESET << std::endl << std::endl;
}

void	Host::run_server(void) {
	std::map<int, Request>::iterator	it;

	// Wait for events
	_nfds = epoll_wait(_fdEpoll, _events.data(), _events.size(), 0);
	if (_nfds < 0) {
		std::ostringstream oss;
		oss << "In the epoll_wait for server " << _nbServer << " and socket " << _fdSetSock << ": " << strerror(errno);
		throw ErrorFdManipulation(oss.str(), ERR_CODE_INTERNAL_ERROR);
	}

	// Handle events
	for (int i = 0; i < _nfds; ++i) {
		if (g_sig)
			break ;
		if (_events[i].events == EPOLLIN) {
			// New connection
			if (_events[i].data.fd == _fdSetSock)
				try {
					new_connection();
				} catch (std::exception & e) {
					ft_perror(e.what());
				}

			// Existing connection
			else {
				parse_request(i);
				// Change the event to EPOLLOUT
				it = _requests.find(_events[i].data.fd);
				if (it != _requests.end() && it->second._stage == BODY_DONE) {
					std::cout << YELLOW "New request cought" RESET << std::endl;
					print_request(it->second._request_line, it->second._headers, it->second._body);
					std::cout << std::endl;
					_events[i].events = EPOLLOUT;
					epoll_ctl(_fdEpoll, EPOLL_CTL_MOD, _events[i].data.fd, &_events[i]);
				}
			}
		} else if (_requests.find(_events[i].data.fd) != _requests.end() && _events[i].events == EPOLLOUT)
			act_on_request(i);
		else {
			ft_close(_events[i].data.fd);
			epoll_ctl(_fdEpoll, EPOLL_CTL_DEL, _events[i].data.fd, NULL);
		}
	}

	// Reset the events vector & check for keep alive to close
	prepare_next_iteration();
}

void	Host::close_everything(void) {
	// Remove all files
	for (size_t i = 0; i < _files.size(); i++)
		unlink((_rootPath + "/uploads/" + _files[i]).c_str());
	
	// Clear files.json
	std::ofstream file((_rootPath + "/uploads/files.json").c_str());
	file << "[]";
	file.close();
	
	// Close all sockets
	for (std::map<int, time_t>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		ft_close(it->first);
	ft_close(_fdSetSock);
	ft_close(_fdEpoll);
	for (size_t i = 0; i < _events.size(); i++)
		ft_close(_events[i].data.fd);
}