#ifndef INCLUDE_GUARD_VSOCK_NAMEDPIPE_HPP
#define INCLUDE_GUARD_VSOCK_NAMEDPIPE_HPP

#include <namedpipe/core/common.hpp>

namespace vsock {

    //////////////////////////////////////////////////////////////////////////////////
    // NamedPipe class declaration
    ////////////////////////////////////////////////////////////////////////////////

    class NamedPipe {
    public:

        NamedPipe() = delete;
        NamedPipe(const NamedPipe&) = delete;
        NamedPipe(NamedPipe&&) = delete;
        NamedPipe& operator=(NamedPipe&&) = delete;
        NamedPipe& operator=(const NamedPipe&) = delete;

    public:

        NamedPipe(const std::string& name, bool is_server);
        ~NamedPipe();

        void Listen();
        NamedPipe* Accept();
        NamedPipe* Accept(std::uint32_t timeout); // miliseconds
        void Connect();
        void Close();

    private:

        NamedPipe(PipeID pipe_id);

        bool is_server_;
        bool is_client_of_server_;
        bool is_opened_;
        std::string path_;
        PipeID pipe_id_;
        #ifndef _WIN32
        SockAddrUn descriptor_;
        #endif
    };

}

#endif // INCLUDE_GUARD_VSOCK_NAMEDPIPE_HPP