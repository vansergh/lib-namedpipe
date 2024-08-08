#include <string>
#include <cstddef>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace vsock {
    #ifdef _WIN32

    #define VSOCK_INVALID_HANDLE_VALUE INVALID_HANDLE_VALUE
    #define VSOCK_NAMED_PIPE_PATH "\\\\.\\pipe\\"
    using PipeID = HANDLE;

    #define VSOCK_NAMED_PIPE_BUFFER_SIZE 1024

    #else

    #define VSOCK_INVALID_HANDLE_VALUE -1
    #define VSOCK_NAMED_PIPE_PATH "/tmp/"
    using PipeID = int;

    using SockAddrUn = sockaddr_un;
    using SockAddrRaw = sockaddr;
    #define VSOCK_SOCKET_ERROR -1
    
    #endif

}