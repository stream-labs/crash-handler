/******************************************************************************
	Copyright (C) 2016-2020 by Streamlabs (General Workings Inc)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

******************************************************************************/

#include "../util.hpp"
#include "../logger.hpp"
#include <windows.h>
#include <string>
#include <chrono>
#include <fstream>
#include <sstream>
#include <codecvt>
#include <psapi.h>
#include "nlohmann/json.hpp"
#include <filesystem>

#include "upload-window-win.hpp"

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/BucketLocationConstraint.h>
#include <aws/s3/model/PutObjectRequest.h>

#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Wininet.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "winhttp.lib")

#include "Dbghelp.h"
#pragma comment(lib, "Dbghelp.lib")

#ifndef AWS_CRASH_UPLOAD_BUCKET_KEY
#define AWS_CRASH_UPLOAD_BUCKET_KEY "KEY"
#endif

#define GET_KEY []() { return AWS_CRASH_UPLOAD_BUCKET_KEY; }()

const std::wstring appStateFileName = L"\\appState";
std::wstring appCachePath = L"";

void Util::restartApp(std::wstring path) {
	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;

	memset(&info, 0, sizeof(info));
	memset(&processInfo, 0, sizeof(processInfo));

	const std::wstring crash_handler_subdir = L"\\resources\\app.asar.unpacked/node_modules/crash-handler/crash-handler-process";
	if(path.size() <= crash_handler_subdir.size())
		return;

	std::wstring slobs_path = path.substr(0, path.size() - crash_handler_subdir.size());
	slobs_path += L"\\Streamlabs OBS.exe";
	log_info << "Slobs path to restart: " << std::string(slobs_path.begin(), slobs_path.end()) << std::endl;
	CreateProcess(slobs_path.c_str(),
		NULL,
		NULL,
		NULL,
		FALSE,
		DETACHED_PROCESS,
		NULL,
		NULL,
		&info,
		&processInfo
	);
}

void Util::runTerminateWindow(bool& shouldRestart) {
    int code = MessageBox(
        NULL,
        L"An error occurred which has caused Streamlabs OBS to close. Don't worry! If you were streaming or recording, that is still happening in the background."
        L"\n\nWhenever you're ready, we can relaunch the application, however this will end your stream / recording session.\n\n"
        L"Click the Yes button to keep streaming / recording. \n\n"
        L"Click the No button to stop streaming / recording.",
        L"An error occurred",
        MB_YESNO | MB_SYSTEMMODAL
        );
    switch (code) {
        case IDYES:
        {
            MessageBox(
                NULL,
                L"Your stream / recording session is still running in the background. Whenever you're ready, click the OK button below to end your stream / recording and relaunch the application.",
                L"Choose when to restart",
                MB_OK | MB_SYSTEMMODAL
            );
			shouldRestart = true;
            break;
        }
        case IDNO:
        default:
            break;
    }
}

static thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

std::string from_utf16_wide_to_utf8(const wchar_t* from, size_t length = -1)
{
	const wchar_t* from_end;
	if (length == 0)
		return {};
	else if (length != -1)
		from_end = from + length;
	else
		return converter.to_bytes(from);
	return converter.to_bytes(from, from_end);
}

std::wstring from_utf8_to_utf16_wide(const char* from, size_t length = -1)
{
	const char* from_end;
	if (length == 0)
		return {};
	else if (length != -1)
		from_end = from + length;
	else
		return converter.from_bytes(from);
	return converter.from_bytes(from, from_end);
}

struct ProcessInfo
{
	uint64_t handle;
	uint64_t id;
	ProcessInfo()
	{
		this->handle = 0;
		this->id = 0;
	};
	ProcessInfo(uint64_t h, uint64_t i)
	{
		this->handle = h;
		this->id = i;
	}
};

ProcessInfo open_process(uint64_t handle)
{
	ProcessInfo pi;
	DWORD       flags = PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE | PROCESS_VM_READ;
	pi.handle = (uint64_t)OpenProcess(flags, false, (DWORD)handle);
	return pi;
}

bool close_process(ProcessInfo pi)
{
	return !!CloseHandle((HANDLE)pi.handle);
}

