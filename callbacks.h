#ifndef CALLBACKS_H
#define CALLBACKS_H


#include <functional>
#include <memory>


using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;



class HttpConn;
typedef std::shared_ptr<HttpConn> HttpConnPtr;

typedef std::function<void (const HttpConnPtr&)> CloseCallback;


#endif  
