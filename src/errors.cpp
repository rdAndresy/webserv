#include "webserv.hpp"

ErrorConfFile::ErrorConfFile(std::string msg) throw(): _errorMsg(msg) { return ; }
ErrorConfFile::~ErrorConfFile() throw() { return ; }
const char *ErrorConfFile::what() const throw() { return _errorMsg.c_str(); }

ErrorFdManipulation::ErrorFdManipulation(std::string msg, int code) throw(): _errorMsg(msg), _code(code) { return ; }
ErrorFdManipulation::~ErrorFdManipulation() throw() { return ; }
const char *ErrorFdManipulation::what() const throw() {
	std::ostringstream	oss;

	oss << _code;
	_errorMsg += " | Response code: ";
	_errorMsg += oss.str();
	return _errorMsg.c_str();
}

ErrorRequest::ErrorRequest(std::string msg, int code) throw(): _errorMsg(msg), _code(code) { return ; }
ErrorRequest::~ErrorRequest() throw() { return ; }
const char *ErrorRequest::what() const throw() {
	std::ostringstream oss;

	oss << _code;
	_errorMsg += " | Response code: ";
	_errorMsg += oss.str();
	return _errorMsg.c_str();
}

ErrorResponse::ErrorResponse(std::string msg, int code) throw(): _errorMsg(msg), _code(code) { return ; }
ErrorResponse::~ErrorResponse() throw() { return ; }
const char *ErrorResponse::what() const throw() {
	std::ostringstream oss;

	oss << _code;
	_errorMsg += " | Response code: ";
	_errorMsg += oss.str();
	return _errorMsg.c_str();
}

void	ft_perror(const char * message) { std::cerr << BOLD RED "Error: " RESET RED << message << RESET << std::endl; }

std::string getStatus(const int & code) {
	std::map<int, std::string> errorMap;
	errorMap.insert(std::make_pair(ERR_CODE_MOVE_PERM, ERR_NAME_MOVE_PERM));
	errorMap.insert(std::make_pair(ERR_CODE_BAD_REQUEST, ERR_NAME_BAD_REQUEST));
	errorMap.insert(std::make_pair(ERR_CODE_FORBIDDEN, ERR_NAME_FORBIDDEN));
	errorMap.insert(std::make_pair(ERR_CODE_NOT_FOUND, ERR_NAME_NOT_FOUND));
	errorMap.insert(std::make_pair(ERR_CODE_MET_NOT_ALLOWED, ERR_NAME_MET_NOT_ALLOWED));
	errorMap.insert(std::make_pair(ERR_CODE_TIMEOUT, ERR_NAME_TIMEOUT));
	errorMap.insert(std::make_pair(ERR_CODE_CONFLICT, ERR_NAME_CONFLICT));
	errorMap.insert(std::make_pair(ERR_CODE_LENGTH_REQUIRED, ERR_NAME_LENGTH_REQUIRED));
	errorMap.insert(std::make_pair(ERR_CODE_PAYLOAD_TOO_LARGE, ERR_NAME_PAYLOAD_TOO_LARGE));
	errorMap.insert(std::make_pair(ERR_CODE_URI_TOO_LONG, ERR_NAME_URI_TOO_LONG));
	errorMap.insert(std::make_pair(ERR_CODE_UNSUPPORTED_MEDIA, ERR_NAME_UNSUPPORTED_MEDIA));
	errorMap.insert(std::make_pair(ERR_CODE_REQ_HEADER_FIELDS, ERR_NAME_REQ_HEADER_FIELDS));
	errorMap.insert(std::make_pair(ERR_CODE_INTERNAL_ERROR, ERR_NAME_INTERNAL_ERROR));
	errorMap.insert(std::make_pair(ERR_CODE_SERVICE_UNAVAIL, ERR_NAME_SERVICE_UNAVAIL));
	errorMap.insert(std::make_pair(ERR_CODE_HTTP_VERSION, ERR_NAME_HTTP_VERSION));

	std::map<int, std::string>::const_iterator it = errorMap.find(code);
	if (it != errorMap.end())
		return it->second;
	else
		return "Unknown";
}

void send_generic_error() {

}

