// Copyright (c) 2024 Damian Nowakowski. All rights reserved.

#include <iostream>
#include <coroutine>

template<typename T>
struct CoroGenerator
{
    struct CoroPromise;
    using promise_type = CoroPromise;
    using CoroHandle = std::coroutine_handle<CoroPromise>;

    struct CoroPromise
    {
        T Value;

        CoroGenerator get_return_object() { return { CoroGenerator(CoroHandle::from_promise(*this)) }; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}

        template<std::convertible_to<T> From>
        std::suspend_always yield_value(From&& from)
        {
            Value = std::forward<From>(from);
            return {};
        }
    };

    CoroHandle Handle;

    explicit operator bool()
    {
        Handle.resume();
        return !Handle.done();
    }

    T operator()()
    {
        return std::move(Handle.promise().Value);
    }

    CoroGenerator(CoroHandle InHandle) : Handle(InHandle) {}
    ~CoroGenerator() { Handle.destroy(); }
};

CoroGenerator<int> FibonacciGenerator(const int Amount)
{
    if (Amount <= 0)
    {
        co_return;
    }

    int n1 = 1;
    int n2 = 1;
    for (int i = 1; i <= Amount; i++)
    {
        if (i < 3)
        {
            co_yield 1;
        }
        else
        {
            const int tmp = n2;
            n2 = n1 + n2;
            n1 = tmp;
            co_yield n2;
        }
    }
}

int main()
{
    auto generator = FibonacciGenerator(10);
    while (generator)
    {
        std::cout << generator() << " ";
    }
    return 0;
}
