#include "../process.hpp"

class Process_OSX : public Process {
public:
    Process_OSX(int32_t pid, bool isCritical);
    virtual ~Process_OSX() {};

public:
    virtual int32_t  getPID(void)     override;
    virtual bool     isCritical(void) override;
    virtual int     isAlive(void)    override;
    virtual void     startMemoryDumpMonitoring(const std::wstring& eventName, const std::wstring& eventFinishedName, const std::wstring& dumpPath) override;
    virtual bool     isResponsive(void) override;
    virtual void     terminate(void)  override;
	virtual void     terminateNicely(void)  override;
};