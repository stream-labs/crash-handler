#include <vector>

#define MINIMUM_BUFFER_SIZE 256

class NamedSocket {
public:
	virtual size_t read(char* buf, size_t length) = 0;
	virtual size_t write(const char* buf, const size_t length) = 0;
	virtual bool flush() = 0;
};