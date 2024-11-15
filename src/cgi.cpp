#include "webserv.hpp"

std::vector<char *>	Response::MakeEnvCgi(std::string & cgi, std::vector<std::string> & storage) {
	std::vector<char *>			env;
	std::ostringstream			oss;
	std::string					keyWord;
	size_t						i;

	oss << _binary_body.size();
	exportENV(storage, env, "CONTENT_LENGTH", oss.str());
	if (_request_line["method"] == "POST")
		exportENV(storage, env, "CONTENT_TYPE", "application/x-www-form-urlencoded");
	else
		exportENV(storage, env, "CONTENT_TYPE", "text/html");
	exportENV(storage, env, "GATEWAY_INTERFACE", "CGI/1.1");
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); it++) {
		keyWord = it->first;
		i = 0;
		while (keyWord[i]) {
			keyWord[i] = toupper(keyWord[i]);
			if (keyWord[i] == '-')
				keyWord[i] = '_';
			i++;
		}
		keyWord.insert(0, "HTTP_");
		if (keyWord != "HTTP_CONNECTION")
			exportENV(storage, env, keyWord, it->second);
	}
	if (_request_line["method"] == "GET")
		exportENV(storage, env, "QUERY_STRING",_startUri.substr(_startUri.find_last_of('?'), _startUri.size()));
	exportENV(storage, env, "REDIRECT_STATUS", "200");
	exportENV(storage, env, "REQUEST_METHOD", _request_line["method"]);
	exportENV(storage, env, "SCRIPT_NAME", _startUri);
	exportENV(storage, env, "SCRIPT_FILENAME", "./" + cgi);
	exportENV(storage, env, "SERVER_PROTOCOL", "HTTP/1.1");
	exportENV(storage, env, "SERVER_SOFTWARE", "webserv");

	env.push_back(NULL);

	return (env);
}

void	Response::exec_cgi( int cgiFdOut, int cgiFdIn, std::string script) {
	int							writeStatus;
	int							fd[2];
	pid_t						pid, pid_result;
	time_t						start, end;
	std::vector<char *> 		env;
	std::vector<std::string>	storage;

	if (pipe(fd) == - 1)
		remove(".cgi.txt"), remove(".body_cgi.txt"), ft_close(cgiFdOut), ft_close(cgiFdIn), throw ErrorResponse("In CGI: pipe CGI", ERR_CODE_INTERNAL_ERROR);

	storage.reserve(_headers.size() + 9);
	env = MakeEnvCgi(script, storage);

	std::string line;
	for (std::size_t i = 0; i < _binary_body.size(); i++) {
			line += _binary_body[i];
		if (line.size() >= 4 && line.size() < _binary_body.size() && line.substr(line.size() - 4) == "\r\n\r\n") {
			start = i + 1;
			break;
		}
	}
	start = time(NULL);
	pid = fork();
	if (pid == -1)
		ft_close(cgiFdOut), ft_close(cgiFdIn), throw ErrorResponse("In CGI: fork CGI", ERR_CODE_INTERNAL_ERROR);
	if (pid == 0) {
		dup2(fd[0], STDIN_FILENO), dup2(cgiFdOut, STDOUT_FILENO);
		ft_close(fd[1]), ft_close(fd[0]), ft_close(cgiFdOut);  
		for (size_t i = 0; i < g_fds.size(); i++)
			if (g_fds[i] >= 0)
				ft_close(g_fds[i]);

		char *av[] = { const_cast<char*>(_Cgi.c_str()), const_cast<char*>(script.c_str()),  NULL };
		
		execve(const_cast<char*>(_Cgi.c_str()), av, env.data());
		throw std::runtime_error("In CGI: execve CGI failed");
	}
	writeToPipe(fd[1], line);
	ft_close(fd[0]), ft_close(fd[1]);
	while (true) {
		pid_result = waitpid(pid, &writeStatus, WNOHANG);
		if (pid_result > 0)
			break;
		if (pid_result == 0) {
			end = time(NULL);
			if (end - start >= 10) {
				ft_close(cgiFdOut), ft_close(cgiFdIn);
				if (kill(pid, SIGKILL) == -1)
					perror("kill");
				pid_result = waitpid(pid, &writeStatus, WNOHANG);
				remove(".cgi.txt"), remove(".body_cgi.txt");
				throw ErrorResponse("In CGI: timeout after 10 seconds", ERR_CODE_GATEWAY_TIMEOUT);
			}
		} else
			ft_close(cgiFdOut), ft_close(cgiFdIn), throw ErrorResponse("In CGI: waitpid CGI", ERR_CODE_INTERNAL_ERROR);
	}
}

