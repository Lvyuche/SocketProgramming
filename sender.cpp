#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

struct FileInfo {
    std::string filename;
    int packets;
};

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cerr << "Usage: ./sender" << " <Receiver IP Address> <file>" << std::endl;
        return 1;
    }

    int sockfd;
    struct sockaddr_in server_addr;
    FileInfo fileInfo, ack;

    // get the name and amount of packets of file  
    fileInfo.filename = argv[2];

    std::ifstream file(fileInfo.filename, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open file." << std::endl;
        return 1;
    }

    file.seekg(0, std::ios::end);  // move the cursor from the beginning to the end
    int fileSize = file.tellg();  // return the position of the cursor, which is the size of the file
    file.seekg(0, std::ios::beg);  // move the cursor back to the beginning
    int packets = (fileSize + BUFFER_SIZE - 1) / BUFFER_SIZE;  // File size might not be divied by buffer size, so add one extra packet.
    fileInfo.packets = packets;

    std::cout << "Need to send "<< fileInfo.packets << " packets" << std::endl;

    // create a socket fd
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // set up IPv4 address, Port number and  of server-
    memset(&server_addr, 0, sizeof(server_addr));

    std::cout << "UDP Socket created!" << std::endl;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    // const std::string file_name = argv[2];

    // Send file info && check the integrity
    while (sendto(sockfd, &fileInfo, sizeof(fileInfo), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr))) {
        recvfrom(sockfd, &ack, sizeof(ack), 0, nullptr, nullptr);
        if (ack.filename == fileInfo.filename && ack.packets == fileInfo.packets) break;
    }

    std::cout << "Start to send file!" << std::endl;

    // Send file
    char buffer[BUFFER_SIZE];
    int i = 1, sum = 0;
    while (file.read(buffer, sizeof(buffer))) {
        sendto(sockfd, buffer, sizeof(buffer), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
        sum += sizeof(buffer);
        std::cout << "Packet " << i++ << " is send: " << sum << std::endl;
    }

    if (file.gcount() > 0) {
        sendto(sockfd, buffer, file.gcount(), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
        std::cout << "Last packet " << i++ << " is send" << std::endl;
        std::cout << "The whole size should be: " << sum + file.gcount() << std::endl;
    }

    file.close();
    close(sockfd);

    return 0;
}
