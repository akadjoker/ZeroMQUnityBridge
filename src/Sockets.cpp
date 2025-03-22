#include <zmq.hpp>
#include "ZMQBridge.h"
#include "Internal.h"

namespace zmq_bridge {
namespace internal {


    int SocketManager::CreateSocket(zmq::socket_type type,
                                    const std::string& endpoint,
                                    bool bind_socket)
    {
        auto context = Context::Instance().GetContext();
        if (!context)
        {
            m_error = "Context not initialized";
            return -1;
        }

        try
        {
            auto socket = std::make_unique<zmq::socket_t>(*context, type);

            // Configura o socket para não bloquear quando fechado
            int linger = 0;
            socket->set(zmq::sockopt::linger, linger);


            if (bind_socket)
            {
                socket->bind(endpoint);
            }
            else
            {
                socket->connect(endpoint);
            }

            // Atribui um ID e armazena o socket
            int socket_id = m_next_socket_id++;

            std::lock_guard<std::mutex> lock(m_mutex);
            m_sockets[socket_id] = std::move(socket);

            return socket_id;
        } catch (const zmq::error_t& e)
        {
            m_error = "Failed to create socket: " + std::string(e.what());
            return -1;
        }
    }


    int SocketManager::CreateSubscriber(const std::string& endpoint,
                                        const std::string& topic)
    {
        auto context = Context::Instance().GetContext();
        if (!context)
        {
            m_error = "Context not initialized";
            return -1;
        }

        try
        {
            auto socket = std::make_unique<zmq::socket_t>(
                *context, zmq::socket_type::sub);

            // Configura o socket para não bloquear quando fechado
            int linger = 0;
            socket->set(zmq::sockopt::linger, linger);

            // Define o tópico de inscrição
            socket->set(zmq::sockopt::subscribe, topic);

            // Conecta ao endpoint
            socket->connect(endpoint);

            // Atribui um ID e armazena o socket
            int socket_id = m_next_socket_id++;

            std::lock_guard<std::mutex> lock(m_mutex);
            m_sockets[socket_id] = std::move(socket);

            return socket_id;
        } catch (const zmq::error_t& e)
        {
            m_error =
                "Failed to create subscriber socket: " + std::string(e.what());
            return -1;
        }
    }


    zmq::socket_t* SocketManager::GetSocket(int socket_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_sockets.find(socket_id);
        if (it == m_sockets.end())
        {
            m_error = "Invalid socket ID";
            return nullptr;
        }

        return it->second.get();
    }


    bool SocketManager::CloseSocket(int socket_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_sockets.find(socket_id);
        if (it == m_sockets.end())
        {
            m_error = "Invalid socket ID";
            return false;
        }

        m_sockets.erase(it);
        return true;
    }


    void SocketManager::CloseAllSockets()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sockets.clear();
    }


    const std::string& SocketManager::GetLastError() const { return m_error; }

} // namespace internal
} // namespace zmq_bridge