std::string get_process_name(ProcessInfo pi)
{
	LPWSTR  lpBuffer = NULL;
	DWORD   dwBufferLength = 256;
	HANDLE  hProcess = (HANDLE)pi.handle;
	HMODULE hModule;
	DWORD   unused1;
	BOOL    bSuccess;
	/* We rely on undocumented behavior here where
	* enumerating a process' modules will provide
	* the process HMODULE first every time. */
	bSuccess = EnumProcessModules(hProcess, &hModule, sizeof(hModule), &unused1);
	if (!bSuccess)
		return {};
	while (32768 >= dwBufferLength) {
		std::wstring lpBuffer(dwBufferLength, wchar_t());
		DWORD dwReturnLength = GetModuleFileNameExW(hProcess, hModule, &lpBuffer[0], dwBufferLength);
		if (!dwReturnLength)
			return {};
		if (dwBufferLength <= dwReturnLength) {
			/* Increased buffer exponentially.
			* Notice this will eventually match
			* a perfect 32768 which is the max
			* length of an NTFS file path. */
			dwBufferLength <<= 1;
			continue;
		}
		/* Notice that these are expensive
		* but they do shrink the buffer to
		* match the string */
		return from_utf16_wide_to_utf8(lpBuffer.data());
	}
	/* Path too long */
	return {};
}

std::fstream open_file(std::string& file_path, std::fstream::openmode mode)
{
	return std::fstream(from_utf8_to_utf16_wide(file_path.c_str()), mode);
}

bool kill(ProcessInfo pinfo, uint32_t code)
{
	return TerminateProcess(reinterpret_cast<HANDLE>(pinfo.handle), code);
}

void Util::check_pid_file(std::string& pid_path) {
	std::fstream::openmode mode = std::fstream::in | std::fstream::binary;
	std::fstream pid_file(open_file(pid_path, mode));
	union
	{
		uint64_t pid;
		char     pid_char[sizeof(uint64_t)];
	};
	if (!pid_file)
		return;
	pid_file.read(&pid_char[0], 8);
	pid_file.close();
	ProcessInfo pi = open_process(pid);
	if (pi.handle == 0)
		return;
	std::string name = get_process_name(pi);
	if (name.find("crash-handler-process.exe") != std::string::npos) {
		kill(pi, -1);
	}
	close_process(pi);
}

void Util::write_pid_file(std::string& pid_path) {
	std::fstream::openmode mode = std::fstream::out | std::fstream::binary;
	std::fstream pid_file(open_file(pid_path, mode));
	union
	{
		uint64_t pid;
		char     pid_char[sizeof(uint64_t)];
	};
	if (!pid_file)
		return;

	pid = GetCurrentProcessId();
	if (pid == 0)
		return;
	pid_file.write(&pid_char[0], 8);
	pid_file.close();
}

std::string Util::get_temp_directory() {
	constexpr DWORD tmp_size = MAX_PATH + 1;
	std::wstring    tmp(tmp_size, wchar_t());
	GetTempPathW(tmp_size, &tmp[0]);
	/* Here we resize an in-use string and then re-use it.
	* Note this is only okay because the long path name
	* will always be equal to or larger than the short
	* path name */
	DWORD tmp_len = GetLongPathNameW(&tmp[0], NULL, 0);
	tmp.resize(tmp_len);
	/* Note that this isn't a hack to use the same buffer,
	* it's explicitly meant to be used this way per MSDN. */
	GetLongPathNameW(&tmp[0], &tmp[0], tmp_len);
	return from_utf16_wide_to_utf8(tmp.data());
}

void Util::setCachePath(std::wstring path)
{
	appCachePath = path;
}

void Util::updateAppState(bool unresponsive_detected)
{
	const std::string freez_flag = "window_unresponsive";
	const std::string flag_name  = "detected";

	std::ifstream state_file(appCachePath + appStateFileName, std::ios::in);
	if (!state_file.is_open())
		return;

	std::ostringstream buffer;
	buffer << state_file.rdbuf();
	state_file.close();

	std::string current_status = buffer.str();
	if (current_status.size() == 0)
		return;

	std::string    updated_status      = "";
	std::string    existing_flag_value = "";
	nlohmann::json jsonEntry           = nlohmann::json::parse(current_status);
	try {
		existing_flag_value = jsonEntry.at(flag_name);
	} catch (...) {
	}
	if (unresponsive_detected) {
		if (existing_flag_value.empty())
			jsonEntry[flag_name] = freez_flag;
	} else {
		if (existing_flag_value.compare(freez_flag) == 0)
			jsonEntry[flag_name] = "";
	}
	updated_status = jsonEntry.dump(-1);

	std::ofstream out_state_file;
	out_state_file.open(appCachePath + appStateFileName, std::ios::trunc | std::ios::out);
	if (!out_state_file.is_open())
		return;

	out_state_file << updated_status << "\n";
	out_state_file.flush();
	out_state_file.close();
}