void	Response::buildCgi(void) {
	size_t						f_interr, extention = _startUri.find_last_of('.');
	std::string					format = _startUri.substr(extention, _startUri.size());
	std::string					script = _startUri.substr(0, extention);
	int							cgiFdOut, cgiFdIn;
	std::stringstream			ss;
	
	if (_request_line["method"] == "POST") {
		if (format.substr(0, extention) == ".php") {
			script += ".php";
			_Cgi = "/usr/bin/php-cgi";
		} else if (format.substr(0, extention) == ".py") {
			script += ".py";
			_Cgi = "/usr/bin/python3";
		}
	} else if (_request_line["method"] == "GET") {
		f_interr = format.find_last_of('?');
		if (format.substr(0, f_interr) == ".php") {
			script += ".php";
			_Cgi = "/usr/bin/php-cgi";
		} else if (format.substr(0, f_interr) == ".py") {
			script += ".py";
			_Cgi = "/usr/bin/python3";
		}
	}
	
	script = _root + script;
	
	if (access(script.c_str(), F_OK))
		throw ErrorResponse("In CGI: access URI CGI", ERR_CODE_NOT_FOUND);

	cgiFdOut = open(".cgi.txt", O_WRONLY | O_CREAT | O_TRUNC);
	if (cgiFdOut == -1 || chmod(".cgi.txt", S_IRWXU | S_IRWXG | S_IRWXO) != 0)
		ft_close(cgiFdOut), throw ErrorResponse("In CGI: file CGI", ERR_CODE_NOT_FOUND);

	cgiFdIn = open(".body_cgi.txt", O_WRONLY | O_CREAT | O_TRUNC);
	if (cgiFdIn == -1 || chmod(".body_cgi.txt", S_IRWXU | S_IRWXG | S_IRWXO) != 0)
		ft_close(cgiFdOut), ft_close(cgiFdOut), throw ErrorResponse("In CGI: file body CGI", ERR_CODE_NOT_FOUND);
	
	try {
		exec_cgi(cgiFdOut, cgiFdIn, script);
	} catch (std::exception & e) {
		remove(".cgi.txt"), remove(".body_cgi.txt");
		throw;
	}		
	ft_close(cgiFdOut), ft_close(cgiFdIn);
	std::ifstream ifs(".cgi.txt");
	if (!ifs.is_open())
		throw ErrorResponse("In CGI: Open file CGI, potential fail in script execution", ERR_CODE_INTERNAL_ERROR);
	remove(".body_cgi.txt"), remove(".cgi.txt");
	ss << ifs.rdbuf();
	_response_body.clear();
	_response_body = ss.str(); 
	if (_response_body.size() > _maxBodySize)
		_response_body = "", throw ErrorResponse("Error in the size of the file requested", ERR_CODE_NOT_FOUND);

	ss.str("");
	ss.clear();
	ss << _response_body.size();
	_response_message = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: ";
	_response_message += ss.str();
	_response_message += "\r\n\r\n";
	_response_message += _response_body;

	_response_ready = true;
}

void	Response::exportENV(std::vector<std::string> & storage, std::vector<char *> & env, std::string key, std::string value) {
	storage.push_back(key + "=" + value);
	env.push_back(const_cast<char*>(storage.back().c_str()));
}