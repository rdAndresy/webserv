#include "webserv.hpp"

Response::Response(const Request & src, const Host &host): Request(src) {
	fillContentTypes();
	_response_ready = false;
	_request_line = src._request_line;
	_headers = src._headers;
	_body = src._body;
	_indexPages = host._IndexFile;
	_root = host._rootPath;
	_pagesError = host._PageError;
	_returnPages = host._CodeReturn;
	_err = 0; // 4 error 404 etc etc 
	_autoInxPrint = 0;
	_autoIndex = host._Autoindex;
	_found = 0;
	_serverName = host._name;
	_maxBodySize= host._maxBodySize;
	_Ip = host._ip;
	_Port = host._port;
	_statusCode = 200;
	_startUri = _request_line["uri"];

	if (_root[0] == '/')
		_root = _root.substr(1,_root.size() - 1);
	if (_root[_root.size() - 1] == '/')
		_root = _root.substr(0,_root.size() - 1);
}

void	Response::send_response(int fd, bool *done) {
	int	sent;

	
	if (_response_message.size() > BUFFER_SIZE)
		sent = send(fd, _response_message.c_str(), BUFFER_SIZE, MSG_MORE | MSG_NOSIGNAL);
	else
		sent = send(fd, _response_message.c_str(), _response_message.size(), MSG_NOSIGNAL);

	if (sent == -1)
		throw ErrorResponse("In the send of the response", ERR_CODE_INTERNAL_ERROR);
	else if (sent == 0)
		throw ErrorResponse("In the send of the response: sent 0 bytes to the client", ERR_CODE_INTERNAL_ERROR);
	else
		_response_message = _response_message.substr(sent);
	
	if (_response_message.empty())
		*done = true;
	else
		return ;
}

void	Response::buildAutoindex(void) {
	std::vector<std::string> filesList;
	DIR *dir;
	struct dirent *fileRead;
	std::string hyperlink, filename, add_content("");
	std::ostringstream 	oss;

	_startUri  = _root + _startUri;
	dir = opendir(_startUri.c_str());
	if (!dir)
		throw ErrorResponse("In the opening of the directory pointer by the URI", ERR_CODE_INTERNAL_ERROR);

	while ((fileRead = readdir(dir)) != NULL)
		if (strcmp(fileRead->d_name, ".") != 0 || (strcmp(fileRead->d_name, "..") != 0 && _startUri != "/"))
			filesList.push_back(fileRead->d_name);

	_headers["content-type"] = "text/html";
	_response_body = build_custom_page(0, _startUri);

	for (std::vector<std::string>::iterator it = filesList.begin(); it != filesList.end(); it++) {
		if (*it == ".")
			continue ;
		if (*it == "..")
			filename = "<< Back to parent directory";
		else
			filename = (*it);
		_root = _root + "/";
		if (isRepertory(_root , filename) == 3)
			hyperlink = (*it) + "/";
		else
			hyperlink = (*it);
		if (it == filesList.begin() + 1)
			add_content += "<div class=\"button-container\"><a class=\"link-button back-button\" href=" + hyperlink + ">" + filename + "</a></div>\n";
		else
			add_content += "<div class=\"button-container\"><a class=\"link-button\" href=" + hyperlink + ">" + filename + "</a></div>\n";
	}
	closedir(dir);
	
	// Add content to page
	_response_body = build_custom_page(ERR_CODE_SUCCESS, add_content);
	_response_body.replace(_response_body.find("<!-- uri -->"), std::string("<!-- uri -->").length(), _startUri);

	// Add remainig headers
	oss << _response_body.size();
	_response_header.insert(std::make_pair("Content-Length",oss.str()));
	if (_response_header.find("Content-Type") == _response_header.end())
		_response_header.insert(std::make_pair("Content-Type", "text/html"));
	
	// Merge everything in the final message to send
	oss.str("");
	oss << "HTTP/1.1 " << _statusCode << " OK" << "\r\n";
	_response_message = oss.str() + "Server: " + _serverName + "\r\n";
	for (std::map<std::string, std::string>::iterator it = _response_header.begin(); it != _response_header.end(); it++)
		_response_message += it->first + ": " + it->second + "\r\n";
	_response_message += "\r\n" + _response_body;

	// Set the response ready
	_response_ready = true;
}

