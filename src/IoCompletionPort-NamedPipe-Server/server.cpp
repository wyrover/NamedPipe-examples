#include <process.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#define ANFO1                  _T("$Anfo1")
#define ANFO2                  _T("$Anfo2")
#define ANSW1                  _T("$Antw1")
#define ANSW2                  _T("$Antw2")
#define BUFSIZE                20
#define BYTES_TO_SEND          (BUFSIZE * sizeof(TCHAR))
#define KEY_CONNECT_NAMED_PIPE  0
#define KEY_READ_FILE           1
#define NUMBER_OF_PIPES        16
#define NUMBER_OF_THREADS       4
#define SUCCESSFUL              0

typedef struct {
    HANDLE     hNp;
    TCHAR      Buf[BUFSIZE];
    DWORD      Type; // 0 f¨¹r ConnectNamedPipe, 1 f¨¹r ReadFile
    OVERLAPPED Ov;
} KEY;

KEY    gaKey[NUMBER_OF_PIPES];

unsigned _stdcall Worker(void*);

int _tmain(int iArgc, LPTSTR lptsArgv[])
{
    BOOL   bReturn   = FALSE;
    DWORD  dwReturn  = 0;
    HANDLE hCompPort = NULL;
    HANDLE ahEvents[NUMBER_OF_PIPES];
    HANDLE ahThread[NUMBER_OF_THREADS];
    size_t auiThreadID[NUMBER_OF_THREADS];
    printf("NamedPipeN: CreateIoCompletionPort.\n");
    // Create the compretion port.
    hCompPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    // check for bad return values
    if (hCompPort == INVALID_HANDLE_VALUE) {
        printf("NamedPipeN: CreateIoCompletionPort -> Errorcode: %i.\n", GetLastError());
        return -1;
    }

    for (size_t i = 0; i < NUMBER_OF_PIPES; i++) {
        printf("NamedPipe%i: CreateNamedPipe.\n", i);
        // Create the named pipe.
        gaKey[i].hNp = CreateNamedPipe(_T("\\\\.\\pipe\\pipe"),   // pipe name
                                       PIPE_ACCESS_DUPLEX |      // read/write access
                                       FILE_FLAG_OVERLAPPED,     // Ovelapped mode
                                       PIPE_TYPE_MESSAGE |       // message type pipe
                                       PIPE_READMODE_MESSAGE |   // message-read mode
                                       PIPE_WAIT,                // blocking mode
                                       NUMBER_OF_PIPES,          // defined number of instances
                                       0,                        // output buffer size
                                       0,                        // input buffer size
                                       INFINITE,                 // client time-out
                                       NULL);                    // default security attribute

        // check for bad return values
        if (gaKey[i].hNp == INVALID_HANDLE_VALUE) {
            printf("NamedPipe%i: CreateNamedPipe -> Errorcode: %i.\n", i, GetLastError());
            continue;
        }

        printf("NamedPipe%i: CreateEvent.\n", i);
        // Create an event.
        ahEvents[i] = CreateEvent(NULL,  // Ignored. Must be NULL.
                                  TRUE,  // manual-reset.
                                  FALSE, // initial state is nonsignaled.
                                  NULL); // no name.

        if (ahEvents[i] == INVALID_HANDLE_VALUE) {
            printf("NamedPipe%i: CreateEvent.\n -> Errorcode: %i.\n", i, GetLastError());
            continue;
        }

        // initialice key.
        OVERLAPPED ovTemp = {0, 0, 0, 0, ahEvents[i]};
        gaKey[i].Ov       = ovTemp;
        gaKey[i].Type     = KEY_CONNECT_NAMED_PIPE;
        printf("NamedPipe%i: CreateIoCompletionPort.\n", i);

        // Add to compretion port.
        if (CreateIoCompletionPort(gaKey[i].hNp, hCompPort, i, 0) == INVALID_HANDLE_VALUE) {
            printf("NamedPipe%i: CreateIoCompletionPort -> Errorcode: %i.\n", i, GetLastError());
            continue;
        }

        printf("NamedPipe%i: ConnectNamedPipe.\n", i);
        // Connect the named pipe.
        bReturn = ConnectNamedPipe(gaKey[i].hNp, &gaKey[i].Ov);

        if ((GetLastError() != ERROR_IO_PENDING) && (bReturn == ERROR)) {
            printf("NamedPipe%i: ConnectNamedPipe -> Errorcode: %i.\n", i, GetLastError());
            continue;
        }
    } // end for(int i=0; i<NUMBER_OF_PIPES; i++)

    for (size_t i = 0; i < NUMBER_OF_THREADS; i++) {
        printf("NamedPipeN: _beginthreadex.\n", i);
        // create the threads
        ahThread[i] = (HANDLE)_beginthreadex(NULL,           // no security attribute
                                             BUFSIZE,        // stack size
                                             Worker,         // the thread function
                                             hCompPort,      // parameter for the thread
                                             0,              // run immediatly
                                             &auiThreadID[i]); // get the thread ID
    } // end for(size_t i=0; i<NUMBER_OF_THREADS; i++)

    printf("NamedPipeN: WaitForMultipleObjects.\n");
    // Wait for the return of the all threads.
    dwReturn = WaitForMultipleObjects(NUMBER_OF_THREADS, // number of objects to wait for
                                      ahThread,          // array of the objects
                                      TRUE,              // wait for all objects
                                      INFINITE);         // wait till all threads are ready

    // check for bad return value
    if (dwReturn == WAIT_FAILED) {
        printf("NamedPipeN: WaitForMultipleObjects -> Errorcode: %i.\n", GetLastError());

        for (int i = 0; i <= NUMBER_OF_PIPES; i++) {
            CloseHandle(gaKey[i].hNp);
        }

        return -5;
    }

    for (size_t i = 0; i < NUMBER_OF_PIPES; i++) {
        printf("NamedPipe%i: CloseHandle(hThread).\n", i);
        // Close the handles of the threads.
        bReturn = CloseHandle(ahThread[i]);

        // Check for bad return values.
        if (bReturn == ERROR) {
            printf("NamedPipe%i: CloseHandle(hThread) -> Errorcode: %i.\n", i, GetLastError());
            return -6;
        }

        printf("NamedPipe%i: CloseHandle(hPipe).\n", i);
        // Close the pipes.
        bReturn = CloseHandle(gaKey[i].hNp);

        // Check for bad return values.
        if (bReturn == ERROR) {
            printf("NamedPipe%i: CloseHandle(hPipe) -> Errorcode: %i.\n", i, GetLastError());
            return -7;
        }
    } // end for

    return 0;
}

