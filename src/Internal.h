// Internal.h - Definições internas da biblioteca
#pragma once

#include <zmq.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace zmq_bridge {
namespace internal {


class SocketManager {
public:
    int CreateSocket(zmq::socket_type type, const std::string& endpoint, bool bind_socket);
    
    int CreateSubscriber(const std::string& endpoint, const std::string& topic);
    
    zmq::socket_t* GetSocket(int socket_id);
    
    bool CloseSocket(int socket_id);
    
    void CloseAllSockets();
    
    const std::string& GetLastError() const;
    
private:
    std::unordered_map<int, std::unique_ptr<zmq::socket_t>> m_sockets;
    
    int m_next_socket_id = 1;
    
    std::mutex m_mutex;
    
    std::string m_error;
};

 
class Context {
public:
    bool Initialize();
    
    void Shutdown();
    
    zmq::context_t* GetContext();
    
    SocketManager& GetSocketManager();
    
    const std::string& GetLastError() const;
    
    static Context& Instance();
    
private:
    Context() = default;
    
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    
    static Context _instance;
    
    std::unique_ptr<zmq::context_t> m_context;
    
    SocketManager m_socket_manager;
    
    bool m_initialized = false;
    
    std::string m_error;
};

} // namespace internal
} // namespace zmq_bridge