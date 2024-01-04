// Copyright (c) 2024 Damian Nowakowski. All rights reserved.

// This is the example of c++ coroutine tasks.
// For more details check: https://github.com/zompi2/cppcorosample

#include <iostream>
#include <coroutine>

struct CoroPromise;
struct CoroHandle : std::coroutine_handle<CoroPromise>
{
    using promise_type = ::CoroPromise;
};

struct CoroPromise
{
    CoroHandle get_return_object() { return { CoroHandle::from_promise(*this) }; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
};

class CoroTask1
{
public:

    void await_resume() {}
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<CoroPromise> Handle)
    {
        std::cout << "Suspended Using Task 1\n";
    };
};

class CoroTask2
{
public:

    void await_resume() {}
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<CoroPromise> Handle)
    {
        std::cout << "Suspended Using Task 2\n";
    };
};

CoroHandle CoroTest()
{
    std::cout << "CoroTest Before Suspend\n";
    co_await CoroTask1();
    std::cout << "CoroTest After First Resume\n";
    co_await CoroTask2();
    std::cout << "CoroTest After Second Resume\n";
}

int main()
{
    CoroHandle handle = CoroTest();
    std::cout << "CoroTest First Resuming\n";
    handle.resume();
    std::cout << "CoroTest Second Resuming\n";
    handle.resume();
    return 0;
}