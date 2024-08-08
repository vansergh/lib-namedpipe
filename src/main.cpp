#include <string>
#include <iostream>
#include <thread>
#include <namedpipe/namedpipe/pipe.hpp>

using namespace std;
using namespace vsock;


int main() {

    cout << "\n=========================================================================\n"s;
    {

        NamedPipe client("hello", false);

        std::thread server_th([]() {
            NamedPipe* pipe_server = new NamedPipe("hello", true);
            cout << "Listening\n";
            pipe_server->Listen();
            NamedPipe* connected_client = pipe_server->Accept();
            cout << "Server: Connected. Sleeping 5 sec and disconnect\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            delete connected_client;
            delete pipe_server;
        });

        std::thread client_th([]() {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            NamedPipe* pipe_client = new NamedPipe("hello", false);
            cout << "Connecting\n";
            pipe_client->Connect();
            cout << "Client: Connected. Sleeping 5 sec and disconnect\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            delete pipe_client;
        });


        client_th.join();
        server_th.join();


        cout << "All Done! waiting 5 sec and terminate" << endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

}