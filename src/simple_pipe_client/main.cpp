#include <windows.h>
#include <stdio.h>

#define PIPE_NAME L"\\\\.\\Pipe\\Jim"

void main(void)
{
    HANDLE PipeHandle;
    DWORD BytesWritten;

    if (WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER) == 0) {
        printf("WaitNamedPipe failed with error %d\n",
               GetLastError());
        return;
    }

    // Create the named pipe file handle
    if ((PipeHandle = CreateFile(PIPE_NAME,
                                 GENERIC_READ | GENERIC_WRITE, 0,
                                 (LPSECURITY_ATTRIBUTES) NULL, OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 (HANDLE) NULL)) == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed with error %d\n", GetLastError());
        return;
    }

    if (WriteFile(PipeHandle, "This is a test", 14, &BytesWritten,
                  NULL) == 0) {
        printf("WriteFile failed with error %d\n", GetLastError());
        CloseHandle(PipeHandle);
        return;
    }

    printf("Wrote %d bytes", BytesWritten);
    CloseHandle(PipeHandle);
}