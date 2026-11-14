#ifndef SINGLETON_H
#define SINGLETON_H

#include <memory>

template <typename T>
class Singleton {
protected:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    ~Singleton() = default;

public:
    static std::shared_ptr<T> GetInstance() {
        static const auto instance = std::shared_ptr<T>(new T);
        return instance;
    }
};

#endif
