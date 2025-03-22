#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <zmq.hpp>
#include <ZMQBridge.h>

 
int main(int argc, char* argv[])
{
    std::cout << "Starting ZeroMQ client example..." << std::endl;

    // Endereço do servidor
    std::string server_address = "localhost";
    if (argc > 1)
    {
        server_address = argv[1];
    }

    // Inicializa a biblioteca
    if (zmq_bridge_init() != 0)
    {
        std::cerr << "Failed to initialize ZeroMQ bridge: "
                  << zmq_bridge_get_last_error() << std::endl;
        return 1;
    }

    // Cria sockets de comunicação
    int sub_socket = zmq_bridge_create_subscriber(
        ("tcp://" + server_address + ":5555").c_str(),
        ""); // Subscreve a todos os tópicos
    int cmd_socket =
        zmq_bridge_create_push(("tcp://" + server_address + ":5556").c_str());
    int ctrl_socket =
        zmq_bridge_create_push(("tcp://" + server_address + ":5557").c_str());

    if (sub_socket < 0 || cmd_socket < 0 || ctrl_socket < 0)
    {
        std::cerr << "Failed to create sockets: " << zmq_bridge_get_last_error()
                  << std::endl;
        zmq_bridge_shutdown();
        return 1;
    }

    std::cout << "Connected to server at " << server_address << std::endl;

    // Flag para controlar o loop principal
    std::atomic<bool> running{ true };

    // Thread para receber dados
    std::thread recv_thread([&]() {
        char buffer[8192];

        while (running)
        {
            // Verifica por mensagens
            if (zmq_bridge_poll(sub_socket, 100) > 0)
            {
                if (zmq_bridge_receive_string(sub_socket, buffer,
                                              sizeof(buffer))
                    == 0)
                {
                    // O primeiro frame é o tópico
                    std::string topic(buffer);

                    // Recebe o segundo frame (dados)
                    if (zmq_bridge_receive_string(sub_socket, buffer,
                                                  sizeof(buffer))
                        == 0)
                    {
                        std::cout << "Received data from topic '" << topic
                                  << "': " << buffer << std::endl;
                    }
                }
            }
        }
    });

    // Envia comando inicial (reset)
    std::cout << "Sending 'reset' command..." << std::endl;
    zmq_bridge_send_string(cmd_socket, "{\"command\":\"reset\"}");

    // Loop principal - envia controles com base na entrada do usuário
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  throttle X  - Set throttle to X (0.0-1.0)" << std::endl;
    std::cout << "  steering X  - Set steering to X (-1.0-1.0)" << std::endl;
    std::cout << "  brake X     - Set brake to X (0.0-1.0)" << std::endl;
    std::cout << "  reset       - Reset simulation" << std::endl;
    std::cout << "  quit        - Exit program" << std::endl;

    std::string input;
    double throttle = 0.0;
    double steering = 0.0;
    double brake = 0.0;

    while (running)
    {
        try
        {
            std::cout << "> ";
            std::getline(std::cin, input);

            if (input == "quit" || input == "exit")
            {
                running = false;
                continue;
            }

            if (input == "reset")
            {
                // Envia comando de reset
                std::cout << "Sending 'reset' command..." << std::endl;
                zmq_bridge_send_string(cmd_socket, "{\"command\":\"reset\"}");
                continue;
            }

            // Processa comandos de controle
            if (input.find("throttle") == 0)
            {
                throttle = std::stod(input.substr(9));
                throttle = (throttle < 0.0) ? 0.0
                    : (throttle > 1.0)      ? 1.0
                                            : throttle;
            }
            else if (input.find("steering") == 0)
            {
                steering = std::stod(input.substr(9));
                steering = (steering < -1.0) ? -1.0
                    : (steering > 1.0)       ? 1.0
                                             : steering;
            }
            else if (input.find("brake") == 0)
            {
                brake = std::stod(input.substr(6));
                brake = (brake < 0.0) ? 0.0 : (brake > 1.0) ? 1.0 : brake;
            }
            else
            {
                std::cout << "Unknown command" << std::endl;
                continue;
            }

            // Cria mensagem de controle
            char control_msg[256];
            snprintf(control_msg, sizeof(control_msg),
                     "{\"throttle\":%.2f,\"steering\":%.2f,\"brake\":%.2f}",
                     throttle, steering, brake);

            // Envia controles
            std::cout << "Sending control: " << control_msg << std::endl;
            zmq_bridge_send_string(ctrl_socket, control_msg);
        } catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    // Aguarda a thread terminar
    std::cout << "Shutting down..." << std::endl;
    recv_thread.join();

    // Fecha os sockets e finaliza
    zmq_bridge_close_socket(sub_socket);
    zmq_bridge_close_socket(cmd_socket);
    zmq_bridge_close_socket(ctrl_socket);

    zmq_bridge_shutdown();

    std::cout << "Client stopped" << std::endl;
    return 0;
}
