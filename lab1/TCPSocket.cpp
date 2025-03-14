#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional> // std::bind函数 需要包含 functional 头文件

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "DES.h"

using namespace std;

// 命令行界面优化
void setColor(int color_code) {
    std::cout << "\033[" << color_code << "m";
}

void resetColor() {
    std::cout << "\033[0m";
}

// 全局变量，用于控制线程运行
atomic<bool> running(true);
mutex mtx;

// 客户端类
class TCPClient {
    int clientSocket;
    sockaddr_in serverAddr;
    int serverPort;

    atomic<bool> running{true}; // 全局变量

public:
    TCPClient(const string& ip, int port) : serverPort(port) {
        // 创建 Socket
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            cerr << "Error creating socket" << endl;
            exit(1);
        }

        // 设置服务器地址
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
            cerr << "Invalid address/ Address not supported" << endl;
            close(clientSocket);
            exit(1);
        }

        // 连接到服务器
        if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "Error connecting to server" << endl;
            close(clientSocket);
            exit(1);
        }
        cout << "Connected to server on port " << serverPort << endl;
    }

    void start(bitset<64> key) {
        // 创建发送和接收线程
        thread sendThread(std::bind(&TCPClient::sendMessages, this, key));
        thread recvThread(std::bind(&TCPClient::receiveMessages, this, key));

        // 等待线程结束
        sendThread.join();
        recvThread.join();

        // 关闭 Socket
        close(clientSocket);
    }

private:
    void sendMessages(bitset<64> key) {
        char buffer[1024];
        char Encryptedbuffer[1024];
        while (running) {
            // 读取用户输入
            cin.getline(buffer, sizeof(buffer));

            mtx.lock();
            setColor(33);
            cout << "You: " << buffer << " ";

            // 填充输入数据到 8 的倍数
            size_t inputLen = strlen(buffer);
            if (inputLen % 8 != 0) {
                size_t paddingLen = 8 - (inputLen % 8);
                memset(buffer + inputLen, ' ', paddingLen); // 用空格填充
                buffer[inputLen + paddingLen] = '\0';       // 添加字符串结束符
                inputLen += paddingLen;                     // 更新输入长度
            }

            // 对输入数据进行 DES 加密
            for (size_t i = 0; i < inputLen; i += 8) {
                // 将 8 字节数据转换为 64 位
                u_int64_t temp = 0;
                memcpy(&temp, buffer + i, 8);

                // 加密
                bitset<64> tempBits(temp);
                bitset<64> encryptedBits = desEncrypt(tempBits, generateSubKeys(key));

                // 将加密后的数据存储到 Encryptedbuffer
                u_int64_t encryptedValue = encryptedBits.to_ullong();
                memcpy(Encryptedbuffer + i, &encryptedValue, 8);
            }

            // 输出加密后的数据（以十六进制形式）
            cout << "(DES加密后：";
            for (size_t i = 0; i < inputLen; i++) {
                printf("%02X ", (unsigned char)Encryptedbuffer[i]);
            }
            cout << ")" << endl;
            resetColor();

            // 发送加密后的数据
            if (send(clientSocket, Encryptedbuffer, inputLen, 0) < 0) {
                cerr << "Error sending message" << endl;
                mtx.unlock();
                break;
            }

            // 如果输入 "exit"，关闭连接
            if (strcmp(buffer, "exit") == 0) {
                running = false;
                break;
            }
 
            mtx.unlock();
        }
    }

    void receiveMessages(bitset<64> key) {
        char buffer[1024];
        char decryptedBuffer[1024];
        while (running) {
            // 接收加密数据
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) {
                cerr << "Server disconnected" << endl;
                running = false;
                break;
            }
    
            // 对接收到的数据进行 DES 解密
            for (int i = 0; i < bytesReceived; i += 8) {
                // 将 8 字节数据转换为 64 位
                u_int64_t temp = 0;
                memcpy(&temp, buffer + i, 8);
    
                // 解密
                bitset<64> encryptedBits(temp);
                bitset<64> decryptedBits = desDecrypt(encryptedBits, generateSubKeys(key));
    
                // 将解密后的数据存储到 decryptedBuffer
                u_int64_t decryptedValue = decryptedBits.to_ullong();
                memcpy(decryptedBuffer + i, &decryptedValue, 8);
            }
    
            // 添加字符串结束符
            decryptedBuffer[bytesReceived] = '\0';

            if (strcmp(buffer, "exit") == 0) {
                running = false;
                cout<<"Server close."<<endl;
                break;
            }
    
            // 输出解密后的数据
            setColor(32);
            cout << "Server: " << decryptedBuffer << endl;
            resetColor();
        }
    }
};

// 服务器类
class TCPServer {
    int serverSocket;
    int clientSocket;
    sockaddr_in serverAddr;
    sockaddr_in clientAddr;
    int serverPort;

    atomic<bool> running{true}; // 全局变量

public:
    TCPServer(int port) : serverPort(port) {
        // 创建 Socket
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            cerr << "Error creating socket" << endl;
            exit(1);
        }

