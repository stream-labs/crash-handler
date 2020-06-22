#include <node.h>

using namespace v8;

NAN_MODULE_INIT(crash_init) 
{
    /// Functions ///
    // NODE_SET_METHOD(exports, "startHook", StartHotkeyThreadJS);
};

NAN_MODULE_WORKER_ENABLED(crash_handler, crash_init)