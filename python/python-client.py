import zmq
import json
import time
import numpy as np
from threading import Thread, Event
from typing import Callable, Dict, Any, Optional, List, Tuple

class SimulatorClient:
    """
    Cliente Python para comunicação com o simulador Unity via ZeroMQ
    """
    
    def __init__(self, host: str = "localhost"):
        """
        Inicializa o cliente do simulador
        
        Args:
            host: Endereço do servidor (por padrão, localhost)
        """
        self.context = zmq.Context()
        self.host = host
        self.subscribers = {}
        self.running = True
        
        # Flag para indicar que o cliente está conectado
        self.connected = False
        
        # Dicionário para armazenar os callbacks de mensagens
        self.message_callbacks = {}
        
        # Threads de polling
        self.polling_threads = {}
        self.stop_events = {}
    
    def connect(self) -> bool:
        """
        Conecta ao servidor do simulador
        
        Returns:
            bool: True se a conexão foi bem-sucedida, False caso contrário
        """
        try:
            # Socket para enviar comandos
            self.command_socket = self.context.socket(zmq.PUB)
            self.command_socket.connect(f"tcp://{self.host}:5556")
            
            # Socket para enviar controles do veículo
            self.control_socket = self.context.socket(zmq.PUSH)
            self.control_socket.connect(f"tcp://{self.host}:5557")
            
            # Aguarda a conexão ser estabelecida
            time.sleep(0.1)
            
            self.connected = True
            print(f"Connected to simulator at {self.host}")
            return True
        except zmq.ZMQError as e:
            print(f"Failed to connect to simulator: {e}")
            return False
    
    def subscribe(self, topic: str, callback: Callable[[Dict[str, Any]], None]) -> bool:
        """
        Subscreve a um tópico do simulador
        
        Args:
            topic: Nome do tópico (ex: "camera", "vehicle", etc.)
            callback: Função de callback para processar as mensagens recebidas
            
        Returns:
            bool: True se a subscrição foi bem-sucedida, False caso contrário
        """
        try:
            if topic in self.subscribers:
                # Já existe uma subscrição para este tópico
                self.message_callbacks[topic] = callback
                return True
            
            # Cria um novo socket subscriber
            subscriber = self.context.socket(zmq.SUB)
            subscriber.connect(f"tcp://{self.host}:5555")
            subscriber.setsockopt_string(zmq.SUBSCRIBE, topic)
            
            # Armazena o socket
            self.subscribers[topic] = subscriber
            
            # Registra o callback
            self.message_callbacks[topic] = callback
            
            # Inicia uma thread para polling deste tópico
            stop_event = Event()
            self.stop_events[topic] = stop_event
            
            thread = Thread(target=self._polling_thread, args=(topic, subscriber, callback, stop_event))
            thread.daemon = True
            thread.start()
            
            self.polling_threads[topic] = thread
            
            print(f"Subscribed to topic '{topic}'")
            return True
        except zmq.ZMQError as e:
            print(f"Failed to subscribe to topic '{topic}': {e}")
            return False
    
    def _polling_thread(self, topic: str, socket: zmq.Socket, callback: Callable, stop_event: Event):
        """
        Thread interna para polling de mensagens de um tópico
        
        Args:
            topic: Nome do tópico
            socket: Socket ZeroMQ
            callback: Função de callback
            stop_event: Evento para sinalizar parada da thread
        """
        poller = zmq.Poller()
        poller.register(socket, zmq.POLLIN)
        
        while not stop_event.is_set():
            socks = dict(poller.poll(timeout=100))
            
            if socket in socks and socks[socket] == zmq.POLLIN:
                try:
                    # Recebe a mensagem
                    topic_recv, data = socket.recv_multipart()
                    
                    # Decodifica o tópico
                    topic_str = topic_recv.decode('utf-8')
                    
                    # Tenta decodificar como JSON
                    try:
                        message = json.loads(data)
                    except json.JSONDecodeError:
                        # Não é JSON, trata como dados binários
                        message = {'raw_data': data}
                    
                    # Chama o callback
                    callback(message)
                except zmq.ZMQError as e:
                    if self.running:
                        print(f"Error receiving message on topic '{topic}': {e}")
                except Exception as e:
                    print(f"Error processing message on topic '{topic}': {e}")
    
    def send_command(self, command: str, params: Optional[Dict[str, Any]] = None) -> bool:
        """
        Envia um comando para o simulador
        
        Args:
            command: Nome do comando
            params: Parâmetros do comando (opcional)
            
        Returns:
            bool: True se o comando foi enviado com sucesso, False caso contrário
        """
        if not self.connected:
            print("Not connected to simulator")
            return False
        
        if params is None:
            params = {}
        
        try:
            message = {
                'command': command,
                'params': params
            }
            
            json_message = json.dumps(message)
            self.command_socket.send_multipart([b"command", json_message.encode('utf-8')])
            return True
        except zmq.ZMQError as e:
            print(f"Failed to send command '{command}': {e}")
            return False
    
    def send_vehicle_control(self, throttle: float, steering: float, brake: float) -> bool:
        """
        Envia controles para o veículo
        
        Args:
            throttle: Valor do acelerador (0.0 a 1.0)
            steering: Valor da direção (-1.0 a 1.0, onde -1.0 é esquerda máxima)
            brake: Valor do freio (0.0 a 1.0)
            
        Returns:
            bool: True se os controles foram enviados com sucesso, False caso contrário
        """
        if not self.connected:
            print("Not connected to simulator")
            return False
        
        try:
            control = {
                'throttle': float(throttle),
                'steering': float(steering),
                'brake': float(brake)
            }
            
            json_control = json.dumps(control)
            self.control_socket.send_string(json_control)
            return True
        except zmq.ZMQError as e:
            print(f"Failed to send vehicle control: {e}")
            return False
    
    def close(self):
        """
        Fecha a conexão com o simulador
        """
        self.running = False
        
        # Para todas as threads de polling
        for topic, event in self.stop_events.items():
            event.set()
        
        # Aguarda as threads terminarem
        for topic, thread in self.polling_threads.items():
            thread.join(timeout=1.0)
        
        # Fecha todos os sockets
        for topic, socket in self.subscribers.items():
            socket.close()
        
        if hasattr(self, 'command_socket'):
            self.command_socket.close()
        
        if hasattr(self, 'control_socket'):
            self.control_socket.close()
        
        self.context.term()
        self.connected = False
        print("Disconnected from simulator")


# Exemplo de uso
if __name__ == "__main__":
    # Cria o cliente
    client = SimulatorClient()
    
    # Conecta ao simulador
    if client.connect():
        try:
            # Callback para dados da câmera
            def on_camera_data(data):
                if 'raw_data' in data:
                    # Dados binários da imagem
                    print(f"Received camera data: {len(data['raw_data'])} bytes")
                else:
                    print(f"Received camera data: {data}")
            
            # Callback para dados do veículo
            def on_vehicle_data(data):
                print(f"Received vehicle data: {data}")
            
            # Subscreve aos tópicos
            client.subscribe("camera", on_camera_data)
            client.subscribe("vehicle", on_vehicle_data)
            
            # Envia comandos de exemplo
            client.send_command("reset_simulation")
            
            # Envia controles do veículo
            client.send_vehicle_control(0.5, 0.0, 0.0)  # 50% acelerador, sem direção, sem freio
            
            # Aguarda para receber mensagens
            print("Waiting for messages (Ctrl+C to exit)...")
            while True:
                time.sleep(0.1)
        
        except KeyboardInterrupt:
            print("Interrupted by user")
        finally:
            # Fecha o cliente
            client.close()
    else:
        print("Failed to connect to simulator")
