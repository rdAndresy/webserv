#include "webserv.hpp"

ServerConf::ServerConf(void) {
	memset(&_address, 0, sizeof _address);
	_address.sin_family = AF_INET; // IPv4
	_address.sin_port = htons(PORT);// port par defaut 8080
	_address.sin_addr.s_addr = htonl(INADDR_ANY);// address par default(00000)
	_port = 8080; // port par default;
	_ip = "0.0.0.0"; // address far default;
	_raw_ip = _ip;
	_NotBind = 0; // check if is bind or not;
	_maxBodySize = MAX_BODY_SIZE;
	_address_len = sizeof(_address);
	_fdSetSock = -1;
	_fdEpoll = -1;
	_nb_keepalive = 0;
	_max_keepalive = MAX_NB_KEEP_ALIVE;
	_IndexPages = 0;
	_nbServer = 0;
	_maxBodyState = false;
	_StateListen = true;
	_DefaultPort = true;
	_IpDefault = true;
	_isServerName = false;
	_rootFlag = 0;
	_Autoindex = false;
	_errorFlag = false;
	_Default_server = false;
	_CheckDefaultServer = false;
	_ReturnFlag = false;
	_PortString = "";
	_rootPath = "";
	_fdEpoll = 0;
}

ServerConf::ServerConf(ServerConf const & src): _address(src._address), _PageError(src._PageError),
	_CodeReturn(src._CodeReturn), _IndexFile(src._IndexFile), _nb_keepalive(src._nb_keepalive),
	_max_keepalive(src._max_keepalive), _fdSetSock(src._fdSetSock), _fdEpoll(src._fdEpoll), _connections(src._connections),
	_address_len(src._address_len), _NotBind(src._NotBind), _port(src._port), _IndexPages(src._IndexPages),
	_nbServer(src._nbServer), _maxBodySize(src._maxBodySize), _PortString(src._PortString), _ip(src._ip), _raw_ip(src._raw_ip),
	_name(src._name), _rootPath(src._rootPath), _Location(src._Location), _maxBodyState(src._maxBodyState),
	_DefaultPort(src._DefaultPort), _StateListen(src._StateListen), _IpDefault(src._IpDefault),
	_isServerName(src._isServerName), _rootFlag(src._rootFlag), _Autoindex(src._Autoindex),
	_errorFlag(src._errorFlag), _Default_server(src._Default_server), _CheckDefaultServer(src._CheckDefaultServer),
	_ReturnFlag(src._ReturnFlag) { return ; }

void    ServerConf::p_IpAddrs(void) {
	int	replace = 0, ip[4], res = 0;
	std::istringstream iss;
	std::string	more;

	if (_ip.empty())
		throw ErrorConfFile("Erron In conf file: ip not found");
	if (_ip == "locahost")
	{
		_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		_IpDefault = false;
		return;
	}
	if (_ip.find_first_not_of("0123456789.") != std::string::npos)
		throw ErrorConfFile("In conf file: ip");
	_raw_ip = _ip;
	for (int i = 0; _ip[i]; i++)
		if (_ip[i] == '.')
			_ip.replace(i, 1, 1, ' '), replace++;
	if (replace != 3)
		throw ErrorConfFile("In conf file: listen: wrong host (ip)4");
	
	iss.str(_ip);
	if (!(iss >> ip[0]) || !(iss >> ip[1]) || !(iss >> ip[2]) || !(iss >> ip[3]))
		throw ErrorConfFile("In conf file: listen: wrong (ip)1");
	for (int i = 0; i < 4; i++) {
		if (ip[i] < 0 || ip[i] > 255)
			throw ErrorConfFile("In conf file: listen: wrong (ip)2");
		res = res << 8 | ip[i];
	}

	if (iss >> more)
		throw ErrorConfFile("In conf file: listen: wrong (ip)3");

	_address.sin_addr.s_addr = htonl(res);
	_IpDefault = false;
}

