// Copyright (c) 2024 Damian Nowakowski. All rights reserved.

// This is the example of coroutine in Unreal Engine 5.
// For more details check: https://github.com/zompi2/cppcorosample

#include <coroutine>
#include "Kismet/GameplayStatics.h"

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

// Definition of the coroutine Task used for suspending a specific amount of time
class WaitSecondsTask
{
private:

    // Time left to resume   
    float TimeRemaining;

    // Coroutine Handle to resume after time
    std::coroutine_handle<CoroPromise> Handle;

    // Unreal ticker handle
    FTSTicker::FDelegateHandle TickerHandle;

public:

    // Task constructor which stores the amount of time to being suspended
    WaitSecondsTask(float Time) : TimeRemaining(Time) {}

    // Called when the coroutine has been resumed
    void await_resume() {}

    // Ignore suspension if the given time is invalid
    bool await_ready() { return TimeRemaining <= 0.f; }

    // Called when the coroutine has been suspended using this Task
    void await_suspend(std::coroutine_handle<CoroPromise> CoroHandle)
    {
        // Remember the coroutine Handle
        Handle = CoroHandle;

        // Start the Unreal ticker
        TickerHandle = FTSTicker::GetCoreTicker().AddTicker(TEXT("CoroWaitSeconds"), 0.f, [this](float DeltaTime) -> bool
        {
            // When the ticker ticks for the desired amount of time...
            TimeRemaining -= DeltaTime;
            if (TimeRemaining <= 0.f)
            {
                // ... stop the Unreal ticker and resume the coroutine
                FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
                Handle.resume();
            }
            return true;
        });
    };
};

// Definition of the coroutine function which fades out the camera
CoroHandle CoroFadeOut()
{
    // Warning, there will be velociraptors: World should be obtained by a World Context Object,
    // but just for the example sake we use nasty GWorld.
    if (GWorld)
    {
        APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GWorld, 0);
        for (int32 Fade = 0; Fade <= 100; Fade += 10)
        {
            // Because the WaitSecondsTask can tick between worlds we can't be sure if the Camera Manager
            // is valid all the time.
            if (IsValid(CameraManager))
            {
                CameraManager->SetManualCameraFade((float)Fade * .01f, FColor::Black, false);
            }
            co_await WaitSecondsTask(.1f);
        }
    }
}
