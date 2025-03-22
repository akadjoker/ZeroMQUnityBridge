
#pragma once

#ifdef _WIN32
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API __attribute__((visibility("default")))
#endif


#define ZMQ_BRIDGE_OK 0
#define ZMQ_BRIDGE_ERROR_INIT -1
#define ZMQ_BRIDGE_ERROR_SOCKET -2
#define ZMQ_BRIDGE_ERROR_BIND -3
#define ZMQ_BRIDGE_ERROR_CONNECT -4
#define ZMQ_BRIDGE_ERROR_SEND -5
#define ZMQ_BRIDGE_ERROR_RECEIVE -6
#define ZMQ_BRIDGE_ERROR_INVALID_SOCKET -7
#define ZMQ_BRIDGE_NO_MESSAGE 1

extern "C" {
 
EXPORT_API int zmq_bridge_init();
EXPORT_API void zmq_bridge_shutdown();

 
EXPORT_API int zmq_bridge_create_publisher(const char* endpoint);
EXPORT_API int zmq_bridge_create_subscriber(const char* endpoint,
                                            const char* topic);
EXPORT_API int zmq_bridge_create_request(const char* endpoint);
EXPORT_API int zmq_bridge_create_reply(const char* endpoint);
EXPORT_API int zmq_bridge_create_push(const char* endpoint);
EXPORT_API int zmq_bridge_create_pull(const char* endpoint);

 EXPORT_API int zmq_bridge_send(int socket_id, const void* data, int size);
EXPORT_API int zmq_bridge_send_string(int socket_id, const char* message);
EXPORT_API int zmq_bridge_publish(int socket_id, const char* topic,
                                  const void* data, int size);

 EXPORT_API int zmq_bridge_receive(int socket_id, void* buffer, int buffer_size,
                                  int* bytes_received);
EXPORT_API int zmq_bridge_receive_string(int socket_id, char* buffer,
                                         int buffer_size);
EXPORT_API int zmq_bridge_check_message(int socket_id); 


EXPORT_API int zmq_bridge_poll(int socket_id, int timeout_ms);


EXPORT_API void zmq_bridge_close_socket(int socket_id);


EXPORT_API const char* zmq_bridge_get_last_error();
}
