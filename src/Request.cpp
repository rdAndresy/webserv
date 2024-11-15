#include "webserv.hpp"

Request::Request(Host & host, struct epoll_event & event, std::string const & raw):
		_host(host), _event(event), _raw(raw), _stage(-1), _eof(1), _chunked(false) {
	if (raw =="\n")
		_raw = "";
}

Request::Request(Request const & src): _host(src._host), _event(src._event),
		_raw(src._raw), _request_line(src._request_line), _headers(src._headers),
		_stage(src._stage), _eof(src._eof), _chunked(src._chunked) {

	for (size_t i = 0; i < src._binary_body.size(); i++)
		_binary_body.push_back(src._binary_body[i]);
}

Request::~Request(void) { return ; }

void	Request::append(const char * buffer, int valread) {
	if (_stage == HEADERS_DONE)
		_binary_body.insert(_binary_body.end(), buffer, buffer + valread);
	else
		_raw += std::string(buffer, valread);
}

void	Request::pnc_request_line(std::istringstream & iss) {
	std::map<std::string, Location>::const_iterator it;
	std::string	line, path_uri;

	// Skip the empty lines
	while (std::getline(iss, line, '\n') && line == "\r");
	
	// Check again if there is a whole line
	if (line.find("\r") == std::string::npos)
		return ;

	// Parse the request line
	std::istringstream	iss_line(line);
	if (!(std::getline(iss_line, _request_line["method"], ' ') && std::getline(iss_line, _request_line["uri"], ' ') && std::getline(iss_line, _request_line["protocol"], ' ')))
		throw ErrorRequest("In the request: method not well formatted", ERR_CODE_BAD_REQUEST);

	// Check the uri validity
	if (_request_line["uri"].empty())
		throw ErrorRequest("In the request: uri not found", ERR_CODE_BAD_REQUEST);

	// Check if method is generally allowed
	if (!(_request_line["method"] == "GET" || _request_line["method"] == "POST" || _request_line["method"] == "DELETE"))
		throw ErrorRequest("In the request: method not supported", ERR_CODE_MET_NOT_ALLOWED);

	// Check if method is allowed for the location
	if (_host._Location.size() == 0)
		return ;
	
	path_uri = foundPathUri(_host, _request_line["uri"]);
	it = _host._Location.find(path_uri);

	// Check methods if location is found
	if (it != _host._Location.end()) {
		if (!it->second.getCgiAllow() && _request_line["uri"].find(path_uri) != std::string::npos &&  _request_line["uri"].find("/cgi") != std::string::npos)
			throw ErrorRequest("In the request: CGI is not allow", ERR_CODE_FORBIDDEN);

		if (_request_line["method"] == "GET" && !it->second.getFlagGet() && _request_line["uri"].find(path_uri) != std::string::npos)
			throw ErrorRequest("In the request: method GET is not allow", ERR_CODE_MET_NOT_ALLOWED);

		if (_request_line["method"] == "POST" && !it->second.getFlagPost() && _request_line["uri"].find(path_uri) != std::string::npos)
			throw ErrorRequest("In the request: method POST is not allow", ERR_CODE_MET_NOT_ALLOWED);
		
		if (_request_line["method"] == "DELETE" && !it->second.getFlagDelete() && _request_line["uri"].find(path_uri) != std::string::npos)
			throw ErrorRequest("In the request: method DELETE is not allow", ERR_CODE_MET_NOT_ALLOWED);
	}

	// Check the protocol
	if (_request_line["protocol"] != "HTTP/1.1\r")
		throw ErrorRequest("In the request: protocol not supported", ERR_CODE_HTTP_VERSION);
}

void	Request::pnc_headers(std::istringstream & iss) {
	std::ostringstream	oss;
	std::string			line, buffer, key, value;

	while (std::getline(iss, line, '\n') && line != "\r") {
		if (line[line.size() - 1] == '\r') {
			buffer.append(line);
		} else {
			buffer = line;
			continue ;
		}
		
		std::istringstream	iss_line(line.erase(buffer.size() - 1));
		
		if (!(std::getline(iss_line, key, ':') && std::getline(iss_line, value)))
			throw ErrorRequest("In the request: header not well formatted", ERR_CODE_BAD_REQUEST);
		
		// Remove leading and trailing whitespaces
		_headers[trim(key)] = trim(value);
		buffer.clear();
	}
	
	if (_headers.empty())
		throw ErrorRequest("In the request: end of headers not found", ERR_CODE_BAD_REQUEST);

	// Check host and retrieve the localhost ip if needed
	oss << _host._port;
	if (_headers["Host"] == "localhost:" + oss.str() || _headers["Host"] == "localhost")
		_headers["Host"] = collect_lh_ip() + ":" + oss.str();
	
	if (!_host._name.empty() 
		&& (_headers.find("Host") == _headers.end()
		|| (_headers["Host"] != _host._name && _headers["Host"] != _host._raw_ip + ":" + oss.str() && _headers["Host"] != _host._name + ":" + oss.str())))
		throw ErrorRequest("In the request: host error", ERR_CODE_NOT_FOUND);

	// Check content for POST
	if (_request_line["method"] == "POST") {
		if (_headers.find("Transfer-Encoding") == _headers.end()) {
			if (_headers.find("Content-Length") == _headers.end())
				throw ErrorRequest("In the request: content-length not found", ERR_CODE_LENGTH_REQUIRED);
			if (_headers["Content-Length"].empty() || _headers["Content-Length"].find_first_not_of("0123456789") != std::string::npos)
				throw ErrorRequest("In the request: content-length mismatch", ERR_CODE_BAD_REQUEST);
		} else if (_headers["Transfer-Encoding"].empty())
			throw ErrorRequest("In the request: transfer-encoding mismatch", ERR_CODE_BAD_REQUEST);
		else if (_headers["Transfer-Encoding"] != "chunked")
			throw ErrorRequest("In the request: transfer-encoding not supported", ERR_CODE_UNSUPPORTED_MEDIA);
		else
			_chunked = true;
		if (_headers.find("Content-Type") == _headers.end() || _headers["Content-Type"].empty())
			throw ErrorRequest("In the request: content-type not found", ERR_CODE_UNSUPPORTED_MEDIA);
		if (_headers["Content-Type"].find("multipart/form-data") == std::string::npos && _headers["Content-Type"].find("application/x-www-form-urlencoded") == std::string::npos)
			throw ErrorRequest("In the request: other than multipart data not supported", ERR_CODE_UNSUPPORTED_MEDIA);
	}

	// Check URI can be DELETED
	if (_request_line["method"] == "DELETE" && _request_line["uri"].find("uploads/") == std::string::npos)
		throw ErrorRequest("In the request: DELETE not allowed for this resource", ERR_CODE_FORBIDDEN);
}

