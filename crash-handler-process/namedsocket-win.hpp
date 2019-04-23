#include "namedsocket.hpp"
#include <vector>

class NamedSocket_win : public NamedSocket {
public:
	NamedSocket_win();
	virtual ~NamedSocket_win();

public:
	HANDLE m_handle;

public:
	virtual bool connect() override;
	virtual bool read(std::vector<Process*>*, std::mutex* mu, bool* exit) override;
	virtual void disconnect() override;
	virtual bool flush() override;

	virtual HANDLE getHandle() override;
};