void	Response::buildReturnPage(void) {
	int	returnCode;
	std::string redirUri;

	for (std::map<int, std::string>::iterator i = _returnPages.begin(); i != _returnPages.end(); i++)
		(returnCode = i->first, redirUri = i->second);

	if (returnCode >= 300 && returnCode < 309)
		_statusCode = returnCode;
	else
		_statusCode = 500;
	
	std::string goodUri = foundPathUri(_host, _startUri);

	if ((_startUri.size() -1 ) == '/' && redirUri[0] == '/') {
		redirUri = redirUri.substr(1, redirUri.size());
		// _startUri += redirUri;
	}
	else if ((_startUri.size() -1 ) != '/' && redirUri[0] != '/')
		redirUri = "/" + redirUri;
	
	_startUri = goodUri + redirUri;

	_headers["location"] = _startUri;

	buildPage();
}

void	Response::buildPage(void) {
	std::stringstream	buffer;
	std::string			resourceName, fileExtension;
	size_t				pos;
	std::ostringstream 	oss;
	
	// Open the requested file
	_startUri = _root + _startUri;

	std::ifstream fileRequested(_startUri.c_str());
	if (fileRequested.good() == false)
		throw ErrorResponse("In the opening of the file requested", ERR_CODE_NOT_FOUND);
	
	buffer << fileRequested.rdbuf();
	
	_response_body = buffer.str();
	if (_response_body.size() > _maxBodySize) {
		_response_body = "";
		throw ErrorResponse("In the size of the file requested", ERR_CODE_NOT_FOUND);
	}

	pos = _startUri.find_last_of("/");
	if (pos != std::string::npos && (_startUri.begin() + pos + 1) != _startUri.end()) {
		resourceName = _startUri.substr(pos + 1);
		pos = resourceName.find_last_of(".");
		if (pos != std::string::npos && (resourceName.begin() + pos + 1) != resourceName.end()) {
			fileExtension = resourceName.substr(pos + 1);
			if (CONTENT_TYPES.find(fileExtension) != CONTENT_TYPES.end())		
				_response_header.insert(std::make_pair("Content-Type", CONTENT_TYPES[fileExtension]));
		}
	}

	// Add remainig headers
	oss << _response_body.size();
	_response_header.insert(std::make_pair("Content-Length",oss.str()));
	if (_response_header.find("Content-Type") == _response_header.end())
		_response_header.insert(std::make_pair("Content-Type", "text/html"));
	
	// Merge everything in the final message to send
	oss.str("");
	oss << "HTTP/1.1 " << _statusCode << " OK" << "\r\n";
	_response_message = oss.str();
	_response_message += "Server: " + _serverName + "\r\n";
	for (std::map<std::string, std::string>::iterator it = _response_header.begin(); it != _response_header.end(); it++)
		_response_message += it->first + ": " + it->second + "\r\n";
	_response_message += "\r\n";
	_response_message += _response_body;

	// Set the response ready
	_response_ready = true;
}

