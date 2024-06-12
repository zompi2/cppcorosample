# C++20 coroutine samples

This is my understanding of [C++20 coroutines](https://en.cppreference.com/w/cpp/language/coroutines) with some samples.  
I've written this document to help others (and myself) to understand what coroutines are, how to write them in C++ and when they are useful.   
This form of a documentation was inspired by the other github manuals, like [GAS](https://github.com/tranek/GASDocumentation) and [MASS](https://github.com/Megafunk/MassSample).  
I wanted to create a collaborative document which should be easier to understand and to learn from than the official cpp reference, which I personally treat as a very good api reference, but not a good "learn the basics" material.  
If you notice any issue or have any idea how to improve the following examples feel free to write an issue or make a pull request.

# Index
* [What is a coroutine](#what-is-a-coroutine)
* [The simpliest c++ coroutine example possible](#the-simpliest-c-coroutine-example-possible)
* [Coroutine Tasks](#coroutine-tasks)
* [Generators - coroutines returning values](#generators---coroutines-returning-values)
* [Camera Fade Out for Unreal Engine 5](#camera-fade-out-for-unreal-engine-5)

# What is a coroutine?

A coroutine is a function which execution can be suspended (without suspending the rest of the code) and it can be resumed later. Such functionality is very useful if we have a function that needs to wait for something, for example for the response from http request or for the result of the other function that runs on another thread.  
Sure, we can use delegates, or lambdas instead of coroutines and get the similar behaviour, but having single line which can command the code to wait is more clear, readable and elegant solution.

## Coroutines in other languages

Coroutines have been a part of other languages for a long time, for example in C#. One of the most famous example is the code for [camera fade out for Unity game engine](https://docs.unity3d.com/Manual/Coroutines.html).

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

Normally the for-loop would simply set the material alpha to 0, but because this is a coroutine it will wait for 0.1 seconds after each for-loop iteration which will lead to a fluid and visible change of the material alpha. We can recognize that this is a coroutine because of the presence of the `yield` keyword. The `yield` suspends the whole function while the Unity built in coroutines system will resume it.

## Coroutines in C++
Function is a coroutine in C++ when there is one of the following keywords inside:
* `co_await` - it simply suspends the function
* `co_yield` - it suspends the function and returns a value
* `co_return` - it completes the function with or without returning a value

[Back to index](#index)

# The simpliest C++ coroutine example possible

This is (probably) the most simple C++ coroutine example possible. There is still a lot of happening there, but all of it will be explained below.

This code with comments is also inside the `Samples` directory here: [01_CoroSimpleExample.cpp](Samples/01_CoroSimpleExample.cpp)

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

## Coroutine Handle
This is a structure based on the `std::coroutine_handle`. It controls the flow of the coroutine and will be used to resume the suspended coroutine later. The absolute minimum which needs to be defined inside the Handle are:
* `promise_type` - this is a type of the coroutine Promise. What is a Promise will be explained later.
* `await_resume` - this function is called when the coroutine is resumed.
* `await_ready` - this function is called before the coroutine suspension. If it returns `false` the coroutine will be suspended. If it returns `true` it will ignore the suspension.
    > Note: You can also write `void await_ready() {}` and it will behave the same as if it return `false`.
* `await_suspend` - this function is called right after the suspension. It contains the actual instance of the handle of the suspended coroutine which might be stored and used later.

## Coroutine Promise
The Promise structure contains a configuration of the coroutine, which defines how it should behave. The absolute minimum that must be defined here are:
* `get_return_object` - function which constructs the coroutine handle.
* `initial_suspend` - can return:
    * `suspend_never` - the function will not be suspended right at the beginning and it will be suspended when the `co_await` or `co_yield` is used.
    * `suspend_always` - the function will be suspended right at the beginning and it must be resumed to even start.
* `final_suspend` - can return:
    * `suspend_never` - the function will not be suspended at the end of the function. The coroutine will be destroyed right after the function has finished.
    * `suspend_always` - the function will be suspended at the end. You must call `destroy()` function on the handle manually. Only with this setting you can check if the coroutine has finished by using the `done()` function on the handle.
    > Note: `initial_suspend` and `final_suspend` are one of the most important functions here, as they can control when the coroutine can be safely destroyed. We will utilize these functions later in more advanced coroutines.
* `return_void` - this function is called when the `co_return` is used. The `co_return` is called by default when the function reaches the end.
* `unhandled_exception` - used to catch any exception. To get more details about the catched extepction use `std::current_exception()` function.

## Using the coroutine
Ok, we have a very simple coroutine defined, but how do we use it? In our example we have a function, which returns our coroutine handle, the `CoroHandle`. It uses `co_await` keyword to suspend it's execution. In the `main` program we call this function and gets the `handle` to it. Later, we can use the `resume()` function on that handle to resume the suspended coroutine. And that's all!

[Back to index](#index)

# Coroutine Tasks
Coroutine Tasks are the elegant way to define different coroutine Handles using the same Handle base and Promise, reducing the amount of boilerplate code you need to write.

The code below implements two different tasks: `CoroTaskA` and `CoroTaskB`. They can be called when using `co_await` in the same coroutine which returns the same type of Handle.

Tasks are defined using the class instead of structs, so they allow better code encapsulation.

This code with comments is also inside the `Samples` directory here: [02_CoroTasks.cpp](Samples/02_CoroTasksExample.cpp)

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

class CoroTaskBase
{
public:
    void await_resume() {}
    bool await_ready() { return false; }
};

class CoroTaskA : public CoroTaskBase
{
public:
    void await_suspend(std::coroutine_handle<CoroPromise> Handle)
    {
        std::cout << "Suspended Using Task A\n";
    };
};

class CoroTaskB : public CoroTaskBase
{
public:
    void await_suspend(std::coroutine_handle<CoroPromise> Handle)
    {
        std::cout << "Suspended Using Task B\n";
    };
};

CoroHandle CoroTest()
{
    std::cout << "CoroTest Before Suspend\n";
    co_await CoroTaskA();
    std::cout << "CoroTest After First Resume\n";
    co_await CoroTaskB();
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
Suspended Using Task A
CoroTest First Resuming
CoroTest After First Resume
Suspended Using Task B
CoroTest Second Resuming
CoroTest After Second Resume
```

The Promise definition is the same as previously. The coroutine Handle has been split into two parts:
* the definition of the coroutine Handle which uses the defined Promise
* the definition of Tasks which implements the Handle functions such as `await_resume`, `await_ready` and `await_suspend`.

As you can see in this example we can call different Tasks inside the same coroutine function.

[Back to index](#index)

# Generators - coroutines returning values
Just suspending a running function is neat, but we can do more. Coroutines can be written in such way that they can store values which can be used later. Such coroutines are called Generators. Let's write a generic Generator and then use it to write a coroutine which will give us every value of the beloved Fibonacci sequence one by one.

This code with comments is also inside the `Samples` directory here: [03_CoroGenerators.cpp](Samples/03_CoroGenerators.cpp)


```c++
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
```

The output of this code will be:

```
1 1 2 3 5 8 13 21 34 55
```

Once again, there is a lot to cover. Let's go through this step by step.

## Coroutine Promise for Generator
The Promise for a Generator is slightly different. You can notice few differences:  
* `T Value` - Promise stores a value of a generic type. This value can be obtained later by a Generator.
* `get_return_object` - doesn't return coroutine Handle, but a Generator with a coroutine Handle passed as an argument to the constructor.
* `initial_suspend` and `final_suspend` returns `suspend_always`. This is important, because with such setup we have the full control over the coroutine flow.
* `yield_value` - this function is called every time when `co_yield` is used. It stores the given value to the `Value` variable and returns `suspend_always` in order to suspend the function.

The Promise is defined inside the Generator struct in order to keep everything in one place and to avoid declaration loop.

## Generator
The generator itself has few interesting parts as well:
* `Handle` - this is the coroutine Handle saved from the Generator constructor.
* `operator bool()` - is very handy for resuming the coroutine and checking if the coroutine has finished. To check if the coroutine is done we use `done()` function on the coroutine Handle. We can use it safely, because the `final_suspend` is set to `suspend_always`, so the coroutine Handle will not be destroyed automatically when the function is finished.
* `operator()` -  will be used to get a stored value from the Promise.
* Constructor - receives and remembers the coroutine Handle. The Generator construcor is used in `get_return_object` function in the Promise.
* Destructor - explicitly destroys the coroutine Handle using `destroy()` function. It must be used, because the `final_suspend` is set to `suspend_always`, so it won't be destroyed automatically.

## Fibonacci Generator
`FibonacciGenerator` function returns the Generator which stores the `int` value. Every next value of the Fibonacci sequence is yielded, which means the function is suspended and the value is stored.

## Using Fibonacci Generator
When our Generator is constructed it automatically suspends, because of the `initial_suspend` set to `suspend_always`. Inside the while loop the `operator bool()` override is used to resume the Generator and check if the coroutine has already finished. Inside the while loop the `operator()` override is used to get lastly yielded value.

[Back to index](#index)

# Camera Fade Out for Unreal Engine 5
[At the beginning of this document](#coroutines-in-other-languages) I showed the example of the coroutine used in Unity game engine to fade out the camera. Let's write the same functionality but for Unreal Engine 5.3 which officially supports C++20, so it should be possible to implement it.

This code with comments is also inside the `Samples` directory here: [04_CoroUE5FadeOut.cpp](Samples/04_CoroUE5FadeOut.cpp)

```c++
#include <coroutine>
#include "Kismet/GameplayStatics.h"

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

class WaitSecondsTask
{
private:
    float TimeRemaining;
    std::coroutine_handle<CoroPromise> Handle;
    FTSTicker::FDelegateHandle TickerHandle;

public:
    WaitSecondsTask(float Time) : TimeRemaining(Time) {}

    void await_resume() {}
    bool await_ready() { return TimeRemaining <= 0.f; }
    void await_suspend(std::coroutine_handle<CoroPromise> CoroHandle)
    {
        Handle = CoroHandle;
        TickerHandle = FTSTicker::GetCoreTicker().AddTicker(TEXT("CoroWaitSeconds"), 0.f, [this](float DeltaTime) -> bool
        {
            TimeRemaining -= DeltaTime;
            if (TimeRemaining <= 0.f)
            {
                FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
                Handle.resume();
            }
            return true;
        });
    };
};

CoroHandle CoroFadeOut()
{
    if (GWorld)
    {
        APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GWorld, 0);
        for (int32 Fade = 0; Fade <= 100; Fade += 10)
        {
            if (IsValid(CameraManager))
            {
                CameraManager->SetManualCameraFade((float)Fade * .01f, FColor::Black, false);
            }
            co_await WaitSecondsTask(.1f);
        }
    }
}
```

The Handle and the Promise are the same as usual. To make it work we need a coroutine Task, which will suspend the coroutine for a given amount of time and the actual coroutine which will fade out the camera.

## Wait Seconds Task
This is a specific case of a coroutine Task which resumes itself. It must somehow receive the coroutine Handle and the amount of seconds we want to wait.  
The time to wait is passed as an argument in the constructor.  
The coroutine Handle is obtained from the `await_suspend` function.  
The `await_suspend` function starts the Unreal Engine ticker which will resume the suspended coroutine using the received coroutine Handle.

## Fade Out function
The fade out function changes the camera fade in a for loop in 10 steps every 0.1 second. You can notice the `co_await WaitSecondsTask(.1f)` after every loop iteration, which triggers our waiting coroutine Task.

In order to use this function, simply call it in your project.

## More coroutines for UE5
If you are interested witch coroutines implementation for Unreal Engine 5 check out this amazing plugin [UE5Coro](https://github.com/landelare/ue5coro). Coroutines are also implemented in my UE plugin [Enhanced Code Flow](https://github.com/zompi2/UE4EnhancedCodeFlow) as well :)

[Back to index](#index)