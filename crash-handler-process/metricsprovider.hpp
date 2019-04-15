#include <thread>
#include <mutex>
#include <windows.h>
#include <fstream>
#include <iostream>
#include <map>

class MetricsProvider
{
	const wchar_t* CrashMetricsFilename = L"backend-metrics.doc";
	const static int StringSize = 64;

	enum class MessageType
	{
		Pid,
		Tag, 
		Status, 
		Shutdown
	};

	struct MetricsMessage
	{
		MessageType type;
		char param1[StringSize];
		char param2[StringSize];
	};

public:

	~MetricsProvider();

	bool Initialize(std::string name);
	void Shutdown();
	bool ConnectToClient();
	void StartPollingEvent();

	bool ServerIsActive();
	bool ServerExitedSuccessfully();

private:

	void InitializeMetricsFile();
	void MetricsFileSetStatus(std::string status);
	void MetricsFileClose();
    void SendMetricsReport(std::string status);
    std::string GetMetricsFileStatus();

private:

	HANDLE        m_Pipe;
	bool		  m_PipeActive = false;
	bool          m_StopPolling = false;
	bool          m_ServerExitedSuccessfully = false;
	std::thread   m_PollingThread;
	std::ofstream m_MetricsFile;
	DWORD         m_ServerPid = 0;
    std::wstring  m_MetricsFilePath;
    std::string   m_LastStatus;
    std::map<std::string, std::string> m_ReportTags;
};