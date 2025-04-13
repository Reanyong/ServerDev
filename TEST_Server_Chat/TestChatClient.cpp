#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// 클라이언트 세션 클래스
class Client {
public:
    // 생성자에서 서버에 연결
    Client(net::io_context& ioc, unsigned int id)
        : id_(id), ws_(net::make_strand(ioc)) {
    }

    // 서버에 연결
    void connect(const std::string& host, unsigned short port) {
        tcp::resolver resolver(ws_.get_executor());
        auto results = resolver.resolve(host, std::to_string(port));

        net::connect(ws_.next_layer(), results.begin(), results.end());
        ws_.handshake(host, "/");

        // 닉네임 설정
        std::string nickname = "TestClient" + std::to_string(id_);
        ws_.write(net::buffer("/nick " + nickname));

        // 환영 메시지 수신
        beast::flat_buffer buffer;
        ws_.read(buffer);
        buffer.consume(buffer.size());
    }

    // 메시지 전송
    void send_messages(int count) {
        for (int i = 0; i < count; ++i) {
            std::string msg = "Test message #" + std::to_string(i) + " from client " + std::to_string(id_);
            ws_.write(net::buffer(msg));

            // 응답 수신
            beast::flat_buffer buffer;
            ws_.read(buffer);
            buffer.consume(buffer.size());

            std::this_thread::sleep_for(std::chrono::milliseconds(100 + rand() % 400));
        }
    }

    // 연결 종료
    void close() {
        ws_.close(websocket::close_code::normal);
    }

private:
    unsigned int id_;
    websocket::stream<beast::tcp_stream> ws_;
};

int main() {
    try {
        const int CLIENT_COUNT = 100;
        const int MESSAGE_COUNT = 50;

        net::io_context ioc;
        std::vector<std::shared_ptr<Client>> clients;

        // 클라이언트 생성
        for (int i = 0; i < CLIENT_COUNT; ++i) {
            clients.push_back(std::make_shared<Client>(ioc, i));
        }

        // 연결
        for (auto& client : clients) {
            client->connect("localhost", 8080);
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        // 메시지 전송
        std::vector<std::thread> threads;
        for (auto& client : clients) {
            threads.emplace_back([&client, MESSAGE_COUNT]() {
                client->send_messages(MESSAGE_COUNT);
                });
        }

        // 모든 스레드 종료 대기
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() / 1000.0;

        std::cout << "Test completed in " << elapsed << " seconds" << std::endl;
        std::cout << "Throughput: " << (CLIENT_COUNT * MESSAGE_COUNT) / elapsed << " messages/sec" << std::endl;

        // 연결 종료
        for (auto& client : clients) {
            client->close();
        }
    }
    catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
