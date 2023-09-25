#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <unordered_set>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctime>
#include <sys/time.h>  // For timeval
#include <chrono>


const int PORT = 8080;
const int BUFFER_SIZE = 1400;

struct Packet {
    bool isHeader;
    bool isEnd;
    int index;
    char data[BUFFER_SIZE];
};

// Function to serialize the Packet
std::vector<char> serializePacket(const Packet& packet) {
    std::vector<char> serializedData(sizeof(Packet));
    std::memcpy(serializedData.data(), &packet, sizeof(Packet));
    return serializedData;
}

// Function to deserialize into Packet
Packet deserializePacket(const std::vector<char>& serializedData) {
    Packet packet;
    std::memcpy(&packet, serializedData.data(), sizeof(Packet));
    return packet;
}



int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cerr << "Usage: ./sender" << " <Receiver IP Address> <file>" << std::endl;
        return 1;
    }

    Packet packetFileInfo;
    int sockfd;
    struct sockaddr_in server_addr;

    // get the name of file and load it 
    strcpy(packetFileInfo.data, argv[2]);

    std::ifstream file(packetFileInfo.data, std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open file." << std::endl;
        return 1;
    }

    // file info
    file.seekg(0, std::ios::end);
    int fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    int packets = (fileSize + BUFFER_SIZE - 1) / BUFFER_SIZE;
    packetFileInfo.index = fileSize;
    packetFileInfo.isHeader = true;
    packetFileInfo.isEnd = false;
    std::cout << "Sending "<< argv[2] << " File size: " << packetFileInfo.index << " Packets number: "<< packets << std::endl;

    // Create a buffer to hold the entire file
    std::vector<char> fileBuffer(fileSize);

    // Read the entire file into the buffer
    file.read(fileBuffer.data(), fileSize);

    // Close the file as we've read it entirely into memory
    file.close();
    
    // create a socket fd
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // set up IPv4 address, Port number and  of server
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    socklen_t len = sizeof(server_addr);

    std::vector<char> serialized = serializePacket(packetFileInfo);
    // Send handshake
    for (int i = 0; i < 5; i++) {
        sendto(sockfd, serialized.data(), serialized.size() * sizeof(char), 0, (const struct sockaddr *) &server_addr, len);
    }

    // start timer
    auto start = std::chrono::high_resolution_clock::now();

    auto current_time_point = std::chrono::system_clock::now();
    // Convert the time point to a time_t
    std::time_t current_time_t = std::chrono::system_clock::to_time_t(current_time_point);

    // Convert the time_t to a human-readable string
    std::cout << "Start time: " << std::ctime(&current_time_t);
    
    Packet packet;
    packet.isHeader = false;
    packet.isEnd = false;
    std::cout<< "Sending whole file" << std::endl;
    // first round send
    for (int index = 0; index < packets; index++) {
        usleep(10);

        memset(packet.data, 0, BUFFER_SIZE);

        if (index == packets - 1) {
            // std::cout << "Current file position: " << file.tellg() << std::endl;
        }

        // Calculate the offset into the buffer for this packet
        int offset = index * BUFFER_SIZE;

        // Calculate the size of data to read for this packet
        int dataSize = std::min(BUFFER_SIZE, fileSize - offset);

        // Copy data from the buffer into the packet
        std::memcpy(packet.data, fileBuffer.data() + offset, dataSize);


        // Send the data to the receiver
        packet.index = index;
        
        if (packet.index == packets - 1) {
            // std::cout<< "packet index: " << packet.index << std::endl;
            // std::cout<< "packet content: " << packet.data << std::endl;
            // std::cout<< "packet size: " << dataSize << std::endl;
        }
        // Send the packet
        std::vector<char> serialized = serializePacket(packet);
        sendto(sockfd, serialized.data(), serialized.size() * sizeof(char), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));

    }
    // ending signal
    // packet.index = 1000000;
    packet.isEnd = true;
    serialized = serializePacket(packet);
    for (int i = 0; i < 10; i++){
        sendto(sockfd, serialized.data(), serialized.size() * sizeof(char), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
    }
    packet.isEnd = false;
    // std::cout<< "Finished sending whole file" << std::endl;

    int round = 1;
    // Resend the file
    while (1) {
        // std::cout<< "------start " << round  << "th round------" << std::endl;
        // Receive the lost_list
        char recvBuffer[BUFFER_SIZE];
        // std::cout << "Waiting for lost list..." << std::endl;
        std::unordered_set<int> lost_list;
        bool IsEndSglRemoved = false;
        bool LostListEnd = false;
        bool LostListEmpty = false;

        while (1) {
            int bytes_received = recvfrom(sockfd, recvBuffer, BUFFER_SIZE, 0, (struct sockaddr *) &server_addr, &len);
            int* index_ptr;
            for (int i = 0; i < bytes_received; i += sizeof(int)) {
                index_ptr = reinterpret_cast<int*>(recvBuffer + i);

                if (bytes_received == 4) // std::cout << "first int: " << *index_ptr<< std::endl;
                if (*index_ptr == -1) {
                    // std::cout << "cleaned the lost list ending signal before receiving lost list. Ending signal size: " << bytes_received<< std::endl;
                    break;
                } else if (*index_ptr == -2) {
                    // std::cout << "received the finished signal: Empty Lost Lost" << std::endl;
                    LostListEmpty = true;
                    IsEndSglRemoved = true;
                    break;
                }
                lost_list.insert(*index_ptr);
                IsEndSglRemoved = true;
            }
            if (IsEndSglRemoved == true) {
                // std::cout << "received first lost list"<< std::endl;
                break;
            } else if (LostListEmpty == true) {
                // std::cout << "received empty lost list signal"<< std::endl;
                break;
            }
        }
        
        if(LostListEmpty == true) {
            break;
        }

        while(1) {
            int bytes_received = recvfrom(sockfd, recvBuffer, BUFFER_SIZE, 0, (struct sockaddr *) &server_addr, &len);
            if (bytes_received < 0) {
                perror("Receiving lost list error");
            }
            for (int i = 0; i < bytes_received; i += sizeof(int)) {
                int* index_ptr = reinterpret_cast<int*>(recvBuffer + i);
                if (*index_ptr == -1) {
                    LostListEnd = true;
                    break;
                }
                lost_list.insert(*index_ptr);
            }
            if (LostListEnd == true) {
                // std::cout << "Lost list received successfully" << std::endl;
                break;
            }
        }

        
        // std::cout << "Received Lost List size: " << lost_list.size() << std::endl;
        // // std::cout << "Fisrt elemet: " << lost_list.begin() <<  " Last elemet: "  << lost_list.end() << std::endl; // segment fault for unordered_set
        // std::cout << "Sending lost packets..." << std::endl;
        // Resend file
        for (int index : lost_list) {
            usleep(10);
            // Seek to the position in the file for this packet
            if (lost_list.find(index) == lost_list.end()) continue;
            memset(packet.data, 0, BUFFER_SIZE);

            packet.index = index;
            // std::cout<< "packet index: " << packet.index << std::endl;
           
            // Calculate the offset into the buffer for this packet
            int offset = index * BUFFER_SIZE;

            // Calculate the size of data to read for this packet
            int dataSize = std::min(BUFFER_SIZE, fileSize - offset);

            // Copy data from the buffer into the packet
            std::memcpy(packet.data, fileBuffer.data() + offset, dataSize);

            // std::cout<< "packet size: " << bytesRead << std::endl;

            // Send the packet
            std::vector<char> serialized = serializePacket(packet);
            sendto(sockfd, serialized.data(), serialized.size() * sizeof(char), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
            if (packet.index == packets - 1) {
                // std::cout<< "packet index: " << packet.index << std::endl;
                // std::cout<< "packet content: " << packet.data << std::endl;
                // std::cout<< "packet size: " << dataSize << std::endl;
            }
        }
        // Send ending signal
        // packet.index = 1000000;
        packet.isEnd = true;
        std::vector<char> serialized = serializePacket(packet);
        for (int i = 0; i < 10; i++) {
            // std::cout<< "sent round finished signal:" << i << std::endl;
            sendto(sockfd, serialized.data(), serialized.size() * sizeof(char), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
        }
        // std::cout<< "sent round finished signal isEnd:" << packet.isEnd << std::endl;
        packet.isEnd = false;
        // std::cout<< "finished " << round  << "th round" << std::endl;
        round++;
    }

    // stop timer
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "----------------Finish Transmissin---------------"<< std::endl;

    auto current_time_point_2 = std::chrono::system_clock::now();
    // Convert the time point to a time_t
    std::time_t current_time_t_2 = std::chrono::system_clock::to_time_t(current_time_point_2);

    // Convert the time_t to a human-readable string
    std::cout << "Finish time: " << std::ctime(&current_time_t_2);

    std::chrono::duration<double> duration = end - start; // Time taken in seconds
    double throughput = (fileSize / (1024.0 * 1024.0)) / duration.count(); // Throughput in MB/s
    std::cout << "Throughput: " << throughput << " MB/s" << std::endl;

    close(sockfd);

    return 0;
}