bool Util::removeMemoryDump(const std::wstring& dumpPath, const std::wstring& dumpFileName)
{
	bool ret = false;
	std::filesystem::path memoryDumpFolder = dumpPath;
	std::filesystem::path memoryDumpFile   = memoryDumpFolder;
	memoryDumpFile.append(dumpFileName);
	try {
		std::filesystem::remove(memoryDumpFile);
		ret = true;
	} catch (...) {}
	return ret;
}

bool Util::saveMemoryDump(uint32_t pid, const std::wstring& dumpPath, const std::wstring& dumpFileName)
{
	bool dumpSaved = false;

	std::filesystem::path memoryDumpFolder = dumpPath;
	std::filesystem::path memoryDumpFile   = memoryDumpFolder;
	memoryDumpFile.append(dumpFileName);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (hProcess == NULL || hProcess == INVALID_HANDLE_VALUE) {
		log_info << "Failed to open process to get memory dump." << std::endl;
		return false;
	}

	bool           enoughDiskSpace = false;
	ULARGE_INTEGER diskBytesAvailable;
	if (GetDiskFreeSpaceEx(memoryDumpFolder.generic_wstring().c_str(), &diskBytesAvailable, NULL, NULL)) {
		PROCESS_MEMORY_COUNTERS pmc;
		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
			log_info << "Disk available space " << diskBytesAvailable.QuadPart << " , process ram size "
			         << pmc.WorkingSetSize << std::endl;

			// There is now way to know a size of memory dump.
			// On test crashes it was about two times bigger than a ram size used by a process.
			if (pmc.WorkingSetSize < diskBytesAvailable.QuadPart * 2) {
				enoughDiskSpace = true;
			}
		}
	}

	if (!enoughDiskSpace) {
		log_info << "Failed to create memory dump. Not enough disk space  available" << std::endl;

		CloseHandle(hProcess);
		return false;
	}

	HANDLE hFile = CreateFile(
	    memoryDumpFile.generic_wstring().c_str(),
	    GENERIC_READ | GENERIC_WRITE,
	    0,
	    NULL,
	    CREATE_ALWAYS,
	    FILE_ATTRIBUTE_NORMAL,
	    NULL);

	if (hFile && hFile != INVALID_HANDLE_VALUE) {
		const DWORD CD_Flags = MiniDumpWithFullMemory | MiniDumpWithHandleData | MiniDumpWithThreadInfo
		                       | MiniDumpWithProcessThreadData | MiniDumpWithFullMemoryInfo
		                       | MiniDumpWithUnloadedModules | MiniDumpIgnoreInaccessibleMemory ;

		BOOL ret = MiniDumpWriteDump(hProcess, pid, hFile, (MINIDUMP_TYPE)CD_Flags, 0, 0, 0);
		if (ret) {
			dumpSaved = true;
			long long bytes_to_send = std::filesystem::file_size(memoryDumpFile);
			UploadWindow::getInstance()->setTotalBytes(bytes_to_send);
			log_info << "Memory dump saved. " << std::endl;
		} else {
			log_info << "Failed to save memory dump. err code = " << GetLastError() << std::endl;
		}

		CloseHandle(hFile);
	} else {
		log_info << "Failed to create memory dump file \"" << memoryDumpFile.generic_string() << "\"" << std::endl;
	}
	CloseHandle(hProcess);

	return dumpSaved;
}


std::mutex              upload_mutex;
std::condition_variable upload_variable;
long long               total_sent_amout = 0;
std::chrono::steady_clock::time_point last_progress_update;

void PutObjectAsyncFinished(
    const Aws::S3::S3Client*                                      s3Client,
    const Aws::S3::Model::PutObjectRequest&                       request,
    const Aws::S3::Model::PutObjectOutcome&                       outcome,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context)
{
	if (outcome.IsSuccess()) {
		log_info << "PutObjectAsyncFinished: Finished uploading '" << context->GetUUID() << "'." << std::endl;
	} else {
		total_sent_amout = -1;
		log_error << "PutObjectAsyncFinished failed " << outcome.GetError().GetMessage() << std::endl;
	}

	upload_variable.notify_one();
}

