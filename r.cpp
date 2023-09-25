#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <ctime>
#include <sys/time.h>  // For timeval


const int PORT = 8080;
const int BUFFER_SIZE = 1400;

// ?
const int UDP_MTU = 1400;
const int MAX_INTS_PER_MESSAGE = (UDP_MTU / sizeof(int)); // 64kB divided by the size of an int

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

    // Server information
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    socklen_t len = sizeof(client_addr);

    // Bind the socket
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    
    std::vector<char> buffer(sizeof(Packet));
    std::cout << "size of packet: "<< sizeof(Packet) << std::endl;
    std::cout << "Waiting for file information... " << std::endl;

    int receivedBytes = recvfrom(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &client_addr, &len);
    if (receivedBytes < 0) {
        perror("Receiving file info error");
    }

    // Deserialize
    Packet packetFileInfo = deserializePacket(buffer);

    std::vector<char> fileBuffer(packetFileInfo.index);
    // Calculate the number of packets
    int packetsNum = (packetFileInfo.index + BUFFER_SIZE - 1) / BUFFER_SIZE;
    
    std::cout << "Received file: " << packetFileInfo.data << " Size: " << packetFileInfo.index << " Packets number: "<< packetsNum << std::endl;
    int BytesLastPacket = packetFileInfo.index % 1400;
    // Create a file to write
    std::ofstream file(packetFileInfo.data, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing." << std::endl;
        return 1;
    }

    // create a lost list
    std::unordered_set<int> lost_set;
    for (int i = 0; i < packetsNum; i++) {
        lost_set.insert(i);
    }

    // deal with the redundant handshake and write the fisrt packet
    while(1){
        memset(buffer.data(), 0, buffer.size());
        int receivedBytes = recvfrom(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &client_addr, &len);    
        if (receivedBytes < 0) {
            perror("Receiving file info error");
        }
        // Deserialize
        Packet packetFileInfo = deserializePacket(buffer);

        if(packetFileInfo.isHeader == true){
            continue;
        }
        else{
            int index = packetFileInfo.index;

            // Write to buffer instead of file
            std::memcpy(&fileBuffer[index * BUFFER_SIZE], packetFileInfo.data, sizeof(packetFileInfo.data));

            lost_set.erase(index);
            // std::cout<< "erased packet index: " << packetFileInfo.index << std::endl;

            memset(packetFileInfo.data, 0, BUFFER_SIZE);
            break;
        }
    }

    // Receive data 
    Packet packet;
    int round = 1;
    while (1) {
        // clean the isEnd signal
        while(1){
            memset(buffer.data(), 0, buffer.size());
            int receivedBytes = recvfrom(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &client_addr, &len);    
            if (receivedBytes < 0) {
                perror("Receiving file info error");
            }
            // Deserialize
            Packet packet = deserializePacket(buffer);

            int index = packet.index;
    
            if (packet.isEnd == true) {
                //  << "Cleaned a redundant stop flag before receiving!" << std::endl;
                continue;
            }
            else{
                std::memcpy(&fileBuffer[index * BUFFER_SIZE], packet.data, (packetsNum - 1 == packet.index) ? BytesLastPacket : sizeof(packet.data));
                lost_set.erase(index);
                // std::cout<< "erased packet index: " << packet.index << std::endl;
                // std::cout << "Receiving packets..." << std::endl;
                // std::cout << "Received index is (Outer Layer)"<< index << std::endl;
                break;
            }
        }
        // std::cout<<"jump out of the while 1" <<std::endl;
        // receive packets
        int NumOfPacRevd = 1;
        int count = 0;
        while (1) {       
            if(lost_set.empty()) {
                break;
            }
            memset(buffer.data(), 0, buffer.size());
            int receivedBytes = recvfrom(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr *) &client_addr, &len);    
            // std::cout<<"count: "<< count++ <<std::endl;
            if (receivedBytes < 0) {
                perror("Receiving file info error");
            }
            // Deserialize
            Packet packet = deserializePacket(buffer);
            int index = packet.index;

            if (packet.isEnd == true) {
                // std::cout << "Received stop flag!" << std::endl;
                break;
            }

            if(lost_set.find(index) == lost_set.end()) {
                // std::cout << "Index not found: " << index << std::endl;
            }
            else {
                NumOfPacRevd++;
                // Write to buffer instead of file
                std::memcpy(&fileBuffer[index * BUFFER_SIZE], packet.data, 
                            (packetsNum - 1 == packet.index) ? BytesLastPacket : sizeof(packet.data));
                lost_set.erase(index);
            }
        }
        // std::cout << "Received "<< NumOfPacRevd <<" packets" << std::endl;
        // std::cout << "Lost set: " << std::endl;
        // for (const int& value : lost_set) {
        //     std::cout << value << " ";
        // }
        // std::cout << std::endl;
        
        //send lost list
        std::vector<int> lost_vector(lost_set.begin(), lost_set.end());
        // std::cout << "Lost vector: " << std::endl;
        // for (const int& value : lost_set) {
        //     std::cout << value << " ";
        // }
        // std::cout << std::endl;
        if (lost_vector.empty()) {
            lost_vector.push_back(-2);
            int tmp = sendto(sockfd, lost_vector.data(), lost_vector.size() * sizeof(int), 0, (struct sockaddr *) &client_addr, len);
            // std::cout << "sent ending chunk signal:" << lost_vector.back() << "size: "<< lost_vector.size()<< std::endl;
            tmp = sendto(sockfd, lost_vector.data(), lost_vector.size() * sizeof(int), 0, (struct sockaddr *) &client_addr, len);
            // std::cout << "sent ending chunk signal:" << lost_vector.back() << "size: "<< lost_vector.size()<< std::endl;
            tmp = sendto(sockfd, lost_vector.data(), lost_vector.size() * sizeof(int), 0, (struct sockaddr *) &client_addr, len);
            // std::cout << "sent ending chunk signal:" << lost_vector.back() << "size: "<< lost_vector.size()<< std::endl;
            break;
        } else {
            // std::cout << "------Start "<< round << "th round------" << std::endl;
            // Split huge lost list into chunks and send each chunk
            // std::cout << "Sending lost list" << std::endl;

            int numChunks = (lost_vector.size() + MAX_INTS_PER_MESSAGE - 1) / MAX_INTS_PER_MESSAGE;
            // std::cout << "Total number of chunks: " << numChunks << std::endl;

            // Populate the chunks
            for (int chunkIndex = 0; chunkIndex < numChunks; ++chunkIndex) {
                // Determine the start and end of this chunk
                int start = chunkIndex * MAX_INTS_PER_MESSAGE;
                int end = std::min(start + MAX_INTS_PER_MESSAGE, (int)lost_vector.size());

                // Create the chunk for this range
                std::vector<int> chunk;
                for (int i = start; i < end; ++i) {
                    chunk.push_back(lost_vector[i]);
                }
                for (int i = 0; i < 2; i++) {
                    int tmp = sendto(sockfd, chunk.data(), chunk.size() * sizeof(int), 0, (struct sockaddr *) &client_addr, len);
                    if(tmp == -1) perror("Chunk sent error");
                    // else std::cout << "sent chunk: size: " << chunk.size() << std::endl;
                }
                if (numChunks < 3) {
                    for (int i = 0; i < 10; i++) {
                        int tmp = sendto(sockfd, chunk.data(), chunk.size() * sizeof(int), 0, (struct sockaddr *) &client_addr, len);
                        if(tmp == -1) perror("Chunk sent error");
                        // else std::cout << "sent chunk: size: " << chunk.size() << std::endl;
                    }
                }
                if(chunkIndex == numChunks -1) {
                    std::vector<int> end_chunk;
                    end_chunk.push_back(-1);
                    for (int i = 0; i < 10; i++) {
                        int tmp = sendto(sockfd, end_chunk.data(), end_chunk.size() * sizeof(int), 0, (struct sockaddr *) &client_addr, len);
                        if(tmp == -1){
                            perror("Chunk sent error");
                        } else{
                            // std::cout << "sent ending chunk signal:"<< end_chunk.back() << "size: "<< end_chunk.size()<< std::endl;
                        }
                    }
                }
            }

            // std::cout << "Lost list length: "<< lost_vector.size()  << " First element: " << lost_vector.front() << " Last element: " << lost_vector.back() << std::endl;
            round++; 
        }
    }

    std::cout << "File successfully transfered!"<< std::endl;
    file.write(fileBuffer.data(), fileBuffer.size());
    file.close();
    close(sockfd);


    return 0;
}