void	Response::buildGet(void) {
	std::string										index, path, uri, path_uri;
	std::map<std::string, Location>::const_iterator it;

	// Check if uri has a file to download
	if (_startUri.find("uploads/") != std::string::npos && _startUri.find("index.html") == std::string::npos)
		_startUri = replace_percentage(_startUri);

	if (_startUri.size() > 2 &&  _startUri[_startUri.size() - 1] == '/')
		uri = _startUri.substr(0, _startUri.size() - 1);
	else
		uri = _startUri;
	path_uri = foundPathUri(_host, uri);

	it = _host._Location.find(path_uri);
	if (it != _host._Location.end()) {
		if (it->second.getFlagIndex())
			_indexPages = it->second.getIndexPages();
		if (it->second.getFlagAutoInx())
			_autoIndex = it->second.getAutoIndex();
		if (it->second.getRootFlag())
			_root = it->second.getRoot() + "/";
		if (it->second.getReturnFlag())
			_returnPages = it->second.getReturnPages();
		if (it->second.getFlagErrorPages())
			_pagesError = it->second.getPagesError();
	}
	
	if (!_returnPages.empty())
		return buildReturnPage();
	
	if (isRepertory(_root, _startUri) == 3) {
		if (_startUri[_startUri.size() -1] != '/') {
			_statusCode = 301;
			std::vector<std::string> filesList;
			std::string tmp  = _root + _startUri;
			DIR *dir = opendir(tmp.c_str());
			if (!dir)
				throw ErrorResponse("Error in the opening of the directory pointer by the URI", ERR_CODE_INTERNAL_ERROR);
			struct dirent *fileRead;
			while ((fileRead = readdir(dir)) != NULL)
				if (strcmp(fileRead->d_name, ".") != 0 || (strcmp(fileRead->d_name, "..") != 0 && _startUri != "/"))
					filesList.push_back(fileRead->d_name);
			for (std::vector<std::string>::const_iterator it = filesList.begin(); it != filesList.end(); it++) {
				if (*it == "." || *it == "..")
					continue ;
				std::string file = (*it);
				if (isRepertory(_root, _startUri + "/"+ file) == 1 && file[file.size() - 5] == '.') {
					_startUri = _startUri + "/"+ (*it);
					closedir(dir);
					return buildPage();
				}
			}
			closedir(dir);
			throw ErrorResponse("Error in the request: URI is not a directory", ERR_CODE_NOT_FOUND);
		} else if (!_indexPages.empty()) {
			for (std::vector<std::string>::const_iterator it = _indexPages.begin(); it != _indexPages.end(); it++) {
				index = (*it)[0] == '/' ? (*it).substr(1, std::string::npos) : (*it);
				path = _startUri + index;
				if (isRepertory(_root, path) == 1) {
					_startUri = path;
					return buildPage();
				}
			}
			throw ErrorResponse("In the response: no index file found", ERR_CODE_NOT_FOUND);
		} else if (_autoIndex)
			return buildAutoindex();
		else
			throw ErrorResponse("In the response: URI points nowhere", ERR_CODE_FORBIDDEN);
	
	} else if (_startUri == "/cgi/print_res.py" || _startUri == "/cgi/print_res.php")
		throw ErrorResponse("In the reponse: URI IS NOT ALLOW", ERR_CODE_FORBIDDEN);
	else if (isRepertory(_root, _startUri) == 1)
		return buildPage();
	else
		throw ErrorResponse("In the response: URI is not a directory", ERR_CODE_NOT_FOUND);
}

