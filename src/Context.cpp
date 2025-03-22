

#include <zmq.hpp>
#include "ZMQBridge.h"
#include "Internal.h"

namespace zmq_bridge 
{
namespace internal 
{

    bool Context::Initialize()
    {
        if (m_initialized)
        {
            return true;
        }

        try
        {
            m_context = std::make_unique<zmq::context_t>(1);
            m_initialized = true;
            return true;
        } catch (const zmq::error_t& e)
        {
            m_error = "Failed to initialize ZeroMQ context: " + std::string(e.what());
            return false;
        }
    }


    void Context::Shutdown()
    {
        m_socket_manager.CloseAllSockets();
        m_context.reset();
        m_initialized = false;
    }

 
    zmq::context_t* Context::GetContext()
    {
        if (!m_initialized)
        {
            m_error = "Context not initialized";
            return nullptr;
        }

        return m_context.get();
    }


    SocketManager& Context::GetSocketManager() { return m_socket_manager; }


    const std::string& Context::GetLastError() const { return m_error; }


    Context& Context::Instance()
    {
        static Context instance;
        return instance;
    }

} // namespace internal
} // namespace zmq_bridge
