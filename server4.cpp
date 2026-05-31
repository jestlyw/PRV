
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <cstdint>

using boost::asio::ip::tcp;


std::vector<std::string> log_storage;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket,
            boost::asio::strand<boost::asio::io_context::executor_type> strand)
        : socket_(std::move(socket)),
          strand_(strand),
          timer_(socket_.get_executor()) {}

    void start() {
      
        timer_.expires_after(boost::asio::chrono::seconds(30));
        timer_.async_wait(boost::asio::bind_executor(strand_,
            [this, self = shared_from_this()](boost::system::error_code ec) {
                if (!ec) {
                    std::cout << "Таймаут клиента, закрываю соединение\n";
                    socket_.close();
                }
            }));
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            boost::asio::bind_executor(strand_,
                [this, self](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        std::string request(data_, length);
                        
                        while (!request.empty() &&
                               (request.back() == '\n' || request.back() == '\r'))
                            request.pop_back();
                        std::cout << "Получено: \"" << request << "\"\n";
                        handle_request(request);
                    } else {
                        std::cout << "Клиент отключился: " << ec.message() << "\n";
                        timer_.cancel();
                    }
                }));
    }

    void handle_request(const std::string& request) {
        auto self(shared_from_this());
        
        boost::asio::post(strand_,
            [this, self, request]() {
                std::string response;
                try {
                    int n = std::stoi(request);
                    if (n < 0 || n > 20) {
                        response = "Ошибка: введите число от 0 до 20\n";
                    } else {
                        uint64_t result = 1;
                        for (int i = 2; i <= n; ++i)
                            result *= i;
                        response = "Factorial(" + std::to_string(n) + ") = "
                                   + std::to_string(result) + "\n";
                    }
                } catch (...) {
                    response = "Ошибка: не удалось распознать число\n";
                }

                
                boost::asio::post(strand_, [this, self, response]() {
                    log_storage.push_back(response);
                    std::cout << "[ЛОГ] " << response;
                });

                do_write(response);
            });
    }

    void do_write(const std::string& response) {
        auto self(shared_from_this());
        auto buf = std::make_shared<std::string>(response);
        boost::asio::async_write(socket_,
            boost::asio::buffer(*buf),
            boost::asio::bind_executor(strand_,
                [this, self, buf](boost::system::error_code ec, std::size_t) {
                    if (!ec) {
                        // Сбрасываем таймаут после каждого ответа
                        timer_.expires_after(boost::asio::chrono::seconds(30));
                        do_read();
                    }
                }));
    }

    tcp::socket socket_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    boost::asio::steady_timer timer_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class Server {
public:
    Server(boost::asio::io_context& io, short port)
        : io_(io),
          acceptor_(io, tcp::endpoint(tcp::v4(), port)),
          strand_(io.get_executor()) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::cout << "Клиент подключён: "
                              << socket.remote_endpoint() << "\n";
                    std::make_shared<Session>(std::move(socket), strand_)->start();
                }
                do_accept();
            });
    }

    boost::asio::io_context& io_;
    tcp::acceptor acceptor_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
};

int main(int argc, char* argv[]) {
    try {
        int num_threads = 4;
        if (argc > 1)
            num_threads = std::stoi(argv[1]);

        std::cout << "Многопоточный сервер, потоков: " << num_threads << "\n";
        std::cout << "Порт: 12348\n";

        boost::asio::io_context io;
        Server s(io, 12348);

        // Пул потоков
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i)
            threads.emplace_back([&io]() { io.run(); });

        std::cout << "Сервер запущен. Ctrl+C для остановки.\n";

        for (auto& t : threads)
            t.join();

        // Выводим лог при завершении
        std::cout << "\n=== ЛОГ СЕССИЙ ===\n";
        for (const auto& entry : log_storage)
            std::cout << entry;

    } catch (std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
    }
    return 0;
}
