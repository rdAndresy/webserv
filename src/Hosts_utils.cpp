#include "webserv.hpp"

Host::Host() { return ;}

Host::~Host() { return ; }

void	Host::json_update(void) {
	std::ostringstream	oss;
	std::string			json;

	oss << "[";
	for (size_t i = 0; i < _files.size(); ++i) {
		oss << "\"" << _files[i] << "\"";
		if (i < _files.size() - 1)
			oss << ",";
	}
	oss << "]";
	json = oss.str();

	std::ofstream file((_rootPath + "/uploads/files.json").c_str());
	try {
		if (!file.is_open())
			throw ErrorFdManipulation("In the opening of the file", ERR_CODE_INTERNAL_ERROR);
	} catch (const ErrorFdManipulation & e) {
		ft_perror(e.what());
		return ;
	}
	file << json;
	file.close();
}

void	Host::manage_connection(int i, int fd, std::map<int, Request>::iterator	it_req) {
	time_t	ka_time = _connections.find(fd)->second;
	if (!it_req->second._headers["Connection"].compare("close") || _nb_keepalive > _max_keepalive
		|| (difftime(time(NULL), ka_time) > KEEP_ALIVE && it_req->second._headers["Connection"].compare("keep-alive"))) {
		ft_close(fd);
		epoll_ctl(_fdEpoll, EPOLL_CTL_DEL, fd, NULL);
		_nb_keepalive--;
		_connections.erase(fd);
		std::cout << MAGENTA "---> Closing connection" RESET << std::endl;
	} else {
		if (!it_req->second._headers["Connection"].compare("keep-alive"))
			_connections.find(fd)->second = time(NULL);
		_events[i].events = EPOLLIN;
		epoll_ctl(_fdEpoll, EPOLL_CTL_MOD, fd, &_events[i]);
		std::cout << MAGENTA "---> Connection kept alive" RESET << std::endl;
	}
	_requests.erase(fd);
	_responses.erase(fd);
}

void	Host::prepare_next_iteration(void) {
	// Reset the events vector
	std::fill(_events.begin(), _events.end(), epoll_event());

	// Check for keep alive to close
	if (_connections.empty())
		return ;
	for (std::map<int, time_t>::iterator it = _connections.begin(); it != _connections.end(); ++it) {
		if (difftime(time(NULL), it->second) > KEEP_ALIVE) {
			ft_close(it->first);
			epoll_ctl(_fdEpoll, EPOLL_CTL_DEL, it->first, NULL);
			_connections.erase(it);
			_nb_keepalive--;
			std::cout << MAGENTA "---> Closing hanging connection" RESET << std::endl;
			if (_connections.empty())
				return ;
			it = _connections.begin();
		}
	}
}