void    ServerConf::p_Listen(std::istringstream& iss) {
	std::string			line;
	std::stringstream	ss;
	size_t				inx;

	if (!(iss >> line))
		throw ErrorConfFile("In the conf file");

	inx = line.find(":");

	_ip = line.substr(0, inx);
	p_IpAddrs();
	_PortString = line.substr(inx + 1, line.size()); 
	
	if (_PortString.empty())
		return ;

	// find_first_not_f Searches the string for the first character that does not match any of the characters specified in its arguments.
	if (_PortString.find_first_not_of("0123456789") != std::string::npos || _PortString.size() > 5)
		throw ErrorConfFile("In conf file: listen: wrong IP");

	ss.str(_PortString);
	ss >> _port;

	if (_port > 65535)// PORT MAX VALEUR
		throw ErrorConfFile("In conf file: listen: wrong port");
	_address.sin_port = htons(_port);

	_DefaultPort = false;
	if (iss >> line)
		throw   ErrorConfFile("In the conf file");
	_StateListen = false;
}   

void ServerConf::printServerNames(void) const {
	for (size_t i = 0; i < _name.size(); ++i)
		std::cout << i + 1 << ". " << _name[i] << std::endl;  // Stampa con un indice
}

void	ServerConf::p_name(std::istringstream &iss) {
	std::string serverNames;
	
	if (!(iss >> serverNames))
		throw ErrorConfFile("In conf file: serve_name not found 1;");

	if (iss >> serverNames)
		throw ErrorConfFile("In conf file: serve_name not found;");

	_name = serverNames;

	_isServerName = true;
}

void	ServerConf::p_Root(std::istringstream &iss) {
	std::string 	pathRoot;
	struct stat		info;

	if (!(iss >> pathRoot))
		throw ErrorConfFile("In conf file: root path don't found");

	if (pathRoot.compare(0, 3, "www") != 0 && pathRoot.compare(0, 4, "www/") != 0)
		throw ErrorConfFile("In conf file: root wrong path");

	_rootPath = pathRoot;

	if (stat(_rootPath.c_str(), &info) != 0)// cannot access path 
		throw ErrorConfFile(("In conf file: root for server: cannot access path or file " + _rootPath).c_str());

	_rootFlag = 1;
}

/* Il comando autoindex è una direttiva comunemente usata nei server come NGINX.
In pratica, controlla se il server web deve mostrare automaticamente una lista di directory
quando non è presente un file di indice (come index.html) nella directory richiesta.
Funzionamento di autoindex

autoindex on: Se abilitato (on), il server mostrerà una lista dei file e delle sottocartelle presenti nella directory richiesta, se non è disponibile un file di indice.
autoindex off: Se disabilitato (off), il server restituirà un errore (tipicamente un 403 Forbidden) se non trova un file di indice nella directory richiesta. */

void	ServerConf::p_AutoIndex(std::istringstream & iss) {
	std::string autoIndex;

	if (iss >> autoIndex) {
		if (autoIndex == "on")
			_Autoindex = true;
		else
			_Autoindex = false; // forbidden page
	}
}

/* client_max_body_size 1M;:
Imposta la dimensione massima del corpo della richiesta a 1 megabyte (MB). Se un client tenta di inviare una richiesta con un corpo che supera questa dimensione,
NGINX restituirà un errore HTTP 413 (Request Entity Too Large). */