void	Response::buildPost(void) {
	std::istringstream	iss;
	std::string			line, path;
	size_t 				pos, start = 0;
	
	// Get the boundary
	iss.str(_headers["Content-Type"]);
	if (!std::getline(iss, _boundary, '=') || !std::getline(iss, _boundary) || _boundary.empty())
		throw ErrorResponse("In the response: content-type not well formatted", ERR_CODE_BAD_REQUEST);
	
	// Get the end of preliminary binary information
	for (std::size_t i = 0; i < _binary_body.size(); i++) {
		line += _binary_body[i];
		if (_chunked && line.find(";") != std::string::npos) {
			start = i + 1;
			break;
		} else if (line.size() >= 4 && line.size() < _binary_body.size() && line.substr(line.size() - 4) == "\r\n\r\n") {
			start = i + 1;
			break;
		}
	}
	if (!start)
		throw ErrorResponse("In the response: missing opening body information", ERR_CODE_BAD_REQUEST);
	
	// Parse the multipart form data
	pos = line.find("filename=\"");
	if (pos != std::string::npos) {
		pos += 10; // Move past 'filename="'
		_filename = line.substr(pos, line.find("\"", pos) - pos);
		if (_filename.empty())
			throw ErrorResponse("In the response: missing filename information", ERR_CODE_BAD_REQUEST);
	} else
		throw ErrorResponse("In the response: missing filename information", ERR_CODE_BAD_REQUEST);
	
	// If chunked, check the start of the chunk

	// Check the end of file string is present
	line.clear();
	_boundary += "--";
	for (size_t i = _binary_body.size() - _boundary.size() - 2; i < _binary_body.size(); i++)
		line += _binary_body[i];
	if (line.find(_boundary) == std::string::npos)
		throw ErrorResponse("In the response: missing closing body information", ERR_CODE_BAD_REQUEST);
	
	// Save the file
	path = _host._rootPath + "/uploads/";

	// Check if the file exists
	if (access((path + _filename).c_str(), F_OK) != -1)
		throw ErrorResponse("In the response: file already exists", ERR_CODE_CONFLICT);

	std::ofstream outfile((path + _filename).c_str(), std::ios::binary);
	if (!outfile.is_open())
		throw ErrorResponse("In the response: cannot open the file", ERR_CODE_INTERNAL_ERROR);
	for (size_t i = start; i < _binary_body.size() - (_boundary.size() + 4); i++)
		outfile.put(_binary_body[i]);
	outfile.close();
	_host._files.push_back(_filename);

	// Send a response to the client
	_response_message = "HTTP/1.1 200 OK\r\n";
	_response_message += "Content-Type: text/plain\r\n";
	_response_message += "Content-Length: 19\r\n";
	_response_message += "\r\n";
	_response_message += "File uploaded successfully!";

	_response_ready = true;
}

void Response::buildDelete(void) {
	int			res;
	struct stat	check;
	std::string	path, file;

	// Check if the URI exists and replace % encoding by their character
	path = _root + _request_line["uri"].substr(0, _request_line["uri"].find_last_of("/") + 1);
	file = replace_percentage(_request_line["uri"].substr(_request_line["uri"].find_last_of("/") + 1));
	res = stat((path + file).c_str(), &check);
	if (res == -1)
		throw ErrorResponse("In the response: file not found", ERR_CODE_NOT_FOUND);
	
	// Check if the URI is a directory
	if (S_ISDIR(check.st_mode)) {

		// Check if the URI is a directory with correct ending
		if (_request_line["uri"].substr(_request_line["uri"].size() - 1) != "/")
			throw ErrorResponse("In the response: URI for directory does not end with /", ERR_CODE_CONFLICT);
		
		// Check for write permission
		if (access((path + file).c_str(), W_OK) == -1)
			throw ErrorResponse("In the response: no write permission", ERR_CODE_FORBIDDEN);

		// Attempt to remove the directory
		if (rmdir((path + file).c_str()) == -1)
			throw ErrorResponse("In the response: directory not removed", ERR_CODE_INTERNAL_ERROR);
	} else {
		
		// // Check for write permission
		if (access((path + file).c_str(), W_OK) == -1)
			throw ErrorResponse("In the response: no write permission", ERR_CODE_FORBIDDEN);

		// Attempt to remove the file
		if (unlink((path + file).c_str()) == -1)
			throw ErrorResponse("In the response: file not removed", ERR_CODE_INTERNAL_ERROR);

		_host._files.erase(std::remove(_host._files.begin(), _host._files.end(), file), _host._files.end());
	}
	
	// Send a response to the client
	_response_message = "HTTP/1.1 204 No Content\r\n";
	_response_message += "Server: " + _serverName + "\r\n";
	_response_message += "\r\n";

	_response_ready = true;
}

Response::~Response(void) {	return ; }
