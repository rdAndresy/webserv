#include "webserv.hpp"

void	Request::pnc_check_chunk(void) {
	std::stringstream			ss;
	std::vector<char>			res;
	std::vector<std::string>	lines;
	std::string					tmp;
	bool						len = true;
	char						ch;
	size_t						chunk_size, length = 0;

	ss.write(_binary_body.data(), _binary_body.size());
	while (ss.get(ch)) {
		if (ch == '\n') {
			lines.push_back(tmp);
			tmp.clear();
		} else
			tmp += ch;
	}
	if (!tmp.empty())
		lines.push_back(tmp);

	for (size_t i = 0; i < lines.size(); i++) {
		if (lines[i].empty())
			continue ;
		if (lines[i].find("\r") == std::string::npos)
			break ;
		else if (len) {
			std::istringstream	iss(lines[i].substr(0, lines[i].size() - 1));
			iss >> std::hex >> chunk_size;
			if (iss.fail() || !is_hex(lines[i].substr(0, lines[i].size() - 1)))
				throw ErrorRequest("In the request: chunk size not well formatted", ERR_CODE_BAD_REQUEST);
			if (chunk_size == 0)
				break ;
			len = false;
		} else if (!len) {
			length += lines[i].size() - 1;
			if (i + 1 < lines.size()) {
				if (is_hex(lines[i + 1].substr(0, lines[i + 1].size() - 1))) {
					if (!len && length != chunk_size && lines[i][lines[i].size() - 1] == '\r')
						throw ErrorRequest("In the request: chunk size mismatch", ERR_CODE_BAD_REQUEST);
					len = true;
					length = 0;
				}
			if (i == lines.size() - 1 || i == lines.size() - 2)
				std::cout << YELLOW "--> Chunk is correct" RESET << std::endl;
			}
		}
	}
}

std::vector<char>	Request::pnc_clean_chunk(void) {
	std::stringstream			ss;
	std::vector<std::string>	lines;
	std::string					tmp;
	bool						len = true;
	char						ch;

	ss.write(_binary_body.data(), _binary_body.size());
	_binary_body.clear();
	while (ss.get(ch)) {
		if (ch == '\n') {
			lines.push_back(tmp);
			tmp.clear();
		} else
			tmp += ch;
	}

	if (!tmp.empty())
		lines.push_back(tmp);

	for (size_t i = 0; i < lines.size(); i++) {
		if (lines[i].empty()) {
			lines.erase(lines.begin() + i);
			i--;
			continue ;
		}
		if (len) {
			lines.erase(lines.begin() + i);
			i--;
			len = false;
		} else if (!len) {
			for (size_t j = 0; j < lines[i].size(); j++)
				_binary_body.push_back(lines[i][j]);
			_binary_body.push_back('\n');
			if (i + 1 < lines.size() && is_hex(lines[i + 1].substr(0, lines[i + 1].size() - 1)))
				len = true;
		}
	}
	_binary_body.pop_back();
	_binary_body.pop_back();
	return _binary_body;
}