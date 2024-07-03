// Copyright (c) 2024 Damian Nowakowski. All rights reserved.

// This is the simpliest c++ coroutine example possible.
// For more details check: https://github.com/zompi2/cppcorosample

#include <iostream>
#include <coroutine>

// Forward declaration of the Promise so it can be used for a Handle definition
struct CoroPromise;

// Definition of the coroutine Handle using our Promise
struct CoroHandle : std::coroutine_handle<CoroPromise>
{
    // Tell the handle to use our Promise
    using promise_type = ::CoroPromise;

    // Called when the coroutine has been resumed
    void await_resume() {}

    // Indicated that coroutine can be suspended
    bool await_ready() { return false; }

    // Called when the coroutine has been suspended
    void await_suspend(std::coroutine_handle<CoroPromise> Handle) {};
};

// Definition of the coroutine Promise
struct CoroPromise
{
    // Called in order to construct the coroutine Handle
    CoroHandle get_return_object() { return { CoroHandle::from_promise(*this) }; }

    // Do not suspend when the coroutine starts
    std::suspend_never initial_suspend() noexcept { return {}; }
    
    // Do not suspend when the coroutine ends
    std::suspend_never final_suspend() noexcept { return {}; }
    
    // Called when co_return void is used
    void return_void() {}

    // Called when exception occurs
    void unhandled_exception() {}
};

// Definition of the coroutine function
CoroHandle CoroTest()
{
    std::cout << "CoroTest Before Suspend\n";

    // Suspends the function. Returns the handle.
    co_await CoroHandle();
    
    std::cout << "CoroTest After Resume\n";
}

// Main program
int main()
{
    // Calls the coroutine function. Stores the Coroutine handle for it.
    CoroHandle handle = CoroTest();

    std::cout << "CoroTest Resuming\n";

    // Resumes the suspended coroutine function.    
    handle.resume();
    
    return 0;
}

/**
 The program should output:

 CoroTest Before Suspend
 CoroTest Resuming
 CoroTest After Resume
*/