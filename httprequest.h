#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <string>
#include <regex>
#include <errno.h>
#include <unordered_map>
#include <unordered_set>


#include "buffer.h"



class HttpRequest
{
public:
	enum PARSE_STATE 
	{
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,        
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

	HttpRequest();
	~HttpRequest();
	
	void init();
	
	bool parse(Buffer& buffer);
	std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
  //  std::string GetPost(const std::string& key) const;
   // std::string GetPost(const char* key) const;


private:
    bool ParseRequestLine_(const std::string& line);
    void ParseHeader_(const std::string& line);
    void ParseBody_(const std::string& line);

    void ParsePath_();
    //void ParsePost_();
    //void ParseFromUrlencoded_();
	
	PARSE_STATE state_;
	std::string method_, path_, version_, body_;
	std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;
	
	static const std::unordered_set<std::string> DEFAULT_HTML;
   // static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
};

#endif