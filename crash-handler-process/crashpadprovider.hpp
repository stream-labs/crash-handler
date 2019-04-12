#include <string>
#include <windows.h>

#define OS_WIN
#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/settings.h"

class CrashpadProvider
{
	

public:

	~CrashpadProvider();

	bool Initialize(std::string url);
	void Shutdown();

private:

private:

    std::wstring                                   m_AppdataDath;
    crashpad::CrashpadClient                       m_Client;
    std::unique_ptr<crashpad::CrashReportDatabase> m_Database;
    std::string                                    m_Url;
    std::vector<std::string>                       m_Arguments;
    std::map<std::string, std::string>             m_Annotations;
    LPTOP_LEVEL_EXCEPTION_FILTER                   m_CrashpadInternalExceptionFilterMethod = nullptr;

};