        // 设置服务器地址
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(serverPort);

        // 绑定 Socket
        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "Error binding socket" << endl;
            close(serverSocket);
            exit(1);
        }

        // 监听连接
        if (listen(serverSocket, 1) < 0) {
            cerr << "Error listening" << endl;
            close(serverSocket);
            exit(1);
        }
        cout << "Server started on port " << serverPort << ". Listen ..." << endl;

        // 接受客户端连接
        socklen_t clientAddrLen = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket < 0) {
            cerr << "Error accepting connection" << endl;
            close(serverSocket);
            exit(1);
        }
        cout << "Client connected" << endl;
    }

    void start(bitset<64> key) {
        // 创建发送和接收线程
        thread sendThread(std::bind(&TCPServer::sendMessages, this, key));
        thread recvThread(std::bind(&TCPServer::receiveMessages, this, key));

        // 等待线程结束
        sendThread.join();
        recvThread.join();

        // 关闭 Socket
        close(clientSocket);
        close(serverSocket);
    }

private:
void sendMessages(bitset<64> key) {
    char buffer[1024];
    char Encryptedbuffer[1024];
    while (running) {
        // 读取用户输入
        cin.getline(buffer, sizeof(buffer));

        mtx.lock();
        setColor(33);
        cout << "You: " << buffer << " ";

        // 填充输入数据到 8 的倍数
        size_t inputLen = strlen(buffer);
        if (inputLen % 8 != 0) {
            size_t paddingLen = 8 - (inputLen % 8);
            memset(buffer + inputLen, ' ', paddingLen); // 用空格填充
            buffer[inputLen + paddingLen] = '\0';       // 添加字符串结束符
            inputLen += paddingLen;                     // 更新输入长度
        }

        // 对输入数据进行 DES 加密
        for (size_t i = 0; i < inputLen; i += 8) {
            // 将 8 字节数据转换为 64 位
            u_int64_t temp = 0;
            memcpy(&temp, buffer + i, 8);

            // 加密
            bitset<64> tempBits(temp);
            bitset<64> encryptedBits = desEncrypt(tempBits, generateSubKeys(key));

            // 将加密后的数据存储到 Encryptedbuffer
            u_int64_t encryptedValue = encryptedBits.to_ullong();
            memcpy(Encryptedbuffer + i, &encryptedValue, 8);
        }

        // 输出加密后的数据（以十六进制形式）
        cout << "(DES加密后：";
        for (size_t i = 0; i < inputLen; i++) {
            printf("%02X ", (unsigned char)Encryptedbuffer[i]);
        }
        cout << ")" << endl;
        resetColor();

        // 发送加密后的数据
        if (send(clientSocket, Encryptedbuffer, inputLen, 0) < 0) {
            cerr << "Error sending message" << endl;
            mtx.unlock();
            break;
        }

         // 如果输入 "exit"，关闭连接
         if (strcmp(buffer, "exit") == 0) {
            running = false;
            //cout<<"Server close"<<endl;
            break;
        }

        mtx.unlock();
    }

}

void receiveMessages(bitset<64> key) {
    char buffer[1024];
    char decryptedBuffer[1024];
    while (running) {
        // 接收加密数据
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            cerr << "Client disconnected" << endl;
            running = false;
            break;
        }

        // 对接收到的数据进行 DES 解密
        for (int i = 0; i < bytesReceived; i += 8) {
            // 将 8 字节数据转换为 64 位
            u_int64_t temp = 0;
            memcpy(&temp, buffer + i, 8);

            // 解密
            bitset<64> encryptedBits(temp);
            bitset<64> decryptedBits = desDecrypt(encryptedBits, generateSubKeys(key));

            // 将解密后的数据存储到 decryptedBuffer
            u_int64_t decryptedValue = decryptedBits.to_ullong();
            memcpy(decryptedBuffer + i, &decryptedValue, 8);
        }

        // 添加字符串结束符
        decryptedBuffer[bytesReceived] = '\0';

        if (strcmp(decryptedBuffer, "exit") == 0) {
            running = false;
            cout<<"Client close."<<endl;
            break;
        }

        // 输出解密后的数据
        setColor(32);
        cout << "Client: " << decryptedBuffer << endl;
        resetColor();
    }
}
};

int main() {
    // 协商密钥
    bitset<64> key = 0x133457799BBCDFF1;

    // 选择客户端或服务器端
    char choice;
    cout << "Server(s) or Client(c): ";
    cin >> choice;
    cin.ignore(); // 忽略换行符

    if (choice == 's') {
        // 服务器端
        TCPServer server(12345);
        server.start(key);
        server.~TCPServer();
    } else if (choice == 'c') {
        // 客户端
        string serverIP;
        cout << "Enter server IP: ";
        getline(cin, serverIP);
        TCPClient client(serverIP, 12345);
        client.start(key);
        client.~TCPClient();
    } else {
        cerr << "Invalid choice" << endl;
    }

    return 0;
}