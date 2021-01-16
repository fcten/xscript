#ifndef _XSCRIPT_UTIL_SINGLETON_HPP_
#define _XSCRIPT_UTIL_SINGLETON_HPP_

namespace xscript::util {

template <class T>
class singleton {
protected:
    singleton() {}
    ~singleton() {}

public:
    singleton(const singleton&) = delete;
    singleton& operator=(const singleton&) = delete;

    static T& instance() {
		static T instance;
        return instance;
    }

};

}

#endif // _XSCRIPT_UTIL_SINGLETON_HPP_
