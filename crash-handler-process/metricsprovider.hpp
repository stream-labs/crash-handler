#include <thread>
#include <mutex>
#include <windows.h>
#include <fstream>
#include <iostream>

class MetricsProvider
{
	const wchar_t* CrashMetricsFilename = L"backend-metrics.doc";

public:

	~MetricsProvider();

	bool Initialize(std::string name);
	bool ConnectToClient();
	void StartPollingEvent();

private:

	void InitializeMetricsFile();
	void MetricsFileSetStatus(std::string status);
	void MetricsFileClose();

private:

	HANDLE        m_Pipe;
	bool          m_StopPolling = false;
	std::thread   m_PollingThread;
	std::ofstream m_MetricsFile;
};