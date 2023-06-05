#include "ticker.h"

namespace app {
    void ApplicationUpdateTimer::Start(){
        ScheduleTick();
    }

    void ApplicationUpdateTimer::Tick(){
        _application.AddTime(_updatePeriod.count());

        ScheduleTick();
    }

    void ApplicationUpdateTimer::ScheduleTick(){
        _timer.expires_after(_updatePeriod);

        _timer.async_wait([self = shared_from_this()](sys::error_code ec){
            if (ec) {
                throw std::runtime_error("timer: "+ec.message());
            }

            self->Tick();
        });
    }
}