bool PutObjectAsync(
    const Aws::S3::S3Client& s3_client,
    const Aws::String&       bucket_name,
    const std::wstring&      file_path,
    const std::wstring&      file_name)
{
	log_info << "PutObjectAsync started  " << std::endl;
	Aws::S3::Model::PutObjectRequest request;
	request.SetDataSentEventHandler([](const Aws::Http::HttpRequest*, long long amount) {
		total_sent_amout += amount;

		std::chrono::steady_clock::time_point now_time = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(now_time - last_progress_update).count() > 500) {
			last_progress_update = now_time;
			UploadWindow::getInstance()->setUploadProgress(total_sent_amout);
		}
	});

	request.SetBucket(bucket_name);

	Aws::String aws_file_name = Aws::String(std::string(file_name.begin(), file_name.end()));
	request.SetKey(Aws::String("crash_memory_dumps/") + aws_file_name);

	std::shared_ptr<Aws::IOStream> input_data;
	std::fstream*                  fs            = new std::fstream();
	std::filesystem::path          uploaded_file = file_path;
	uploaded_file.append(file_name);
	input_data.reset(fs);
	try {
		fs->exceptions(std::fstream::failbit | std::fstream::badbit);
		fs->open(uploaded_file, std::ios_base::in | std::ios_base::binary);
	} catch (std::fstream::failure f) {
		log_info << "PutObjectAsync failed open file " << uploaded_file.generic_string() << std::endl;
		return false;
	} catch (std::exception e) {
		log_info << "PutObjectAsync failed open file " << uploaded_file.generic_string() << std::endl;
		return false;
	}
	fs->exceptions(std::fstream::goodbit);

	request.SetBody(input_data);

	log_info << "PutObjectAsync ready to call PutObjectAsync " << std::endl;
	std::shared_ptr<Aws::Client::AsyncCallerContext> context =
	    Aws::MakeShared<Aws::Client::AsyncCallerContext>("PutObjectAllocationTag");
	context->SetUUID(request.GetKey());
	s3_client.PutObjectAsync(request, PutObjectAsyncFinished, context);
	log_info << "PutObjectAsync finished. Wait for async result." << std::endl;

	return true;
}

bool Util::uploadToAWS(const std::wstring& dumpPath, const std::wstring& dumpFileName)
{
	UploadWindow::getInstance()->uploadStarted();
	bool            ret = false;
	Aws::SDKOptions options;
	Aws::InitAPI(options);
	{
		const Aws::String bucket_name = "streamlabs-obs-user-cache";
		const Aws::String region      = "us-west-2";
		const Aws::String accessIDKey = "AKIAIAINC32O7I3KUJGQ";
		const Aws::String Key   = GET_KEY;

		std::unique_lock<std::mutex> lock(upload_mutex);

		Aws::Client::ClientConfiguration config;

		if (!region.empty()) {
			config.region = region;
		}
		config.scheme = Aws::Http::Scheme::HTTPS;
		config.verifySSL = false;
		config.followRedirects = Aws::Client::FollowRedirectsPolicy::NEVER;
		Aws::Auth::AWSCredentials aws_credentials;
		aws_credentials.SetAWSAccessKeyId(accessIDKey);
		aws_credentials.SetAWSSecretKey(Key);

		Aws::S3::S3Client s3_client(
		    aws_credentials, config, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false);
		log_info << "Upload to ASW ready to start upload" << std::endl;
		if (!PutObjectAsync(s3_client, bucket_name, dumpPath, dumpFileName)) {
			log_info << "Upload to ASW PutObjectAsync failed" << std::endl;
			UploadWindow::getInstance()->uploadFailed();
		} else {
			upload_variable.wait(lock);
			if (total_sent_amout > 0) {
				log_info << "Upload to ASW File upload attempt completed." << std::endl;
				UploadWindow::getInstance()->uploadFinished();
				ret = true;
			} else {
				UploadWindow::getInstance()->uploadFailed();
			}
		}
	}
	Aws::ShutdownAPI(options);
	return ret;
}
