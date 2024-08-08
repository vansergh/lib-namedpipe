#include <namedpipe/namedpipe/pipe.hpp>
#include <namedpipe/core/error.hpp>

namespace vsock {

    //////////////////////////////////////////////////////////////////////////////////
    // NamedPipe class defenition
    ////////////////////////////////////////////////////////////////////////////////    

    NamedPipe::NamedPipe(const std::string& name, bool is_server) :
        is_server_{ is_server },
        is_client_of_server_{ false },
        is_opened_{ false },
        path_{ std::string(VSOCK_NAMED_PIPE_PATH).append(name) },
        pipe_id_{ NULL }
    {
        #ifndef _WIN32
        memset(&descriptor_, 0, sizeof(SockAddrUn));
        #endif
    }

    NamedPipe::NamedPipe(PipeID pipe_id) :
        is_server_{ false },
        is_client_of_server_{ true },
        is_opened_{ true },
        pipe_id_{ pipe_id }
    {
        #ifndef _WIN32
        memset(&descriptor_, 0, sizeof(SockAddrUn));
        #endif
    }

    NamedPipe::~NamedPipe() {
        if (is_opened_) {
            Close();
        }
    }

    void NamedPipe::Listen() {
        if (!is_server_) {
            throw RuntimeError(
                "Method: NamedPipe::Listen()"s,
                "Message: Is not a server pipe"s
            );
        }        
        if (is_opened_) {
            throw RuntimeError(
                "Method: NamedPipe::Listen()"s,
                "Message: Already opened"s
            );
        }        
        #ifdef _WIN32
        pipe_id_ = CreateNamedPipeA(
            static_cast<LPCSTR>(path_.c_str()),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            VSOCK_NAMED_PIPE_BUFFER_SIZE,
            VSOCK_NAMED_PIPE_BUFFER_SIZE,
            0,
            NULL
        );
        #else
        pipe_id_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
        #endif

        if (pipe_id_ == VSOCK_INVALID_HANDLE_VALUE) {
            throw RuntimeError(
                "Method: NamedPipe::Listen()"s,
                "Message: Creating named pipe failed"s
            );
        }

        #ifndef _WIN32
        unlink(path_.c_str());
        descriptor_.sun_family = AF_UNIX;
        ::strcpy(descriptor_.sun_path, path_.c_str());
        if (::bind(pipe_id_, reinterpret_cast<SockAddrRaw*>(&descriptor_), sizeof(SockAddrUn)) == VSOCK_INVALID_HANDLE_VALUE) {
            throw RuntimeError(
                "Method: NamedPipe::Listen()"s,
                "Message: Connection failed (::bind)"s
            );
        }
        if (::listen(pipe_id_, SOMAXCONN) == VSOCK_INVALID_HANDLE_VALUE) {
            throw RuntimeError(
                "Method: NamedPipe::Listen()"s,
                "Message: Connection failed (::listen)"s
            );
        }
        #endif
        is_opened_ = true;
    }

    NamedPipe* NamedPipe::Accept() {
        if (!is_server_) {
            throw RuntimeError(
                "Method: NamedPipe::Accept()"s,
                "Message: Is not a server pipe"s
            );
        }
        if (!is_opened_) {
            throw RuntimeError(
                "Method: NamedPipe::Accept()"s,
                "Message: Pipe is not opened"s
            );
        }
        #ifdef _WIN32
        DWORD error;
        if (ConnectNamedPipe(pipe_id_, NULL) || (error = GetLastError()) == ERROR_PIPE_CONNECTED) {
            PipeID client = pipe_id_;
            is_opened_ = false;
            Listen();
            return new NamedPipe(client);
        }
        else {
            throw RuntimeError(
                "Method: NamedPipe::Accept()"s,
                "Message: Accept() failed"s
            );
        }
        #else
        int client = ::accept(pipe_id_, NULL, NULL);
        if (client != VSOCK_INVALID_HANDLE_VALUE) {
            return new NamedPipe(client);
        }
        else {
            throw RuntimeError(
                "Method: NamedPipe::Accept()"s,
                "Message: Accept() failed"s
            );
        }
        #endif
        is_opened_ = true;
    }

