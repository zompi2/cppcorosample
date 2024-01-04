// Copyright (c) 2024 Damian Nowakowski. All rights reserved.

// This is the example of coroutine in Unreal Engine 5.
// For more details check: https://github.com/zompi2/cppcorosample

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
public:

    float TimeRemaining;
    std::coroutine_handle<CoroPromise> Handle;
    FTSTicker::FDelegateHandle TickerHandle;

    WaitSecondsTask(float DelayTime) : TimeRemaining(DelayTime) {}

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
        if (CameraManager)
        {
            for (float Fade = 0.f; Fade <= 1.f; Fade += .1f)
            {
                CameraManager->SetManualCameraFade(Fade, FColor::Black, false);
                co_await WaitSecondsTask(.1f);
            }
            CameraManager->SetManualCameraFade(1.f, FColor::Black, false);
        }
    }
}