template <typename T>
void	send_error_page(Host & host, int i, const T & e, std::string uri) {
	std::string 									status, response, body, line, image;
	std::fstream									file;
	std::ostringstream								oss, str_code, str_port;
	std::stringstream								buffer;
	std::map<int, std::string>::const_iterator		it;
	std::map<std::string, Location>::const_iterator	loc_it = host._Location.find(foundPathUri(host, uri));
	
	ft_perror(e.what());
	if (host._nb_keepalive)
		host._nb_keepalive -= 1;

	// Get the status
	status = getStatus(e._code);
		
	// Get URI for specific error code page
	uri = foundPathUri(host, uri);

	// Get error page if defined in location or at global server level
	if (loc_it != host._Location.end() && loc_it->second.getFlagErrorPages()) {
		it = loc_it->second.getPagesError().find(e._code);
		if (it != loc_it->second.getPagesError().end()) {

			std::ifstream fileRequested(it->second.c_str());
			if (fileRequested.good() == false) {
				ft_perror("In the opening of the file requested");
				str_code << ERR_CODE_NOT_FOUND;
				status = getStatus(ERR_CODE_NOT_FOUND);
				image = "<img src=\"https://http.cat/" + str_code.str() + ".jpg\">";	
				body = build_custom_page(ERR_CODE_NOT_FOUND, image);
			} else {
				buffer << fileRequested.rdbuf();
				body = buffer.str();	
			}
		}
		else if (host._errorFlag) {
			it = host.getPagesError().find(e._code);
			if (it != host.getPagesError().end()) {
				std::ifstream fileRequested(it->second.c_str());
				if (fileRequested.good() == false) {
					ft_perror("In the opening of the file requested");
					str_code << ERR_CODE_NOT_FOUND;
					status = getStatus(ERR_CODE_NOT_FOUND);
					image = "<img src=\"https://http.cat/" + str_code.str() + ".jpg\">";	
					body = build_custom_page(ERR_CODE_NOT_FOUND, image);
				} else {
					buffer << fileRequested.rdbuf();
					body = buffer.str();	
				}
			}
		}
		else {
			body = build_custom_page(e._code, image);
		}

	} else if (host._errorFlag) {
		it = host.getPagesError().find(e._code);
		if (it != host.getPagesError().end()) {
			std::ifstream fileRequested(it->second.c_str());
			if (fileRequested.good() == false) {
				str_code << ERR_CODE_NOT_FOUND;
				status = getStatus(ERR_CODE_NOT_FOUND);
				image = "<img src=\"https://http.cat/" + str_code.str() + ".jpg\">";	
				body = build_custom_page(ERR_CODE_NOT_FOUND, image);
			} else {
				buffer << fileRequested.rdbuf();
				body = buffer.str();	
			}
		}
	} else {
		str_code << e._code;
		if (status == "Unkown")
			image = "<img src=\"https://http.cat/450.jpg\">";
		else
			image = "<img src=\"https://http.cat/" + str_code.str() + ".jpg\">";	
		body = build_custom_page(e._code, image);
	}

	// Set the response
	// std::cout << body << " <<<<<< ----------- Body\n";
	oss << "HTTP/1.1 " << e._code << " " << status << "\r\n";
	if (host._name.empty()) {
		str_port << host._port;
		oss << "Server: " << host._raw_ip << ":" << str_port.str() << "\r\n";	
	} else
		oss << "Server: " << host._name << "\r\n";
	oss << "Content-Type: text/html\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "Connection: close\r\n";
	oss << "\r\n";
	oss << body;
	response = oss.str();

	int check_send = send(host._events[i].data.fd, response.c_str(), response.size(), MSG_NOSIGNAL);
	// Send the response
	if (check_send < 0)
		ft_perror(("In the send of error page: " + str_code.str()).c_str());
	else if (check_send == 0)
		ft_perror("In the send of error page: Send 0 byte to client "); 

	// Close the connection
	ft_close(host._events[i].data.fd);
	epoll_ctl(host._fdEpoll, EPOLL_CTL_DEL, host._events[i].data.fd, NULL);
	if (host._requests.find(host._events[i].data.fd) != host._requests.end()) {
		host._requests.erase(host._events[i].data.fd);
		host._responses.erase(host._events[i].data.fd);
		g_fds.erase(std::remove(g_fds.begin(), g_fds.end(), host._events[i].data.fd), g_fds.end());
	}
	if (host._connections.find(host._events[i].data.fd) != host._connections.end()) {
		std::cout << MAGENTA "---> Closing error connection" RESET << std::endl;
		host._connections.erase(host._events[i].data.fd);
	}
}

template void send_error_page<ErrorFdManipulation>(Host &, int, const ErrorFdManipulation &, std::string);
template void send_error_page<ErrorRequest>(Host &, int, const ErrorRequest &, std::string);
template void send_error_page<ErrorResponse>(Host &, int, const ErrorResponse &, std::string);