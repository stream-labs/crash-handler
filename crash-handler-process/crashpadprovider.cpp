#include "crashpadprovider.hpp"
#include <windows.h>
#include <WinBase.h>
#include "Shlobj.h"
#include "DbgHelp.h"
#include <WinBase.h>
#include <iostream>
#include <filesystem>

#include <nlohmann/json.hpp>

CrashpadProvider::~CrashpadProvider()
{

}

bool CrashpadProvider::Initialize(std::string url)
{
    bool isPreview = true;


    HRESULT hResult;
    PWSTR   ppszPath;

    hResult = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &ppszPath);

    m_AppdataDath.assign(ppszPath);
    m_AppdataDath.append(L"\\obs-studio-node-server");

    CoTaskMemFree(ppszPath);


    m_Arguments.push_back("--no-rate-limit");

    std::wstring handler_path(L"node_modules\\crash-handler\\crashpad_handler.exe");

    url = isPreview
        ? std::string("https://sentry.io/api/1406061/minidump/?sentry_key=7376a60665cd40bebbd59d6bf8363172")
        : std::string("https://sentry.io/api/1283431/minidump/?sentry_key=ec98eac4e3ce49c7be1d83c8fb2005ef");

    auto db = base::FilePath(m_AppdataDath);
    auto handler = base::FilePath(handler_path);

    m_Database = crashpad::CrashReportDatabase::Initialize(db);
    if (m_Database == nullptr || m_Database->GetSettings() == NULL)
        return false;

    m_Database->GetSettings()->SetUploadsEnabled(true);

    bool rc = m_Client.StartHandler(handler, db, db, url, m_Annotations, m_Arguments, true, true);
    if (!rc)
        return false;

    rc = m_Client.WaitForHandlerStart(INFINITE);
    if (!rc)
        return false;

    return true;
}