    NamedPipe* NamedPipe::Accept(std::uint32_t timeout) {
        if (!is_server_) {
            throw RuntimeError(
                "Method: NamedPipe::Accept()"s,
                "Message: Is not a server pipe"s
            );
        }
        if (!is_opened_) {
            throw RuntimeError(
                "Method: NamedPipe::Accept()"s,
                "Message: Pipe is not opened"s
            );
        }        
        #ifdef _WIN32
        OVERLAPPED overlaped;
        overlaped.Internal = 0;
        overlaped.hEvent = CreateEventA(0, 1, 1, 0);
        if (ConnectNamedPipe(pipe_id_, &overlaped) == 0) {
            if (GetLastError() == ERROR_PIPE_CONNECTED) {
                if (!SetEvent(overlaped.hEvent)) {
                    throw RuntimeError(
                        "Method: NamedPipe::Accept()"s,
                        "Message: SetEvent failed"s
                    );
                }
            }
            int result = WaitForSingleObject(overlaped.hEvent, timeout);
            if (WAIT_OBJECT_0 == result) {
                HANDLE client = pipe_id_;
                Listen();
                return new NamedPipe(client);
            }
            else {
                return NULL;
            }
        }
        else {
            throw RuntimeError(
                "Method: NamedPipe::Accept()"s,
                "Message: AsyncWaitForConnection failed"s
            );
        }
        #else
        PipeID nsock;
        int retour;
        fd_set readf;
        fd_set writef;
        struct timeval to;

        FD_ZERO(&readf);
        FD_ZERO(&writef);
        FD_SET(pipe_id_, &readf);
        FD_SET(pipe_id_, &writef);
        to.tv_usec = timeout * 1000;

        retour = select(pipe_id_ + 1, &readf, &writef, 0, &to);

        if (retour == 0) {
            return NULL;
        }

        if ((FD_ISSET(pipe_id_, &readf)) || (FD_ISSET(pipe_id_, &writef))) {
            nsock = accept(pipe_id_, NULL, NULL);
            return new NamedPipe(nsock);
        }
        else {
            throw RuntimeError(
                "Method: NamedPipe::Accept()"s,
                "Message: invalid socket descriptor"s
            );
        }
        #endif
        is_opened_ = true;
    }


    void NamedPipe::Connect() {
        if (is_server_) {
            throw RuntimeError(
                "Method: NamedPipe::Connect()"s,
                "Message: Is not a client pipe"s
            );
        }
        if (is_opened_) {
            throw RuntimeError(
                "Method: NamedPipe::Connect()"s,
                "Message: Already opened"s
            );
        }
        #ifdef _WIN32
        while (true) {
            WaitNamedPipeA(static_cast<LPCSTR>(path_.c_str()), NMPWAIT_WAIT_FOREVER);
            pipe_id_ = CreateFileA(
                static_cast<LPCSTR>(path_.c_str()),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
            );
            if (pipe_id_ != VSOCK_INVALID_HANDLE_VALUE || GetLastError() != ERROR_PIPE_BUSY) {
                break;
            }
        }
        #else
        pipe_id_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
        #endif

        if (pipe_id_ == VSOCK_INVALID_HANDLE_VALUE) {
            throw RuntimeError(
                "Method: NamedPipe::Connect()"s,
                "Message: Could not open pipe"s
            );
        }

        #ifdef _WIN32
        DWORD dw_mode = PIPE_TYPE_BYTE;
        BOOL success = SetNamedPipeHandleState(
            pipe_id_,
            &dw_mode,
            NULL,
            NULL
        );
        if (!success) {
            throw RuntimeError(
                "Method: NamedPipe::Connect()"s,
                "Message: SetNamedPipeHandleState() failed"s
            );
        }
        #else
        descriptor_.sun_family = AF_UNIX;
        ::strcpy(descriptor_.sun_path, path_.c_str());
        if (::connect(pipe_id_, reinterpret_cast<SockAddrRaw*>(&descriptor_), sizeof(SockAddrUn)) == VSOCK_INVALID_HANDLE_VALUE) {
            throw RuntimeError(
                "Method: NamedPipe::Connect()"s,
                "Message: Could not connect to pipe"s
            );
        }
        #endif
        is_opened_ = true;
    }

    void NamedPipe::Close() {
        if (!is_opened_) {
            throw RuntimeError(
                "Method: NamedPipe::Open()"s,
                "Message: Not opened"s
            );
        }
        #ifdef _WIN32
        if (is_server_ || is_client_of_server_) {
            DisconnectNamedPipe(pipe_id_);
            CloseHandle(pipe_id_);
            pipe_id_ = NULL;
        }
        #else
        if (is_server_) {
            unlink(path_.c_str());
        }
        ::close(pipe_id_);
        #endif
        is_opened_ = false;
    }

}