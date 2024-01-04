// Copyright (c) 2024 Damian Nowakowski. All rights reserved.

// This is the example of c++ coroutine generators.
// For more details check: https://github.com/zompi2/cppcorosample

#include <iostream>
#include <coroutine>

// Definition of the coroutine Generator which can store a value of a generic type
template<typename T>
struct CoroGenerator
{
    // Forward declaration of the Promise so it can be used for a Handle definition
    struct CoroPromise;

    // Tell the Generator to use our Promise
    using promise_type = CoroPromise;

    // Convinient alias for the coroutine Handle type which uses declared Promise
    using CoroHandle = std::coroutine_handle<CoroPromise>;

    // Definition of the Generator Promise
    struct CoroPromise
    {
        // Stored value of a generic type
        T Value;

        // Called in order to construct the Generator
        CoroGenerator get_return_object() { return { CoroGenerator(CoroHandle::from_promise(*this)) }; }
        
        // Suspend the Generator at the beginning
        std::suspend_always initial_suspend() noexcept { return {}; }

        // Suspend the Generator at the end
        std::suspend_always final_suspend() noexcept { return {}; }

        // Called when co_return is used
        void return_void() {}

        // Called when exception occurs
        void unhandled_exception() {}

        // Called when co_yield is used. Stores the value which comes with this yield
        template<std::convertible_to<T> From>
        std::suspend_always yield_value(From&& from)
        {
            Value = std::forward<From>(from);
            return {};
        }
    };

    // Stores the coroutine Handle used within this Generator
    CoroHandle Handle;

    // Try to resume the Generator and check if it's execution is done
    explicit operator bool()
    {
        Handle.resume();
        return !Handle.done();
    }

    // Get the lastly stored by co_yield value
    T operator()()
    {
        return std::move(Handle.promise().Value);
    }

    // Constructor - save the coroutine Handle given during the get_return_object call in the Promise
    CoroGenerator(CoroHandle InHandle) : Handle(InHandle) {}

    // Destructor - explicitly destroy the coroutine Handle, because it is not destroyed automatically, because
    // of the final_suspend set to suspend_always
    ~CoroGenerator() { Handle.destroy(); }
};

// Fibonacci Sequence Generator. Yields every next value of the sequence and suspends it's execution.
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

// Main program
int main()
{
    // Constructs the Generator. Because initial_suspend is set to suspend_always the defined function
    // will not start immediately.
    auto generator = FibonacciGenerator(10);

    // Every time a bool() operator is called the Generator resumes it's execution and checks if the coroutine has finished.
    // When the coroutine has finished the wile loop will finish.
    while (generator)
    {
        // Get and print every value yielded by  the Generator.
        std::cout << generator() << " ";
    }

    // To check if the coroutine has finished the final_suspend was set to suspend_always. Thanks to that
    // the coroutine will not be destroyed automatically when it finishes. This will allow us to check the Handle
    // if the handling coroutine has been finished or not. In the end, the Generator goes out of scope and it's desctructor
    // destroys the coroutine Handle.
    return 0;
}

/**
 The program should output:

 1 1 2 3 5 8 13 21 34 55
*/