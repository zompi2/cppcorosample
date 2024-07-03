// Copyright (c) 2024 Damian Nowakowski. All rights reserved.

// This is the example of c++ coroutine Awaiters.
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

    // Called when co_return is used
    void return_void() {}

    // Called when exception occurs
    void unhandled_exception() {}
};

// Definition of the base coroutine Awaiter
// It should contain the common part of every Awaiter we need
class CoroAwaiterBase
{
public:

    // Called when the coroutine has been resumed
    void await_resume() {}

    // Indicated that coroutine can be suspended
    bool await_ready() { return false; }
};

// Definition of the coroutine Awaiter A
class CoroAwaiterA : public CoroAwaiterBase
{
public:

    // Called when the coroutine has been suspended using this Awaiter
    void await_suspend(std::coroutine_handle<CoroPromise> Handle)
    {
        std::cout << "Suspended Using Awaiter A\n";
    };
};

// Definition of the coroutine Awaiter B
class CoroAwaiterB : public CoroAwaiterBase
{
public:

    // Called when the coroutine has been suspended using this Awaiter
    void await_suspend(std::coroutine_handle<CoroPromise> Handle)
    {
        std::cout << "Suspended Using Awaiter B\n";
    };
};

// Definition of the coroutine function
CoroHandle CoroTest()
{
    std::cout << "CoroTest Before Suspend\n";

    // Suspending the function using Awaiter A
    co_await CoroAwaiterA();

    std::cout << "CoroTest After First Resume\n";

    // Suspending the function using Awaiter B
    co_await CoroAwaiterB();

    std::cout << "CoroTest After Second Resume\n";
}

// Main program
int main()
{
    // Calls the coroutine function. Stores the coroutine Handle for it.
    CoroHandle handle = CoroTest();

    std::cout << "CoroTest First Resuming\n";

    // Resumes the suspended coroutine function.    
    handle.resume();

    std::cout << "CoroTest Second Resuming\n";

    // Resumes the suspended coroutine function.    
    handle.resume();

    return 0;
}

/**
 The program should output:

 CoroTest Before Suspend
 Suspended Using Awaiter A
 CoroTest First Resuming
 CoroTest After First Resume
 Suspended Using Awaiter B
 CoroTest Second Resuming
 CoroTest After Second Resume
*/