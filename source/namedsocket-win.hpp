#include "namedsocket.hpp"
#include <vector>
#include <windows.h>

class NamedSocket_win : public NamedSocket {
public:
	NamedSocket_win(std::string path);
	virtual ~NamedSocket_win();

private:
	HANDLE m_handle;

public:
	// Reading
	virtual size_t read(char* buf, size_t length) override;

	// Writing
	virtual size_t write(const char* buf, const size_t length) override;

	virtual bool flush() override;
};