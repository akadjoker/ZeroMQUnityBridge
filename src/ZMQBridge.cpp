#include "ZMQBridge.h"
#include <zmq.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstring>

// Contexto global ZeroMQ
static std::unique_ptr<zmq::context_t> g_context = nullptr;

// Armazena os sockets criados
static std::unordered_map<int, std::unique_ptr<zmq::socket_t>> g_sockets;

// Próximo ID de socket disponível
static int g_next_socket_id = 1;

// Mutex para operações thread-safe
static std::mutex g_mutex;

// Último erro
static std::string g_last_error;

// Define o último erro
static void set_last_error(const std::string& error)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_last_error = error;
}

 
EXPORT_API int zmq_bridge_init()
{
    try
    {
        std::lock_guard<std::mutex> lock(g_mutex);

   
        if (g_context)
        {
            return ZMQ_BRIDGE_OK;
        }

     
        g_context = std::make_unique<zmq::context_t>(1);
        g_next_socket_id = 1;
        g_sockets.clear();

        return ZMQ_BRIDGE_OK;
    } catch (const zmq::error_t& e)
    {
        set_last_error("ZMQ initialization error: " + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_INIT;
    } catch (const std::exception& e)
    {
        set_last_error("General error during initialization: "
                       + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_INIT;
    }
}

 
EXPORT_API void zmq_bridge_shutdown()
{
    std::lock_guard<std::mutex> lock(g_mutex);

 
    g_sockets.clear();

  
    g_context.reset();
}

 
EXPORT_API int zmq_bridge_create_publisher(const char* endpoint)
{
    if (!g_context)
    {
        set_last_error("ZeroMQ context not initialized");
        return ZMQ_BRIDGE_ERROR_INIT;
    }

    try
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        auto socket =
            std::make_unique<zmq::socket_t>(*g_context, zmq::socket_type::pub);

        // Configura o socket
        int linger = 0;
        socket->set(zmq::sockopt::linger, linger);

        // Vincula ao endpoint
        socket->bind(endpoint);

        // Atribui um ID e armazena o socket
        int socket_id = g_next_socket_id++;
        g_sockets[socket_id] = std::move(socket);

        return socket_id;
    } catch (const zmq::error_t& e)
    {
        set_last_error("Failed to create publisher socket: "
                       + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_SOCKET;
    }
}

 
EXPORT_API int zmq_bridge_create_subscriber(const char* endpoint,
                                            const char* topic)
{
    if (!g_context)
    {
        set_last_error("ZeroMQ context not initialized");
        return ZMQ_BRIDGE_ERROR_INIT;
    }

    try
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        auto socket =
            std::make_unique<zmq::socket_t>(*g_context, zmq::socket_type::sub);

        // Configura o socket
        int linger = 0;
        socket->set(zmq::sockopt::linger, linger);

        // Define o tópico de inscrição
        socket->set(zmq::sockopt::subscribe, topic);

        // Conecta ao endpoint
        socket->connect(endpoint);

        // Atribui um ID e armazena o socket
        int socket_id = g_next_socket_id++;
        g_sockets[socket_id] = std::move(socket);

        return socket_id;
    } catch (const zmq::error_t& e)
    {
        set_last_error("Failed to create subscriber socket: "
                       + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_SOCKET;
    }
}

 
EXPORT_API int zmq_bridge_create_request(const char* endpoint)
{
    if (!g_context)
    {
        set_last_error("ZeroMQ context not initialized");
        return ZMQ_BRIDGE_ERROR_INIT;
    }

    try
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        auto socket =
            std::make_unique<zmq::socket_t>(*g_context, zmq::socket_type::req);

        // Configura o socket
        int linger = 0;
        socket->set(zmq::sockopt::linger, linger);

        // Conecta ao endpoint
        socket->connect(endpoint);

        // Atribui um ID e armazena o socket
        int socket_id = g_next_socket_id++;
        g_sockets[socket_id] = std::move(socket);

        return socket_id;
    } catch (const zmq::error_t& e)
    {
        set_last_error("Failed to create request socket: "
                       + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_SOCKET;
    }
}

 
EXPORT_API int zmq_bridge_create_reply(const char* endpoint)
{
    if (!g_context)
    {
        set_last_error("ZeroMQ context not initialized");
        return ZMQ_BRIDGE_ERROR_INIT;
    }

    try
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        auto socket =
            std::make_unique<zmq::socket_t>(*g_context, zmq::socket_type::rep);

        // Configura o socket
        int linger = 0;
        socket->set(zmq::sockopt::linger, linger);

        // Vincula ao endpoint
        socket->bind(endpoint);

        // Atribui um ID e armazena o socket
        int socket_id = g_next_socket_id++;
        g_sockets[socket_id] = std::move(socket);

        return socket_id;
    } catch (const zmq::error_t& e)
    {
        set_last_error("Failed to create reply socket: "
                       + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_SOCKET;
    }
}

 
EXPORT_API int zmq_bridge_create_push(const char* endpoint)
{
    if (!g_context)
    {
        set_last_error("ZeroMQ context not initialized");
        return ZMQ_BRIDGE_ERROR_INIT;
    }

    try
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        auto socket =
            std::make_unique<zmq::socket_t>(*g_context, zmq::socket_type::push);

        // Configura o socket
        int linger = 0;
        socket->set(zmq::sockopt::linger, linger);

        // Conecta ao endpoint
        socket->connect(endpoint);

        // Atribui um ID e armazena o socket
        int socket_id = g_next_socket_id++;
        g_sockets[socket_id] = std::move(socket);

        return socket_id;
    } catch (const zmq::error_t& e)
    {
        set_last_error("Failed to create push socket: "
                       + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_SOCKET;
    }
}

 
EXPORT_API int zmq_bridge_create_pull(const char* endpoint)
{
    if (!g_context)
    {
        set_last_error("ZeroMQ context not initialized");
        return ZMQ_BRIDGE_ERROR_INIT;
    }

    try
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        auto socket =
            std::make_unique<zmq::socket_t>(*g_context, zmq::socket_type::pull);

        // Configura o socket
        int linger = 0;
        socket->set(zmq::sockopt::linger, linger);

        // Vincula ao endpoint
        socket->bind(endpoint);

        // Atribui um ID e armazena o socket
        int socket_id = g_next_socket_id++;
        g_sockets[socket_id] = std::move(socket);

        return socket_id;
    } catch (const zmq::error_t& e)
    {
        set_last_error("Failed to create pull socket: "
                       + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_SOCKET;
    }
}

 
EXPORT_API int zmq_bridge_send(int socket_id, const void* data, int size)
{
    if (!g_context)
    {
        set_last_error("ZeroMQ context not initialized");
        return ZMQ_BRIDGE_ERROR_INIT;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    auto it = g_sockets.find(socket_id);
    if (it == g_sockets.end())
    {
        set_last_error("Invalid socket ID");
        return ZMQ_BRIDGE_ERROR_INVALID_SOCKET;
    }

    try
    {
        zmq::message_t message(data, size);
        auto result = it->second->send(message, zmq::send_flags::none);

        if (!result.has_value())
        {
            set_last_error("Failed to send message");
            return ZMQ_BRIDGE_ERROR_SEND;
        }

        return ZMQ_BRIDGE_OK;
    } catch (const zmq::error_t& e)
    {
        set_last_error("Send error: " + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_SEND;
    }
}

 
EXPORT_API int zmq_bridge_send_string(int socket_id, const char* message)
{
    return zmq_bridge_send(socket_id, message,
                           static_cast<int>(strlen(message)));
}

 
EXPORT_API int zmq_bridge_publish(int socket_id, const char* topic,
                                  const void* data, int size)
{
    if (!g_context)
    {
        set_last_error("ZeroMQ context not initialized");
        return ZMQ_BRIDGE_ERROR_INIT;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    auto it = g_sockets.find(socket_id);
    if (it == g_sockets.end())
    {
        set_last_error("Invalid socket ID");
        return ZMQ_BRIDGE_ERROR_INVALID_SOCKET;
    }

    try
    {
        // Envia o tópico
        zmq::message_t topic_msg(topic, strlen(topic));
        auto topic_result =
            it->second->send(topic_msg, zmq::send_flags::sndmore);

        if (!topic_result.has_value())
        {
            set_last_error("Failed to send topic");
            return ZMQ_BRIDGE_ERROR_SEND;
        }

        // Envia os dados
        zmq::message_t data_msg(data, size);
        auto data_result = it->second->send(data_msg, zmq::send_flags::none);

        if (!data_result.has_value())
        {
            set_last_error("Failed to send data");
            return ZMQ_BRIDGE_ERROR_SEND;
        }

        return ZMQ_BRIDGE_OK;
    } catch (const zmq::error_t& e)
    {
        set_last_error("Publish error: " + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_SEND;
    }
}

 
EXPORT_API int zmq_bridge_receive(int socket_id, void* buffer, int buffer_size,
                                  int* bytes_received)
{
    if (!g_context)
    {
        set_last_error("ZeroMQ context not initialized");
        return ZMQ_BRIDGE_ERROR_INIT;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    auto it = g_sockets.find(socket_id);
    if (it == g_sockets.end())
    {
        set_last_error("Invalid socket ID");
        return ZMQ_BRIDGE_ERROR_INVALID_SOCKET;
    }

    try
    {
        zmq::message_t message;
        auto result = it->second->recv(message, zmq::recv_flags::dontwait);

        if (!result.has_value())
        {
            // Não há mensagem disponível
            *bytes_received = 0;
            return ZMQ_BRIDGE_NO_MESSAGE;
        }

        // Copia os dados para o buffer
        size_t bytes_to_copy =
            std::min(static_cast<size_t>(buffer_size), message.size());
        memcpy(buffer, message.data(), bytes_to_copy);
        *bytes_received = static_cast<int>(bytes_to_copy);

        return ZMQ_BRIDGE_OK;
    } catch (const zmq::error_t& e)
    {
        set_last_error("Receive error: " + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_RECEIVE;
    }
}

 
EXPORT_API int zmq_bridge_receive_string(int socket_id, char* buffer,
                                         int buffer_size)
{
    int bytes_received = 0;
    int result =
        zmq_bridge_receive(socket_id, buffer, buffer_size - 1, &bytes_received);

    if (result == ZMQ_BRIDGE_OK)
    {
        // Adiciona terminador nulo
        buffer[bytes_received] = '\0';
    }

    return result;
}

 
EXPORT_API int zmq_bridge_check_message(int socket_id)
{
    if (!g_context)
    {
        set_last_error("ZeroMQ context not initialized");
        return ZMQ_BRIDGE_ERROR_INIT;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    auto it = g_sockets.find(socket_id);
    if (it == g_sockets.end())
    {
        set_last_error("Invalid socket ID");
        return ZMQ_BRIDGE_ERROR_INVALID_SOCKET;
    }

    try
    {
        zmq::pollitem_t items[] = { { it->second->handle(), 0, ZMQ_POLLIN,
                                      0 } };

        zmq::poll(&items[0], 1, std::chrono::milliseconds(0));

        return (items[0].revents & ZMQ_POLLIN) ? 1 : 0;
    } catch (const zmq::error_t& e)
    {
        set_last_error("Poll error: " + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_RECEIVE;
    }
}

 
EXPORT_API int zmq_bridge_poll(int socket_id, int timeout_ms)
{
    if (!g_context)
    {
        set_last_error("ZeroMQ context not initialized");
        return ZMQ_BRIDGE_ERROR_INIT;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    auto it = g_sockets.find(socket_id);
    if (it == g_sockets.end())
    {
        set_last_error("Invalid socket ID");
        return ZMQ_BRIDGE_ERROR_INVALID_SOCKET;
    }

    try
    {
        zmq::pollitem_t items[] = { { it->second->handle(), 0, ZMQ_POLLIN,
                                      0 } };

        zmq::poll(&items[0], 1, std::chrono::milliseconds(timeout_ms));

        return (items[0].revents & ZMQ_POLLIN) ? 1 : 0;
    } catch (const zmq::error_t& e)
    {
        set_last_error("Poll error: " + std::string(e.what()));
        return ZMQ_BRIDGE_ERROR_RECEIVE;
    }
}

 
EXPORT_API void zmq_bridge_close_socket(int socket_id)
{
    std::lock_guard<std::mutex> lock(g_mutex);

    auto it = g_sockets.find(socket_id);
    if (it != g_sockets.end())
    {
        g_sockets.erase(it);
    }
}

 
EXPORT_API const char* zmq_bridge_get_last_error()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_last_error.c_str();
}
