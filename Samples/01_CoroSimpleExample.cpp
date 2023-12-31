// Copyright (c) 2023 Damian Nowakowski. All rights reserved.

#include <iostream>
#include <coroutine>

struct CoroPromise;
struct CoroHandle : std::coroutine_handle<CoroPromise>
{
    using promise_type = ::CoroPromise;

    void await_resume() {}
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<CoroPromise> Handle) {};
};

struct CoroPromise
{
    CoroHandle get_return_object() { return { CoroHandle::from_promise(*this) }; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
};

CoroHandle CoroTest()
{
    std::cout << "CoroTest Before Suspend\n";
    co_await CoroHandle();
    std::cout << "CoroTest After Resume\n";
}

int main()
{
    CoroHandle handle = CoroTest();
    std::cout << "CoroTest Resuming\n";
    handle.resume();
    return 0;
}