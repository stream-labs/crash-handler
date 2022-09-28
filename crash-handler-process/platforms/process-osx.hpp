#include "../process.hpp"

class Process_OSX : public Process {
public:
    Process_OSX(int32_t pid, bool isCritical);
    virtual ~Process_OSX() {};

public:
    virtual int32_t  getPID(void)     override;
    virtual bool     isCritical(void) override;
    virtual bool     isAlive(void)    override;
    virtual bool     isUnResponsive(void) override;
    virtual void     terminate(void)  override;
    
public:
    virtual void     startMemoryDumpMonitoring(const std::wstring& eventName_Start, const std::wstring& eventName_Fail, const std::wstring& eventName_Success, const std::wstring& dumpPath, const std::wstring& dumpName) override;
};