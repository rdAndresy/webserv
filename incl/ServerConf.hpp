#ifndef SERVERCONF_HPP
# define SERVERCONF_HPP

#include "webserv.hpp"

class ServerConf {
	public:
		ServerConf(void);
		ServerConf(ServerConf const & src);
		~ServerConf(void);

		struct sockaddr_in				_address;
		std::map<int, std::string>		_PageError;
		std::map<int, std::string>		_CodeReturn;
		std::vector<std::string>		_IndexFile;
		int								_nb_keepalive;
		int								_max_keepalive;
		int								_fdSetSock;
		int								_fdEpoll;
		std::map<int, time_t>			_connections;
		int								_address_len;
		int								_NotBind;
		int								_port;
		int								_IndexPages;
		int								_nbServer;
		size_t							_maxBodySize;
		std::string						_PortString;
		std::string						_ip;
		std::string						_raw_ip;
		std::string						_name;
		std::string						_rootPath;
		std::map<std::string, Location>	_Location;
		bool							_maxBodyState;
		bool							_DefaultPort;
		bool							_StateListen;
		bool							_IpDefault;
		bool							_isServerName;
		bool							_rootFlag;
		bool							_Autoindex;
		bool							_errorFlag;
		bool							_Default_server;
		bool							_CheckDefaultServer;
		bool							_ReturnFlag;

		// P means parsing;
		void							printServerNames(void)const;
		void							initWServer(std::istream & file);
		void							setNbServer(int nb);
		void							p_Listen(std::istringstream & iss);
		void							p_IpAddrs(void);
		void							p_name(std::istringstream & iss);
		void							p_MaxClientBodySize(std::istringstream &iss);
		void							p_Root(std::istringstream & iss);
		void							p_AutoIndex(std::istringstream & iss);
		void							p_ErrorPages(std::istringstream & iss);
		void							p_Index(std::istringstream & iss);
		void							p_DefaultServer(std::istringstream & iss);
		void							p_CodeReturn(std::istringstream & iss);
		// void							p_Location(std::istringstream& iss, std::string kw);
		int								p_errorCodes(std::string & pgError);
		int								p_Return(std::string & codeRetrn);
		
		//SETEUR -------------------//	
		void							set_fdEpoll(int _fdEpoll);
		// void							setDefaultErrorPages(void);
		//GETEUR ------------------//
		int								get_fdEpoll(void) const;
		std::map<int, std::string>		getPagesError(void) const;
		std::map<int, std::string>		getCodesReturn(void) const;
		std::string const &				getIp(void) const;
		int								getPort(void) const;
		std::string const &				getNameServer(void) const;
		int								getNumberServer(void) const;
		std::vector<std::string>		getIndexFiles(void) const;
};

#endif