void	ServerConf::p_MaxClientBodySize(std::istringstream &iss)
{
	const size_t			KB_IN_BYTES = 1024;
	const size_t			MB_IN_BYTES = KB_IN_BYTES * 1024;
	const size_t			MAX_SIZE_LIMIT = 10 * MB_IN_BYTES; // 3MB
	size_t					size, sizeValue = 0;
	std::string::size_type	index = 0;
	std::string				maxClntBodySize;
	char					unitChar;
	
	if (!(iss >> maxClntBodySize) || maxClntBodySize.empty())
		throw ErrorConfFile("In conf file: client_max_body_size");
	if (maxClntBodySize.find_first_not_of("0123456789") == 0)
		throw ErrorConfFile("In conf file: client_max_body_size");

	sizeValue = std::strtoul(maxClntBodySize.c_str(), NULL, 10);

	if (index < maxClntBodySize.size()) {
		size = maxClntBodySize.size();
		unitChar = maxClntBodySize[size - 1];
		switch (unitChar) {
			case 'k':
			case 'K':
				sizeValue *= KB_IN_BYTES;
				break;
			case 'm':
			case 'M':
				sizeValue *= MB_IN_BYTES;
				break;
			case '\0':
				sizeValue *= MB_IN_BYTES; // Default is bytes if no unit provided 
				break;
			default:
				throw ErrorConfFile("In conf file: max_client_body_size: wrong unit, must be k, K, m, M or empty");
		}
	}

	if (sizeValue > MAX_SIZE_LIMIT)
		throw ErrorConfFile("In conf file: max_client_body_size: size has to be between 0 and 3MB");

	_maxBodySize = sizeValue;
	_maxBodyState = true;
}

int	ServerConf::p_errorCodes(std::string &pgError) {
	int 		errorCode;
	std::string	path;
	size_t		ix = pgError.find_first_not_of("0123456789");

	if (ix == std::string::npos) {
		errorCode = strtol(pgError.c_str(), NULL, 10);
		if (errorCode < 100 || errorCode > 599)
			throw ErrorConfFile("In conf file: error_page  x < 100 || x > 599 ");
		return (errorCode);
	} else
		throw ErrorConfFile("In conf file: error_page parseCde");
}

void	ServerConf::p_ErrorPages(std::istringstream& iss)
{
	std::string pgError;;
	std::vector<int> erroCodeVector;
	int				eCode;

	if (!(iss >> pgError))
		throw ErrorConfFile("In conf file: error_pages");
	eCode = p_errorCodes(pgError);
	erroCodeVector.push_back(eCode);
	while ((iss >> pgError) && pgError.find_first_not_of("0123456789") == std::string::npos) {
		eCode =  p_errorCodes(pgError);
		erroCodeVector.push_back(eCode);
	}

	if (pgError.empty())
		throw ErrorConfFile("In conf file: error_pages 1");
	if (pgError[0] != '/' && pgError.find("..") != std::string::npos)
		throw ErrorConfFile("In conf file: error_pages 2");
	if (iss >> pgError)
		throw ErrorConfFile("In conf file: error_pages format /html");
	for (size_t i = 0; i < erroCodeVector.size(); i++)
		_PageError[erroCodeVector[i]] = pgError;

	_errorFlag = true;
}


int	ServerConf::p_Return(std::string &codeRetrn) {
	size_t		ix = codeRetrn.find_first_not_of("0123456789");
	std::string	path;
	int			errorCode;

	if (ix == std::string::npos) {
		errorCode = strtol(codeRetrn.c_str(), NULL, 10);
		if (errorCode < 300 || errorCode > 308)
			throw ErrorConfFile("In conf file: error_page  x < 100 || x > 599 ");
		return (errorCode);
	} else
		throw ErrorConfFile("In conf file: error_page parseCde");
}

void	ServerConf::p_CodeReturn(std::istringstream& iss) {
	std::string 		codeRetrn;
	std::vector<int>	vectorCode;
	int					codeRe;

	if (!(iss >> codeRetrn))
		throw ErrorConfFile("In conf file: return");
	codeRe = p_Return(codeRetrn);
	vectorCode.push_back(codeRe);
	while ((iss >> codeRetrn) && codeRetrn.find_first_not_of("0123456789") == std::string::npos) {
		codeRe = p_Return(codeRetrn);
		vectorCode.push_back(codeRe);
	}

	if (codeRetrn.empty())
		throw ErrorConfFile("In conf file: return empty");
	if (codeRetrn[0] != '/' && codeRetrn.find("..") != std::string::npos)
		throw ErrorConfFile("In conf file: return");
	if (iss >> codeRetrn)
		throw ErrorConfFile("In conf file: return");
	for (size_t i = 0; i < vectorCode.size(); i++)
		_CodeReturn[vectorCode[i]] = codeRetrn;
	_ReturnFlag = true;
}

