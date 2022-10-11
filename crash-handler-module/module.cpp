#include <napi.h>

Napi::Object main_node(Napi::Env env, Napi::Object exports)
{
	return exports;
}

NODE_API_MODULE(crash_handler, main_node)