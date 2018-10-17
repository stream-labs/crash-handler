#include <node.h>

using namespace v8;

void init(Local<Object> exports) {
    /// Functions ///
    // NODE_SET_METHOD(exports, "startHook", StartHotkeyThreadJS);
}

NODE_MODULE(crash_handler_module, init)