#ifndef _Util_H_
#define _Util_H_

#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <functional>

class Util
{
    
public:
    Util();
    ~Util();
    
    static uint32_t adler32(const char *data, size_t len); 

};



struct ExitCaller
{
	~ExitCaller() { functor_(); }
	ExitCaller(std::function<void()>&& functor) : functor_(std::move(functor)) {}

private:
	std::function<void()> functor_;
};

#endif