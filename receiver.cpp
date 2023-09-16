#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    std::cout << "Socket created!" << std::endl;

    // Server information
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    std::ofstream file("received_file.txt", std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing." << std::endl;
        return 1;
    }

    char buffer[BUFFER_SIZE];
    socklen_t len = sizeof(client_addr);
    int n;

    std::cout << "Socket binded!" << std::endl;

    int sum = 0;
    while ((n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &len)) > 0) {
        std::cout << buffer << std::endl;
        sum += n;
        file.write(buffer, n);
    }

    std::cout << "The total number of data is: " << sum << std::endl;

    file.close();
    close(sockfd);

    return 0;
}
