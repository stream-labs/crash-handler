#include "../process.hpp"

class Process_OSX : public Process {
public:
    Process_OSX(int32_t pid, bool isCritical);
    virtual ~Process_OSX() {};

public:
    virtual int32_t  getPID(void)     override;
    virtual bool     isCritical(void) override;
    virtual bool     isAlive(void)    override;
    virtual void     startMemoryDumpMonitoring(void) override;
    virtual bool     isResponsive(void) override;
    virtual void     terminate(void)  override;
};