unsigned _stdcall Worker(void* lpvCompPort)
{
    BOOL          bReturn      = FALSE;
    unsigned long ulKeyIndex   = 0;
    DWORD         dwTransfered = 0;
    HANDLE        hCompPort    = (HANDLE) lpvCompPort;
    LPOVERLAPPED  pov          = NULL;
    TCHAR   tcBuffer[BYTES_TO_SEND];
    printf("Thread%i: Running.\n", GetCurrentThreadId());

    while (true) {
        printf("Thread%i: GetQueuedCompletionStatus.\n", GetCurrentThreadId());
        // Dequeue an I/O completion packet.
        bReturn = GetQueuedCompletionStatus(hCompPort,     // Handle to the completion port of interest.
                                            &dwTransfered, // Receives the number of bytes transferred.
                                            &ulKeyIndex,   // Receives the completion key value.
                                            &pov,          // Receives the address of the OVERLAPPED structure.
                                            INFINITE);     // Number of milliseconds that the caller is willing to wait.

        if ((bReturn == ERROR) || (pov == NULL)) {
            printf("Thread%i: GetQueuedCompletionStatus -> Errorcode: %i.\n", GetCurrentThreadId(), GetLastError());
            continue;
        }

        switch (gaKey[ulKeyIndex].Type) {
        case KEY_CONNECT_NAMED_PIPE : {
            gaKey[ulKeyIndex].Type = KEY_READ_FILE;
            printf("Thread%i: ReadFile.\n", GetCurrentThreadId());
            // Read from the stream
            bReturn = ReadFile(gaKey[ulKeyIndex].hNp,  // source
                               gaKey[ulKeyIndex].Buf,  // destination buffer
                               BYTES_TO_SEND,          // number of bytes to read
                               &dwTransfered,          // number of bytes readed
                               &gaKey[ulKeyIndex].Ov); // overlapped structure

            // check for bad return value
            if ((GetLastError() != ERROR_IO_PENDING) && !bReturn) {
                printf("Thread%i: ReadFile -> Errorcode: %i.\n", GetCurrentThreadId(), GetLastError());
                // restart the thread
                printf("Thread%i: restart this thread.\n", GetCurrentThreadId());
                // reset type
                gaKey[ulKeyIndex].Type = KEY_CONNECT_NAMED_PIPE;
                printf("Thread%i: DisconnectNamedPipe.\n", GetCurrentThreadId());

                // disconnect
                if (!DisconnectNamedPipe(gaKey[ulKeyIndex].hNp)) {
                    printf("Thread%i: DisconnectNamedPipe -> Errorcode: %i.\n", GetCurrentThreadId(), GetLastError());
                }

                printf("Thread%i: ConnectNamedPipe.\n", GetCurrentThreadId());
                // reconnect
                ConnectNamedPipe(gaKey[ulKeyIndex].hNp, &gaKey[ulKeyIndex].Ov);
                {
                    if (GetLastError() != ERROR_IO_PENDING) {
                        printf("Thread%i: ConnectNamedPipe -> Errorcode: %i.\n", GetCurrentThreadId(), GetLastError());
                        continue;
                    }
                }
                break;
            }
        } // end case KEY_CONNECT_NAMED_PIPE

        case KEY_READ_FILE : {
            printf("Thread%i: Compute message.\n", GetCurrentThreadId());

            // look whats been sendet to us
            if (gaKey[ulKeyIndex].Buf[5] == _T('1')) {
                OVERLAPPED ov = gaKey[ulKeyIndex].Ov;
                ov.hEvent = (HANDLE)((DWORD)ov.hEvent | 1);
                printf("Thread%i: WriteFile(ANSW1).\n", GetCurrentThreadId());
                // send the answer
                WriteFile(gaKey[ulKeyIndex].hNp,  // destination
                          ANSW1,                  // signed to write
                          BYTES_TO_SEND,          // number of bytes to write
                          &dwTransfered,          // number of bytes written
                          &ov);                   // overlapped structure
                // wait for the event.
                WaitForSingleObject(ov.hEvent, INFINITE);
            } else if (gaKey[ulKeyIndex].Buf[5] == _T('2')) {
                OVERLAPPED ov = gaKey[ulKeyIndex].Ov;
                ov.hEvent = (HANDLE)((DWORD)ov.hEvent | 1);
                printf("Thread%i: WriteFile(ANSW2).\n", GetCurrentThreadId());
                // send the answer
                WriteFile(gaKey[ulKeyIndex].hNp,  // destination
                          ANSW2,                  // signed to write
                          BYTES_TO_SEND,          // number of bytes to write
                          &dwTransfered,          // number of bytes written
                          &ov);                   // overlapped structure
                // wait for the event.
                WaitForSingleObject(ov.hEvent, INFINITE);
            } else {
                printf("Thread%i: Received only trash.\n", GetCurrentThreadId());
                continue;
            }

            printf("Thread%i: ReadFile.\n", GetCurrentThreadId());
            // Read from the stream
            bReturn = ReadFile(gaKey[ulKeyIndex].hNp,  // source
                               gaKey[ulKeyIndex].Buf,  // destination buffer
                               BYTES_TO_SEND,          // number of bytes to read
                               &dwTransfered,          // number of bytes readed
                               &gaKey[ulKeyIndex].Ov); // overlapped structure

            // check for bad return value
            if ((GetLastError() != ERROR_IO_PENDING) && !bReturn) {
                printf("Thread%i: ReadFile -> Errorcode: %i.\n", GetCurrentThreadId(), GetLastError());
                // restart the thread
                printf("Thread%i: restart this thread.\n", GetCurrentThreadId());
                // reset type
                gaKey[ulKeyIndex].Type = KEY_CONNECT_NAMED_PIPE;
                printf("Thread%i: DisconnectNamedPipe.\n", GetCurrentThreadId());

                // disconnect
                if (!DisconnectNamedPipe(gaKey[ulKeyIndex].hNp)) {
                    printf("Thread%i: DisconnectNamedPipe -> Errorcode: %i.\n", GetCurrentThreadId(), GetLastError());
                }

                printf("Thread%i: ConnectNamedPipe.\n", GetCurrentThreadId());
                // reconnect
                ConnectNamedPipe(gaKey[ulKeyIndex].hNp, &gaKey[ulKeyIndex].Ov);
                {
                    if (GetLastError() != ERROR_IO_PENDING) {
                        printf("Thread%i: ConnectNamedPipe -> Errorcode: %i.\n", GetCurrentThreadId(), GetLastError());
                        continue;
                    }
                }
            }

            // gaKey[ulKeyIndex].Ov.hEvent; ???
        } // end case KEY_READ_FILE
        } // end switch(gaKey[ulKeyIndex].Type)
    } // end while(true)

    return 0;
}