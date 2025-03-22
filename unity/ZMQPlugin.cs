using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Text;
using UnityEngine;

public class ZMQPlugin : MonoBehaviour
{
    #region Native Import Declarations
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_init();
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern void zmq_bridge_shutdown();
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_create_publisher(string endpoint);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_create_subscriber(string endpoint, string topic);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_create_request(string endpoint);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_create_reply(string endpoint);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_create_push(string endpoint);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_create_pull(string endpoint);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_send(int socketId, byte[] data, int size);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_send_string(int socketId, string message);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_publish(int socketId, string topic, byte[] data, int size);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_receive(int socketId, byte[] buffer, int bufferSize, ref int bytesReceived);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_receive_string(int socketId, StringBuilder buffer, int bufferSize);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_check_message(int socketId);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern int zmq_bridge_poll(int socketId, int timeoutMs);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern void zmq_bridge_close_socket(int socketId);
    
    [DllImport("ZeroMQUnityBridge", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr zmq_bridge_get_last_error();
    
    #endregion
    
    // Constantes de erro
    private const int ZMQ_BRIDGE_OK = 0;
    private const int ZMQ_BRIDGE_NO_MESSAGE = 1;
    private const int ZMQ_BRIDGE_ERROR_INIT = -1;
    private const int ZMQ_BRIDGE_ERROR_SOCKET = -2;
    private const int ZMQ_BRIDGE_ERROR_INVALID_SOCKET = -7;
    
    // Delegados para eventos
    public delegate void MessageReceivedHandler(string topic, byte[] data);
    public delegate void StringMessageReceivedHandler(string topic, string message);
    
    // Eventos
    public event MessageReceivedHandler OnMessageReceived;
    public event StringMessageReceivedHandler OnStringMessageReceived;
    
    // Sockets ativos
    private Dictionary<string, int> _sockets = new Dictionary<string, int>();
    
    // Buffers de recepção
    private byte[] _receiveBuffer = new byte[1024 * 1024]; // 1MB de buffer por padrão
    private StringBuilder _stringBuffer = new StringBuilder(8192);
    
    // Configurações de polling
    [Header("ZeroMQ Settings")]
    public int pollingIntervalMs = 10;
    public bool autoPolling = true;
    
    // Inicialização do plugin
    void Awake()
    {
        int result = zmq_bridge_init();
        if (result != ZMQ_BRIDGE_OK)
        {
            Debug.LogError($"Failed to initialize ZeroMQ bridge: {result} - {GetLastError()}");
        }
        else
        {
            Debug.Log("ZeroMQ bridge initialized successfully");
        }
    }
    
    // Verifica por mensagens automaticamente se habilitado
    void Update()
    {
        if (autoPolling)
        {
            foreach (var socket in _sockets)
            {
                PollSocket(socket.Key);
            }
        }
    }
    
    // Limpeza ao destruir o objeto
    void OnDestroy()
    {
        foreach (var socket in _sockets)
        {
            zmq_bridge_close_socket(socket.Value);
        }
        _sockets.Clear();
        
        zmq_bridge_shutdown();
        Debug.Log("ZeroMQ bridge shutdown");
    }
    
    // Configura um socket publicador
    public bool SetupPublisher(string name, string endpoint)
    {
        if (_sockets.ContainsKey(name))
        {
            zmq_bridge_close_socket(_sockets[name]);
        }
        
        int socketId = zmq_bridge_create_publisher(endpoint);
        if (socketId < 0)
        {
            Debug.LogError($"Failed to create publisher socket: {GetLastError()}");
            return false;
        }
        
        _sockets[name] = socketId;
        Debug.Log($"Publisher socket '{name}' created at {endpoint}");
        return true;
    }
    
    // Configura um socket assinante
    public bool SetupSubscriber(string name, string endpoint, string topic)
    {
        if (_sockets.ContainsKey(name))
        {
            zmq_bridge_close_socket(_sockets[name]);
        }
        
        int socketId = zmq_bridge_create_subscriber(endpoint, topic);
        if (socketId < 0)
        {
            Debug.LogError($"Failed to create subscriber socket: {GetLastError()}");
            return false;
        }
        
        _sockets[name] = socketId;
        Debug.Log($"Subscriber socket '{name}' created at {endpoint} for topic '{topic}'");
        return true;
    }
    
    // Configura um socket requisitante
    public bool SetupRequestSocket(string name, string endpoint)
    {
        if (_sockets.ContainsKey(name))
        {
            zmq_bridge_close_socket(_sockets[name]);
        }
        
        int socketId = zmq_bridge_create_request(endpoint);
        if (socketId < 0)
        {
            Debug.LogError($"Failed to create request socket: {GetLastError()}");
            return false;
        }
        
        _sockets[name] = socketId;
        Debug.Log($"Request socket '{name}' created at {endpoint}");
        return true;
    }
    
    // Configura um socket respondente
    public bool SetupReplySocket(string name, string endpoint)
    {
        if (_sockets.ContainsKey(name))
        {
            zmq_bridge_close_socket(_sockets[name]);
        }
        
        int socketId = zmq_bridge_create_reply(endpoint);
        if (socketId < 0)
        {
            Debug.LogError($"Failed to create reply socket: {GetLastError()}");
            return false;
        }
        
        _sockets[name] = socketId;
        Debug.Log($"Reply socket '{name}' created at {endpoint}");
        return true;
    }
    
    // Configura um socket push
    public bool SetupPushSocket(string name, string endpoint)
    {
        if (_sockets.ContainsKey(name))
        {
            zmq_bridge_close_socket(_sockets[name]);
        }
        
        int socketId = zmq_bridge_create_push(endpoint);
        if (socketId < 0)
        {
            Debug.LogError($"Failed to create push socket: {GetLastError()}");
            return false;
        }
        
        _sockets[name] = socketId;
        Debug.Log($"Push socket '{name}' created at {endpoint}");
        return true;
    }
    
    // Configura um socket pull
    public bool SetupPullSocket(string name, string endpoint)
    {
        if (_sockets.ContainsKey(name))
        {
            zmq_bridge_close_socket(_sockets[name]);
        }
        
        int socketId = zmq_bridge_create_pull(endpoint);
        if (socketId < 0)
        {
            Debug.LogError($"Failed to create pull socket: {GetLastError()}");
            return false;
        }
        
        _sockets[name] = socketId;
        Debug.Log($"Pull socket '{name}' created at {endpoint}");
        return true;
    }
    
    // Envia dados através de um socket
    public bool SendData(string socketName, byte[] data)
    {
        if (!_sockets.TryGetValue(socketName, out int socketId))
        {
            Debug.LogError($"Socket '{socketName}' not found");
            return false;
        }
        
        int result = zmq_bridge_send(socketId, data, data.Length);
        if (result != ZMQ_BRIDGE_OK)
        {
            Debug.LogError($"Failed to send data through socket '{socketName}': {GetLastError()}");
            return false;
        }
        
        return true;
    }
    
    // Envia uma string através de um socket
    public bool SendString(string socketName, string message)
    {
        if (!_sockets.TryGetValue(socketName, out int socketId))
        {
            Debug.LogError($"Socket '{socketName}' not found");
            return false;
        }
        
        int result = zmq_bridge_send_string(socketId, message);
        if (result != ZMQ_BRIDGE_OK)
        {
            Debug.LogError($"Failed to send string through socket '{socketName}': {GetLastError()}");
            return false;
        }
        
        return true;
    }
    
    // Publica dados em um tópico
    public bool PublishData(string socketName, string topic, byte[] data)
    {
        if (!_sockets.TryGetValue(socketName, out int socketId))
        {
            Debug.LogError($"Socket '{socketName}' not found");
            return false;
        }
        
        int result = zmq_bridge_publish(socketId, topic, data, data.Length);
        if (result != ZMQ_BRIDGE_OK)
        {
            Debug.LogError($"Failed to publish data on topic '{topic}' through socket '{socketName}': {GetLastError()}");
            return false;
        }
        
        return true;
    }
    
    // Publica uma string em um tópico
    public bool PublishString(string socketName, string topic, string message)
    {
        byte[] data = Encoding.UTF8.GetBytes(message);
        return PublishData(socketName, topic, data);
    }
    
    // Recebe dados através de um socket
    public byte[] ReceiveData(string socketName)
    {
        if (!_sockets.TryGetValue(socketName, out int socketId))
        {
            Debug.LogError($"Socket '{socketName}' not found");
            return null;
        }
        
        int bytesReceived = 0;
        int result = zmq_bridge_receive(socketId, _receiveBuffer, _receiveBuffer.Length, ref bytesReceived);
        
        if (result == ZMQ_BRIDGE_NO_MESSAGE)
        {
            return null; // Nenhuma mensagem disponível
        }
        
        if (result != ZMQ_BRIDGE_OK)
        {
            Debug.LogError($"Failed to receive data from socket '{socketName}': {GetLastError()}");
            return null;
        }
        
        byte[] data = new byte[bytesReceived];
        Array.Copy(_receiveBuffer, data, bytesReceived);
        return data;
    }
    
    // Recebe uma string através de um socket
    public string ReceiveString(string socketName)
    {
        if (!_sockets.TryGetValue(socketName, out int socketId))
        {
            Debug.LogError($"Socket '{socketName}' not found");
            return null;
        }
        
        _stringBuffer.Clear();
        int result = zmq_bridge_receive_string(socketId, _stringBuffer, _stringBuffer.Capacity);
        
        if (result == ZMQ_BRIDGE_NO_MESSAGE)
        {
            return null; // Nenhuma mensagem disponível
        }
        
        if (result != ZMQ_BRIDGE_OK)
        {
            Debug.LogError($"Failed to receive string from socket '{socketName}': {GetLastError()}");
            return null;
        }
        
        return _stringBuffer.ToString();
    }
    
    // Verifica se há mensagens disponíveis em um socket
    public bool CheckMessage(string socketName)
    {
        if (!_sockets.TryGetValue(socketName, out int socketId))
        {
            Debug.LogError($"Socket '{socketName}' not found");
            return false;
        }
        
        int result = zmq_bridge_check_message(socketId);
        return result == 1;
    }
    
    // Aguarda mensagens com timeout
    public bool WaitForMessage(string socketName, int timeoutMs)
    {
        if (!_sockets.TryGetValue(socketName, out int socketId))
        {
            Debug.LogError($"Socket '{socketName}' not found");
            return false;
        }
        
        int result = zmq_bridge_poll(socketId, timeoutMs);
        return result == 1;
    }
    
    // Verifica e processa mensagens disponíveis
    public void PollSocket(string socketName)
    {
        if (!_sockets.TryGetValue(socketName, out int socketId))
        {
            return;
        }
        
        if (zmq_bridge_poll(socketId, 0) == 1)
        {
            // Há mensagem disponível, vamos processá-la
            int bytesReceived = 0;
            int result = zmq_bridge_receive(socketId, _receiveBuffer, _receiveBuffer.Length, ref bytesReceived);
            
            if (result == ZMQ_BRIDGE_OK && bytesReceived > 0)
            {
                // Copia os dados recebidos
                byte[] data = new byte[bytesReceived];
                Array.Copy(_receiveBuffer, data, bytesReceived);
                
                // Dispara o evento de recepção
                OnMessageReceived?.Invoke(socketName, data);
                
                // Tenta converter para string
                try
                {
                    string message = Encoding.UTF8.GetString(data);
                    OnStringMessageReceived?.Invoke(socketName, message);
                }
                catch
                {
                    // Ignora erros de conversão
                }
            }
        }
    }
    
    // Fecha um socket
    public void CloseSocket(string socketName)
    {
        if (_sockets.TryGetValue(socketName, out int socketId))
        {
            zmq_bridge_close_socket(socketId);
            _sockets.Remove(socketName);
            Debug.Log($"Socket '{socketName}' closed");
        }
    }
    
    // Obtém a descrição do último erro
    private string GetLastError()
    {
        IntPtr errorPtr = zmq_bridge_get_last_error();
        return Marshal.PtrToStringAnsi(errorPtr);
    }
}