void	Request::pnc_body(void) {
	size_t				len, len_max;
	std::string			line;

	len = _binary_body.size();
	
	// Check the body size during reading
	_chunked ? len_max = MAX_BODY_SIZE : len_max = std::atof(_headers["Content-Length"].c_str());
	if (len > MAX_BODY_SIZE + 1 || len > _host._maxBodySize + 1	|| len > len_max + 1)
		throw ErrorRequest("In the request: body too long", ERR_CODE_BAD_REQUEST);

	// Is there no body?
	if (len == 0 && _eof != BUFFER_SIZE)
		throw ErrorRequest("In the request: body not found", ERR_CODE_BAD_REQUEST);

	// Is the body complete?
	if (_eof > 0 && len != len_max && !_chunked)
		return ;
	
	// If the body is chunked, decode it and check it
	if (_chunked)
		pnc_check_chunk();

	// Is the chunk request done?
	if (_chunked) {
		if (_binary_body.size() < 5)
			return ;
		for (size_t i = 1; i <= 5; i++)
			line = _binary_body[_binary_body.size() - i] + line;
		if (line != "0\r\n\r\n")
			return ;
	}

	// Clean chunked body
	if (_chunked)
		_binary_body = pnc_clean_chunk();

	// Check body's length
	if (len != len_max && len != len_max + 1 && !_chunked) {
		std::cout << YELLOW "Body size: " << len << " | Expected: " << len_max << RESET << std::endl;
		throw ErrorRequest("In the request: body size mismatch", ERR_CODE_BAD_REQUEST);
	}

	_stage = BODY_DONE;
}

void	Request::parse() {
	std::istringstream	iss(_raw);
	long				header_len;
	std::string			line;

	// Parse the request line
	if (_stage == -1) {

		// Check if there is a whole line
		if (_raw.find("\r\n") == std::string::npos && _raw.length() < MAX_URI_SIZE + 16)
			return ;
		else if (_raw.length() >= MAX_URI_SIZE + 16)
			throw ErrorRequest("In the request: request line too long", ERR_CODE_URI_TOO_LONG);

		// Parse & Check the request line

		pnc_request_line(iss);

		// Mark the request line as done & store the end of the request line
		_stage = RL_DONE;
		if (iss.tellg() < 0)
			return ;
		_raw = _raw.substr(static_cast<std::string::size_type>(iss.tellg()));
	}
	
	// Parse the headers
	if (_stage == RL_DONE) {
		iss.clear();
		iss.str(_raw);

		// Check if there is a whole header
		header_len = 0;
		while (std::getline(iss, line, '\n') && line != "\r") {
			header_len += line.size();
			if (header_len > MAX_HEADER_SIZE)
				throw ErrorRequest("In the request: headers too long", ERR_CODE_REQ_HEADER_FIELDS);
		}
		if (header_len == 0 && line == "\r")
			throw ErrorRequest("In the request: end of headers not found", ERR_CODE_BAD_REQUEST);
		if (line != "\r")
			return ;
		
		// Parse & Check the headers
		iss.clear();
		iss.str(_raw);
		pnc_headers(iss);

		// Mark the headers as done & store the end of the request line
		_stage = HEADERS_DONE;
		if (iss.tellg() < 0)
			return ;
		_raw = _raw.substr(static_cast<std::string::size_type>(iss.tellg()));
		_binary_body.insert(_binary_body.end(), _raw.begin(), _raw.end());
	}

	// Parse the body
	if (_stage == HEADERS_DONE) {
		if (_request_line["method"] != "POST" && _binary_body.size() > 0
			&& _headers.find("Transfer-Encoding") != _headers.end())
			throw ErrorRequest("In the request: body forbiddent for method", ERR_CODE_BAD_REQUEST);
		else if (_request_line["method"] == "POST")
			pnc_body();
		else
			_stage = BODY_DONE;
	}
}