void	ServerConf::p_Index(std::istringstream& iss) {
	std::string		index;

	if (!(iss >> index) || index.empty())
		throw ErrorConfFile("In conf file: index ");
	_IndexFile.push_back(index);
	_IndexPages = 1;
	while (iss >> index) {
		if (index.empty() || index.find(".") == std::string::npos)
			throw ErrorConfFile("In conf file: index ");
		else
			_IndexFile.push_back(index);
	}
}

void	ServerConf::p_DefaultServer(std::istringstream& iss) {
	std::string line;

	_Default_server = true;
	if (iss >> line)
		throw ErrorConfFile("In conf file: defualt_server");
	_CheckDefaultServer = true;
}

void    ServerConf::initWServer(std::istream &file) {
	std::string	line, kw;
	int			i = 0;

	while (std::getline(file, line)) {
		std::istringstream iss(line);
		if (line.empty() || line == "\t\t")
			continue;
		if (!(iss >> kw))
			continue;
		else if (kw == "listen" && _StateListen )
			p_Listen(iss);
		else if (kw == "server_name" && !_isServerName)
			p_name(iss);
		else if (kw == "root" && !_rootFlag)
			p_Root(iss);
		else if (kw == "autoindex")
			p_AutoIndex(iss);
		else if (kw == "client_max_body_size" && !_maxBodyState)
			p_MaxClientBodySize(iss);
		else if (kw == "error_page" &&  !_errorFlag)
			p_ErrorPages(iss);
		else if (kw == "index" && !_IndexPages ) // is the file in the root path 
			p_Index(iss);
		else if (kw == "return" && !_ReturnFlag)
			p_CodeReturn(iss);
		else if (kw == "default_server" && !_CheckDefaultServer)
			p_DefaultServer(iss);
		else if (kw == "location") {
			Location	location;
			std::string	prefix;
			if (!(iss >> kw))
				throw ErrorConfFile("In conf file: location: wrong content");
			if (kw != "{") {
				prefix = kw;
				location.setUri(prefix);
				if ((iss >> kw) && kw != "{")
					throw ErrorConfFile("In conf file: location: wrong content");
			} else
				throw ErrorConfFile("In conf file: location: wrong content");
			location.ParseLocation(file);
			_Location[location.getUri()] = location;
			i++;
			
		} else if (line == "}") {
			if (!_rootFlag )
				throw ErrorConfFile("In conf file: missing root path");
			if (_StateListen)
				throw ErrorConfFile("In conf file: missing listen address");
			if (!_isServerName)
				throw ErrorConfFile("In conf file: missing server name");
			break;
		} else
			throw ErrorConfFile("In conf file: server section");
	}
}

void	ServerConf::set_fdEpoll(int _fdEpoll) { this->_fdEpoll = _fdEpoll; }

int	ServerConf::get_fdEpoll(void) const { return (this->_fdEpoll); }

std::map<int, std::string> ServerConf::getPagesError(void) const { return (this->_PageError); }

std::map<int, std::string> ServerConf::getCodesReturn(void) const { return (this->_CodeReturn); }

std::string const & ServerConf::getIp(void) const { return (this->_ip); }

int	ServerConf::getPort(void) const { return (this->_port); }

std::string const &	ServerConf::getNameServer(void) const { return(this->_name); }

int	ServerConf::getNumberServer(void) const { return (this->_nbServer); }

std::vector<std::string> 	ServerConf::getIndexFiles(void) const { return (this->_IndexFile); }

void	ServerConf::setNbServer(int nb) { _nbServer = ++nb; }

ServerConf::~ServerConf(void) {    return; }
