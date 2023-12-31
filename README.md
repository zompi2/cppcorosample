# C++20 coroutine samples

This is my understanding of [c++20 coroutines](https://en.cppreference.com/w/cpp/language/coroutines) with some sample code.  
I've written this document to help others to understand what the coroutines are, how to write them and when they are useful. 

## What is a coroutine?

Coroutine is a function which execution can be suspended (without suspending the rest of the code) and it can be resumed later. Such functionality is very useful if we have a function that needs to wait for something, for example for the response from http request or for the result of the other function that runs on another thread.

## Coroutines in other languages

Coroutines have been a part of other languages for a long time, for example in C#. One of the most famous example is a code for [camera fade out for Unity game engine](https://docs.unity3d.com/Manual/Coroutines.html).

```cs
IEnumerator Fade()
{
    Color c = renderer.material.color;
    for (float alpha = 1f; alpha >= 0; alpha -= 0.1f)
    {
        c.a = alpha;
        renderer.material.color = c;
        yield return new WaitForSeconds(.1f);
    }
}
```

Normally the for-loop would simply set the material alpha to 0, but because this is a coroutine it will wait for 0.1 seconds after each for-loop iteration which will lead to a fluid change of the material alpha. We can recognize that this is a coroutine because of the presence of the `yield` keyword. The `yield` suspends the whole function while the Unity built in coroutines system will resume it.

## Coroutine in C++
Function is a coroutine in C++ when there is one of the following keywords inside:
* `co_await` - it simply suspends the function
* `co_yield` - it suspends the function and it returns a value
* `co_return` - it completes the function with or without a value

## The simpliest C++ coroutine example possible

This is (probably) the most simple c++ coroutine example possible. There is a lot of happening there, but it will all be explained below.

```c++
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
```

The output of this code will be:

```
CoroTest Before Suspend
CoroTest Resuming
CoroTest After Resume
```

Now let's go through all of this from top to bottom. First of all we have to define the coroutine. The coroutine is made of two parts:

### Coroutine Handle
This is a structure based on the `std::coroutine_handle`. It controls the flow of the coroutine and will be used to resume the suspended coroutine later. The absolute minimum which needs to be defined inside the handle are:
* `promise_type` - it is a type of the Coroutine Promise. What is a Promise will be described later.
* `await_resume` - this function is called when the coroutine is resumed.
* `await_ready` - this function is called before the suspension. If it returns `false` (or `void`) the coroutine will be suspended. If it returns `true` it will ignore the suspension.
* `await_suspend` - this function is called right after the suspension. It contains the actual instance of the handle of the suspended coroutine which might be stored and used later.

### Coroutine Promise
The Promise structure contains a configuration of the coroutine, which defines how it should behave. The absolute minimum that must be defined here are:
* `get_return_object` - function which constructs the coroutine handle. You can always use the code from the example.
* `initial_suspend` - can return:
    * `suspend_never` - the function will not be suspended right at the beginning and it will be suspended when the `co_await` or `co_yield` is used.
    * `suspend_always` - the function will be suspended right at the beginning and it must be resumed to even start.
* `final_suspend` - can return:
    * `suspend_never` - the function will not be suspended at the end of the function. The coroutine will be destroyed right after the function has finished.
    * `suspend_always` - the function will be suspended at the end. You must call `destroy()` function on the handle manually. Only with this setting you can check if the coroutine has finished by using the `done()` function on the handle.
> `initial_suspend` and `final_suspend` are the most important functions here, as they can control when the coroutine can be safely destroyed. We will utilize these functions later in more advanced coroutines.
* `return_void` - this function is called when the `co_return` is used. The `co_return` is called by default when the function reaches the end.
* `unhandled_exception` - used to catch any exception. To get more details about the extepction use `std::current_exception()` function.

### Using the coroutine
Ok, we have a very simple coroutine defined, but how do we use it? In our example we can see a function, which returns our coroutine handle, the `CoroHandle`. It uses `co_await` keyword to suspend it's execution. In the `main` program we call this function and gets the `handle` to it. Later, we can use the `resume()` function of that handle to resume the suspended coroutine. And that's all!

## Coroutine Tasks

You can define different classes with different `await_suspend` functionalities, all based on the same handle and promise. It is useful if you wan't to have multiple coroutine suspension behaviour in the same  coroutine. I like to call such classes Coroutine Tasks.

```c++
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
```

The output of this code will be:

```
CoroTest Before Suspend
Suspended Using Task 1
CoroTest First Resuming
CoroTest After First Resume
Suspended Using Task 2
CoroTest Second Resuming
CoroTest After Second Resume
```

## Generators - coroutines returning values

