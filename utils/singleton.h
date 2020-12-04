#ifndef _SINGLETON_H_
#define _SINGLETON_H_

template <typename T>
class Singleton
{
private:
	static T* instance_;
	Singleton()
	{
	}

public:
	static T* get_instance()
	{
		return instance_;
	}
};

template <typename T>
T* Singleton<T>::instance_ = new T;

#endif//_SINGLETON_H_