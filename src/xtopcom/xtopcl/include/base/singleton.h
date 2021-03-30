#pragma once

#include <memory>
#include <mutex>

namespace xChainSDK {

    template<typename T>
    class Singleton {
    public:
        static std::shared_ptr<T> instance() {
            std::call_once(_onceFlag, []() {
                _instance = std::shared_ptr<T>(new T());
                });
            return _instance;
        };
    protected:
        Singleton() {};
        ~Singleton() {};
    private:
        Singleton(const Singleton& other);
        Singleton& operator=(const Singleton& other);
        static std::once_flag _onceFlag;
        static std::shared_ptr<T> _instance;
    };
    template<typename T>
    std::once_flag Singleton<T>::_onceFlag;
    template<typename T>
    std::shared_ptr<T> Singleton<T>::_instance;
}
