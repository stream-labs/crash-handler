#include "namedsocket.hpp"
#include <vector>

class NamedSocket_win : public NamedSocket {
public:
	NamedSocket_win(std::string path);
	virtual ~NamedSocket_win();

public:
	HANDLE m_handle;

public:
	virtual bool connect() override;
	virtual size_t read(char* buf, size_t length) override;
	virtual size_t write(const char* buf, const size_t length) override;
	virtual void disconnect() override;
	virtual bool flush() override;

	virtual HANDLE getHandle() override;
};