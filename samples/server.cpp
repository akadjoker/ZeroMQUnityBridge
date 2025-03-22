#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <zmq.hpp>
#include <ZMQBridge.h>

// Exemplo de servidor simulando um simulador Unity
int main(int argc, char* argv[])
{
    std::cout << "Starting ZeroMQ server example..." << std::endl;

    // Inicializa a biblioteca
    if (zmq_bridge_init() != 0)
    {
        std::cerr << "Failed to initialize ZeroMQ bridge: "
                  << zmq_bridge_get_last_error() << std::endl;
        return 1;
    }

    // Cria sockets de comunicação
    int pub_socket = zmq_bridge_create_publisher("tcp://*:5555");
    int cmd_socket = zmq_bridge_create_pull("tcp://*:5556");
    int ctrl_socket = zmq_bridge_create_pull("tcp://*:5557");

    if (pub_socket < 0 || cmd_socket < 0 || ctrl_socket < 0)
    {
        std::cerr << "Failed to create sockets: " << zmq_bridge_get_last_error()
                  << std::endl;
        zmq_bridge_shutdown();
        return 1;
    }

    std::cout << "Server is running..." << std::endl;
    std::cout << "PUB socket: tcp://*:5555" << std::endl;
    std::cout << "CMD socket: tcp://*:5556" << std::endl;
    std::cout << "CTRL socket: tcp://*:5557" << std::endl;

    // Flag para controlar o loop principal
    std::atomic<bool> running{ true };

    // Valores de simulação do veículo
    double throttle = 0.0;
    double steering = 0.0;
    double brake = 0.0;
    double position_x = 0.0;
    double position_y = 0.0;
    double speed = 0.0;

    // Thread para receber comandos
    std::thread cmd_thread([&]() {
        char buffer[1024];

        while (running)
        {
            // Verifica por comandos
            if (zmq_bridge_poll(cmd_socket, 100) > 0)
            {
                if (zmq_bridge_receive_string(cmd_socket, buffer,
                                              sizeof(buffer))
                    == 0)
                {
                    std::cout << "Received command: " << buffer << std::endl;

                    // Processa o comando
                    std::string cmd(buffer);
                    if (cmd.find("reset") != std::string::npos)
                    {
                        // Reseta a simulação
                        throttle = 0.0;
                        steering = 0.0;
                        brake = 0.0;
                        position_x = 0.0;
                        position_y = 0.0;
                        speed = 0.0;
                        std::cout << "Simulation reset" << std::endl;
                    }
                }
            }
        }
    });

    // Thread para receber controles
    std::thread ctrl_thread([&]() {
        char buffer[1024];

        while (running)
        {
            // Verifica por controles
            if (zmq_bridge_poll(ctrl_socket, 100) > 0)
            {
                if (zmq_bridge_receive_string(ctrl_socket, buffer,
                                              sizeof(buffer))
                    == 0)
                {
                    std::cout << "Received control: " << buffer << std::endl;

                    // Processa o controle
                    std::string ctrl(buffer);

                    // Extração simples de valores
                    size_t throttle_pos = ctrl.find("\"throttle\":");
                    size_t steering_pos = ctrl.find("\"steering\":");
                    size_t brake_pos = ctrl.find("\"brake\":");

                    if (throttle_pos != std::string::npos)
                    {
                        throttle_pos += 11; // Comprimento de "\"throttle\":"
                        throttle = std::stod(ctrl.substr(
                            throttle_pos,
                            ctrl.find(',', throttle_pos) - throttle_pos));
                    }

                    if (steering_pos != std::string::npos)
                    {
                        steering_pos += 11; // Comprimento de "\"steering\":"
                        steering = std::stod(ctrl.substr(
                            steering_pos,
                            ctrl.find(',', steering_pos) - steering_pos));
                    }

                    if (brake_pos != std::string::npos)
                    {
                        brake_pos += 8; // Comprimento de "\"brake\":"
                        brake = std::stod(ctrl.substr(
                            brake_pos, ctrl.find('}', brake_pos) - brake_pos));
                    }
                }
            }
        }
    });

    // Loop principal - simula e publica dados
    while (running)
    {
        try
        {
            // Simula o movimento do veículo
            if (throttle > 0.0)
            {
                speed += throttle * 0.1; // Aceleração simples
            }

            if (brake > 0.0)
            {
                speed -= brake * 0.2; // Desaceleração
            }

            // Limite de velocidade
            if (speed > 10.0) speed = 10.0;
            if (speed < 0.0) speed = 0.0;

            // Movimento com base na direção
            position_x += speed * steering * 0.05;
            position_y += speed * 0.1;

            // Prepara os dados do veículo
            char vehicle_data[256];
            snprintf(vehicle_data, sizeof(vehicle_data),
                     "{\"position\":[%.2f,%.2f,0.0],\"speed\":%.2f,"
                     "\"throttle\":%.2f,\"steering\":%.2f,\"brake\":%.2f}",
                     position_x, position_y, speed, throttle, steering, brake);

            // Publica os dados do veículo
            zmq_bridge_publish(pub_socket, "vehicle", vehicle_data,
                               strlen(vehicle_data));

            // Simula dados da câmera (apenas uma string simples neste exemplo)
            const char* camera_data = "Simulated camera data (would be binary "
                                      "image data in a real scenario)";
            zmq_bridge_publish(pub_socket, "camera", camera_data,
                               strlen(camera_data));

            // A cada segundo, mostra o estado da simulação
            std::cout << "Vehicle state: position=(" << position_x << ","
                      << position_y << "), speed=" << speed
                      << ", controls=(throttle=" << throttle
                      << ", steering=" << steering << ", brake=" << brake << ")"
                      << std::endl;

  
            std::this_thread::sleep_for(std::chrono::milliseconds(100));


            if (std::cin.peek() == 'q')
            {
                running = false;
            }
        } catch (const std::exception& e)
        {
            std::cerr << "Error in main loop: " << e.what() << std::endl;
        }
    }

    // Aguarda as threads terminarem
    std::cout << "Shutting down..." << std::endl;
    cmd_thread.join();
    ctrl_thread.join();


    zmq_bridge_close_socket(pub_socket);
    zmq_bridge_close_socket(cmd_socket);
    zmq_bridge_close_socket(ctrl_socket);

    zmq_bridge_shutdown();

    std::cout << "Server stopped" << std::endl;
    return 0;
}
