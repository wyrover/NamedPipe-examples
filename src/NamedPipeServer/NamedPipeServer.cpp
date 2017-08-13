// NamedPipeServer.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <tchar.h>
#include <Windows.h>

void
DisplayError(WCHAR* pszAPI)
{
    LPVOID lpvMessageBuffer;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&lpvMessageBuffer, 0, NULL);
    //... now display this string
    wprintf(L"ERROR: API        = %s.\n", pszAPI);
    wprintf(L"       error code = %d.\n", GetLastError());
    wprintf(L"       message    = %s.\n", (char *)lpvMessageBuffer);
    // Free the buffer allocated by the system
    LocalFree(lpvMessageBuffer);
    ExitProcess(GetLastError());
}

class SEC
{
    // Class to handle security attributes
public:
    SEC();
    ~SEC();
    BOOL BuildSecurityAttributes(SECURITY_ATTRIBUTES* psa);

private:
    BOOL GetUserSid(PSID*  ppSidUser);

    BOOL    allocated;
    PSECURITY_DESCRIPTOR    psd;
    PACL    pACL;
    PTOKEN_USER pTokenUser;
};
/**  Constructor */
SEC::
SEC()
{
    allocated = FALSE;
    psd = NULL;
    pACL = NULL;
    pTokenUser = NULL;
}

/** Destructor */
SEC::
~SEC()
{
    if (allocated) {
        if (psd) HeapFree(GetProcessHeap(), 0, psd);

        if (pACL) HeapFree(GetProcessHeap(), 0, pACL);

        if (pTokenUser) HeapFree(GetProcessHeap(), 0, pTokenUser);

        allocated = FALSE;
    }
}

/** Builds security attributes that allows full access to unthenticated users
Input parameters: psa: security attributes to build
Output parameters: TRUE | FALSE */
BOOL SEC::
BuildSecurityAttributes(SECURITY_ATTRIBUTES* psa)
{
    DWORD dwAclSize;
    PSID  pSidAnonymous = NULL; // Well-known AnonymousLogin SID
    PSID  pSidOwner = NULL;

    if (allocated) return FALSE;

    SID_IDENTIFIER_AUTHORITY siaAuthenticatedUser = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY siaOwner = SECURITY_NT_AUTHORITY;

    do {
        psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(),
                                              HEAP_ZERO_MEMORY,
                                              SECURITY_DESCRIPTOR_MIN_LENGTH);

        if (psd == NULL) {
            DisplayError(L"HeapAlloc");
            break;
        }

        if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION)) {
            DisplayError(L"InitializeSecurityDescriptor");
            break;
        }

        // Build anonymous SID
        AllocateAndInitializeSid(&siaAuthenticatedUser, 1,
                                 SECURITY_AUTHENTICATED_USER_RID, // FYI, for everyone user, we can use SECURITY_ANONYMOUS_LOGON_RID.
                                 0, 0, 0, 0, 0, 0, 0,
                                 &pSidAnonymous
                                );

        if (!GetUserSid(&pSidOwner)) {
            return FALSE;
        }

        // Compute size of ACL
        dwAclSize = sizeof(ACL) +
                    2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                    GetLengthSid(pSidAnonymous) +
                    GetLengthSid(pSidOwner);
        pACL = (PACL)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwAclSize);

        if (pACL == NULL) {
            DisplayError(L"HeapAlloc");
            break;
        }

        InitializeAcl(pACL, dwAclSize, ACL_REVISION);

        if (!AddAccessAllowedAce(pACL,
                                 ACL_REVISION,
                                 GENERIC_ALL,
                                 pSidOwner
                                )) {
            DisplayError(L"AddAccessAllowedAce");
            break;
        }

        if (!AddAccessAllowedAce(pACL,
                                 ACL_REVISION,
                                 FILE_GENERIC_READ | FILE_GENERIC_WRITE | FILE_GENERIC_EXECUTE,
                                 pSidAnonymous
                                )) {
            DisplayError(L"AddAccessAllowedAce");
            break;
        }

        if (!SetSecurityDescriptorDacl(psd, TRUE, pACL, FALSE)) {
            DisplayError(L"SetSecurityDescriptorDacl");
            break;
        }

        psa->nLength = sizeof(SECURITY_ATTRIBUTES);
        psa->bInheritHandle = TRUE;
        psa->lpSecurityDescriptor = psd;
        allocated = TRUE;
    } while (0);

    if (pSidAnonymous)   FreeSid(pSidAnonymous);

    //jhkimd this line is not required.
    //if (pSidOwner)       FreeSid(pSidOwner);

    if (!allocated) {
        if (psd) HeapFree(GetProcessHeap(), 0, psd);

        if (pACL) HeapFree(GetProcessHeap(), 0, pACL);
    }

    return allocated;
}

/** Obtains the SID of the user running this thread or process.
Output parameters: ppSidUser: the SID of the current user,
TRUE   | FALSE: could not obtain the user SID */
BOOL SEC::
GetUserSid(PSID*  ppSidUser)
{
    HANDLE      hToken;
    DWORD       dwLength;
    DWORD       cbName = 250;
    DWORD       cbDomainName = 250;

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken)) {
        if (GetLastError() == ERROR_NO_TOKEN) {
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    if (!GetTokenInformation(hToken,       // handle of the access token
                             TokenUser,    // type of information to retrieve
                             pTokenUser,   // address of retrieved information
                             0,            // size of the information buffer
                             &dwLength     // address of required buffer size
                            )) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            pTokenUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);

            if (pTokenUser == NULL) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    if (!GetTokenInformation(hToken,     // handle of the access token
                             TokenUser,  // type of information to retrieve
                             pTokenUser, // address of retrieved information
                             dwLength,   // size of the information buffer
                             &dwLength   // address of required buffer size
                            )) {
        HeapFree(GetProcessHeap(), 0, pTokenUser);
        pTokenUser = NULL;
        return FALSE;
    }

    *ppSidUser = pTokenUser->User.Sid;
    return TRUE;
}

int main(void)
{
    HANDLE hPipe;
    char buffer[1024];
    DWORD dwRead;
    SEC sec;
    SECURITY_ATTRIBUTES     sa;
    sec.BuildSecurityAttributes(&sa);
    hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\Pipe"),
                            PIPE_ACCESS_DUPLEX | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
                            PIPE_WAIT,
                            1,
                            1024 * 16,
                            1024 * 16,
                            NMPWAIT_USE_DEFAULT_WAIT,
                            &sa);

    while (hPipe != INVALID_HANDLE_VALUE) {
        if (ConnectNamedPipe(hPipe, NULL) != FALSE) { // wait for someone to connect to the pipe
            while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE) {
                /* add terminating zero */
                buffer[dwRead] = '\0';
                /* do something with data in buffer */
                printf("%s", buffer);
            }
        }

        DisconnectNamedPipe(hPipe);
    }

    return 0;
}