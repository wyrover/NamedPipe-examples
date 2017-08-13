#include <iostream>
using std::cin;
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#define ANFO1           _T("$Anfo1")
#define ANFO2           _T("$Anfo2")
#define BUFSIZE         20
#define BYTES_TO_SEND   (BUFSIZE * sizeof(TCHAR))
#define SUCCESSFUL      0

int _tmain(int iArgc, LPTSTR lptsArgv[])
{
    BOOL    bReturn                 = 0;
    DWORD   dwMode                  = PIPE_READMODE_MESSAGE;
    DWORD   dwTransfered            = 0;
    HANDLE  hPipe                   = NULL;
    int     iInput                  = 0;
    LPTSTR  lptsMessage             = ANFO1;
    TCHAR   tcBuffer[BYTES_TO_SEND] = {'\0'};
    BOOL    bQuit                   = FALSE;

    while (true) {
        do {
            printf("WaitNamedPipe.\n");

            // Wait for a pipe.
            if (!WaitNamedPipe(_T("\\\\.\\pipe\\pipe"), INFINITE)) {
                printf("WaitNamedPipe -> Errorcode: %i.\n", GetLastError());
                continue;
            }

            printf("CreateFile.\n");
            // Open the pipe
            hPipe = CreateFile(_T("\\\\.\\pipe\\pipe"), // pipe name
                               GENERIC_READ |           // read access
                               GENERIC_WRITE,           // write access
                               0,                       // no sharing
                               NULL,                    // default security attributes
                               OPEN_EXISTING,           // opens existing pipe
                               FILE_ATTRIBUTE_NORMAL,   // default attributes
                               NULL);                   // no template file
        } while (hPipe == INVALID_HANDLE_VALUE);

        printf("SetNamedPipeHandleState.\n");
        // Set to message-read mode.
        bReturn = SetNamedPipeHandleState(hPipe,    // pipe
                                          &dwMode,  // pipe mode to set
                                          NULL,     // no maximum bytes
                                          NULL);    // no maximum time

        // Check for bad return values
        if (bReturn == ERROR) {
            printf("SetNamedPipeHandleState -> Errorcode: %i.\n", GetLastError());
            CloseHandle(hPipe);
            return NULL;
        }

        while (true) {
            printf("\nCoose one option:\n\n0 - ANFO1\n1 - ANFO2\n2 - QUIT\n\n");
            // Get the request.
            cin >> iInput;

            switch (iInput) {
            case 0  :
                lptsMessage = ANFO1;
                bQuit = false;
                break;

            case 1  :
                lptsMessage = ANFO2;
                bQuit = false;
                break;

            default :
                bQuit =  true;
                break;
            }

            // Check for shutdown.
            if (bQuit) {
                printf("\nClose the client.\n");
                CloseHandle(hPipe);
                return 0;
            }

            printf("\nWriteFile.\n");
            // Send a message throw the pipe.
            bReturn = WriteFile(hPipe,         // pipe
                                lptsMessage,   // message
                                BYTES_TO_SEND, // message length
                                &dwTransfered, // bytes written
                                NULL);         // no overlapped structur

            // Check for bad return values.
            if (bReturn == ERROR) {
                printf("WriteFile -> Errorcode: %i.\n", GetLastError());
                CloseHandle(hPipe);
                return -3;
            }

            printf("ReadFile.\n");
            // Listen for an answer.
            bReturn = ReadFile(hPipe,         // pipe
                               tcBuffer,      // destination buffer
                               BYTES_TO_SEND, // buffer length
                               &dwTransfered, // bytes read
                               NULL);         // no overlapped structur

            // Check for bad return values.
            if (bReturn == ERROR) {
                printf("ReadFile -> Errorcode: %i.\n", GetLastError());
                CloseHandle(hPipe);
                return -4;
            }

            printf("_tprintf.\n");
            // Print the message in the screen
            _tprintf(_T("\n-> Sended:   %s\n-> Received: %s\n"), lptsMessage, tcBuffer);
        } // end while true
    }// end while true
}