#ifndef LOCATION_HPP
# define LOCATION_HPP

#include "webserv.hpp"

class Location {
	public:
		std::string							_Uri;
		std::string							_ServerName;
		std::string							_Root;
		std::string							_CgiPath;
		std::map<int,std::string>			_Retourn;
		std::map<int,std::string>			_PageError;
		std::vector<std::string>			_Methods;
		std::vector<std::string>			_Indx;
		bool								_indexFlag;
		bool								_CgiFlag;
		bool								_ReturnFlag;
		bool								_ErPages;
		bool								_AutoFlag;
		bool								_AutoIndex;
		bool								_MetFlag;
		bool								_rootflag;
		int									_flagPost;
		int									_flagGet;
		int									_flagDelete;
		int									_CgiAllow;

		Location(/*args*/);
		~Location();
		void								InLoc_Index(std::istringstream & iss);
		void								InLoc_Cgi(std::istringstream & iss);
		void								InLoc_Return(std::istringstream & iss);
		int									InLoc_p_Return(std::string & codeRetrn);
		void								InLoc_ErPages(std::istringstream & iss);
		int									InLoc_p_errorCodes(std::string & pgError);
		void								InLoc_AutoIndex(std::istringstream & iss);
		void								InLoc_Methos(std::istringstream & iss);
		void								InLoc_root(std::istringstream & iss);
		void								setUri(std::string uri);
		void								ParseLocation(std::istream&  file);
				
		std::string							getUri(void) const;
		int									getFlagAutoInx(void) const;
		int									getFlagIndex(void) const;
		int									getReturnFlag(void) const;
		int									getFlagErrorPages(void) const;
		int									getRootFlag(void) const;
		int									getFlagCgi(void) const;
		std::map<int,std::string> const &	getReturnPages() const;
		std::map<int,std::string> const &	getPagesError() const;
		std::vector<std::string> const &	getIndexPages() const;
		std::string							getRoot() const;
		bool								getAutoIndex() const;
		std::string							getCgiPath() const;
		int									getCgiAllow() const;
		int									getFlagGet() const;
		int									getFlagPost() const;
		int									getFlagDelete() const;
	std::vector<std::string>				getMtods() const;
};

#endif