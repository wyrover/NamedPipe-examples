#ifndef _PROCESS_STARTER_H_
#define _PROCESS_STARTER_H_

#include "windows.h"
#include "winbase.h"
#include <comdef.h>

#include <string>
#define MAX_NAME 256

class ProcessStarter
{
public:
    ProcessStarter(const std::wstring& processPath, const std::wstring& arguments = L"");
    PHANDLE GetCurrentUserToken();
    BOOL Run();

    BOOL GetLogonFromToken(HANDLE hToken, _bstr_t& strUser, _bstr_t& strdomain)
    {
        DWORD dwSize = MAX_NAME;
        BOOL bSuccess = FALSE;
        DWORD dwLength = 0;
        strUser = "";
        strdomain = "";
        PTOKEN_USER ptu = NULL;

        //Verify the parameter passed in is not NULL.
        if (NULL == hToken)
            goto Cleanup;

        if (!GetTokenInformation(
                hToken,         // handle to the access token
                TokenUser,    // get information about the token's groups
                (LPVOID)ptu,   // pointer to PTOKEN_USER buffer
                0,              // size of buffer
                &dwLength       // receives required buffer size
            )) {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                goto Cleanup;

            ptu = (PTOKEN_USER)HeapAlloc(GetProcessHeap(),
                                         HEAP_ZERO_MEMORY, dwLength);

            if (ptu == NULL)
                goto Cleanup;
        }

        if (!GetTokenInformation(
                hToken,         // handle to the access token
                TokenUser,    // get information about the token's groups
                (LPVOID)ptu,   // pointer to PTOKEN_USER buffer
                dwLength,       // size of buffer
                &dwLength       // receives required buffer size
            )) {
            goto Cleanup;
        }

        SID_NAME_USE SidType;
        WCHAR lpName[MAX_NAME];
        WCHAR lpDomain[MAX_NAME];

        if (!LookupAccountSid(NULL, ptu->User.Sid, lpName, &dwSize, lpDomain, &dwSize, &SidType)) {
            DWORD dwResult = GetLastError();

            if (dwResult == ERROR_NONE_MAPPED) {
                lstrcpy(lpName, L"NONE_MAPPED");
            } else {
                printf("LookupAccountSid Error %u\n", GetLastError());
            }
        } else {
            printf("Current user is  %s\\%s\n",
                   lpDomain, lpName);
            strUser = lpName;
            strdomain = lpDomain;
            bSuccess = TRUE;
        }

Cleanup:

        if (ptu != NULL)
            HeapFree(GetProcessHeap(), 0, (LPVOID)ptu);

        return bSuccess;
    }

    HRESULT GetUserFromProcess(const DWORD procId, _bstr_t& strUser, _bstr_t& strdomain)
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, procId);

        if (hProcess == NULL)
            return E_FAIL;

        HANDLE hToken = NULL;

        if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
            CloseHandle(hProcess);
            return E_FAIL;
        }

        BOOL bres = GetLogonFromToken(hToken, strUser, strdomain);
        CloseHandle(hToken);
        CloseHandle(hProcess);
        return bres ? S_OK : E_FAIL;
    }

    HANDLE hToken;

    void CurrentUserToken()
    {
        auto procId = GetCurrentProcessId();
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, procId);

        if (hProcess == NULL)
            hToken = 0;

        if (!OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken)) {
            CloseHandle(hProcess);
            hToken = 0;
        }
    }


private:
    std::wstring processPath_;
    std::wstring arguments_;
};

#endif //_PROCESS_STARTER_H_