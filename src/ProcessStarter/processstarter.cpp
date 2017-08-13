#include "stdafx.h"
#include "ProcessStarter.h"
#include "userenv.h"
#include "wtsapi32.h"
#include "winnt.h"

ProcessStarter::ProcessStarter(const std::wstring& processPath, const std::wstring& arguments)
    : processPath_(processPath), arguments_(arguments)
{
}

HANDLE primaryToken = 0;
PHANDLE ProcessStarter::GetCurrentUserToken()
{
    int dwSessionId = 0;
    CurrentUserToken();
    auto currentToken = hToken;
    BOOL bRet = DuplicateTokenEx(currentToken, TOKEN_ALL_ACCESS, 0, SecurityImpersonation, TokenPrimary, &primaryToken);
    int errorcode = GetLastError();

    if (bRet == false) {
        return 0;
    }

    return &primaryToken;
}

BOOL ProcessStarter::Run()
{
    //PHANDLE primaryToken = GetCurrentUserToken();
    CurrentUserToken();
    PHANDLE primaryToken = &hToken;

    if (primaryToken == 0) {
        return FALSE;
    }

    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION processInfo;
    StartupInfo.cb = sizeof(STARTUPINFO);
    SECURITY_ATTRIBUTES Security1;
    SECURITY_ATTRIBUTES Security2;
    std::wstring command = L"\"" + processPath_ + L"\"";

    if (arguments_.length() != 0) {
        command += L" " + arguments_;
    }

    void* lpEnvironment = NULL;
    BOOL resultEnv = CreateEnvironmentBlock(&lpEnvironment, primaryToken, FALSE);

    if (resultEnv == 0) {
        long nError = GetLastError();
        lpEnvironment = NULL;
    }

    BOOL result = CreateProcessAsUser(primaryToken, 0, (LPWSTR)(command.c_str()), &Security1, &Security2, FALSE, CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT, lpEnvironment, 0, &StartupInfo, &processInfo);
    DestroyEnvironmentBlock(lpEnvironment);
    CloseHandle(primaryToken);
    return result;
}