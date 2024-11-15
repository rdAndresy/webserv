#ifndef ERRORS_HPP
# define ERRORS_HPP

# include "webserv.hpp"

class Host;

class ErrorConfFile: public std::exception {
	public:
		ErrorConfFile(std::string errorMsg) throw();
		~ErrorConfFile(void) throw();

		virtual const char* what() const throw();

		mutable std::string	_errorMsg;
};

class ErrorFdManipulation: public std::exception {
	public:
		ErrorFdManipulation(std::string errorMsg, int code) throw();
		~ErrorFdManipulation(void) throw();

		virtual const char* what() const throw();

		mutable std::string	_errorMsg;
		int					_code;
};

class ErrorRequest: public std::exception {
	public:
		ErrorRequest(std::string errorMsg, int code) throw();
		~ErrorRequest(void) throw();

		virtual const char* what() const throw();

		mutable std::string	_errorMsg;
		int					_code;
};

class ErrorResponse: public std::exception {
	public:
		ErrorResponse(std::string errorMsg, int code) throw();
		~ErrorResponse(void) throw();

		virtual const char* what() const throw();

		mutable std::string	_errorMsg;
		int					_code;
};

template <typename T>
void		send_error_page(Host & host, int i, const T & e, std::string uri);

std::string getStatus(const int & code);
void		ft_perror(const char * message);

# define ERR_CODE_SUCCESS			200
# define ERR_NAME_SUCCESS			"OK"

# define ERR_CODE_MOVE_PERM			301
# define ERR_NAME_MOVE_PERM			"Moved Permanently"

# define ERR_CODE_BAD_REQUEST		400
# define ERR_NAME_BAD_REQUEST		"Bad Request"

# define ERR_CODE_FORBIDDEN			403
# define ERR_NAME_FORBIDDEN			"Forbidden"

# define ERR_CODE_NOT_FOUND			404
# define ERR_NAME_NOT_FOUND			"Not Found"

# define ERR_CODE_MET_NOT_ALLOWED	405
# define ERR_NAME_MET_NOT_ALLOWED	"Method Not Allowed"

# define ERR_CODE_TIMEOUT			408
# define ERR_NAME_TIMEOUT			"Request Timeout"

# define ERR_CODE_CONFLICT			409
# define ERR_NAME_CONFLICT			"Conflict"

# define ERR_CODE_LENGTH_REQUIRED	411
# define ERR_NAME_LENGTH_REQUIRED	"Length Required"

# define ERR_CODE_PAYLOAD_TOO_LARGE	413
# define ERR_NAME_PAYLOAD_TOO_LARGE	"Payload Too Large"

# define ERR_CODE_URI_TOO_LONG		414
# define ERR_NAME_URI_TOO_LONG		"URI Too Long"

# define ERR_CODE_UNSUPPORTED_MEDIA	415
# define ERR_NAME_UNSUPPORTED_MEDIA	"Unsupported Media Type"

# define ERR_CODE_REQ_HEADER_FIELDS	431
# define ERR_NAME_REQ_HEADER_FIELDS	"Request Header Fields Too Large"

# define ERR_CODE_INTERNAL_ERROR	500
# define ERR_NAME_INTERNAL_ERROR	"Internal Server Error"

# define ERR_CODE_SERVICE_UNAVAIL	503
# define ERR_NAME_SERVICE_UNAVAIL	"Service Unavailable"

# define ERR_CODE_GATEWAY_TIMEOUT	504
# define ERR_NAME_GATEWAY_TIMEOUT	"Gateway Timeout"

# define ERR_CODE_HTTP_VERSION		505
# define ERR_NAME_HTTP_VERSION		"HTTP Version Not Supported"

#endif