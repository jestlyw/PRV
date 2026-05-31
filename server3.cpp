
#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket)
        : socket_(std::move(socket)),
          timer_(socket_.get_executor()) {}

    void start() { do_read(); }

private:
    void do_read() {
        auto self(shared_from_this());
        boost::asio::async_read_until(socket_, buf_, '\n',
            [this, self](boost::system::error_code ec, std::size_t) {
                if (!ec) {
                    std::istream is(&buf_);
                    std::string line;
                    std::getline(is, line);
                    if (!line.empty() && line.back() == '\r')
                        line.pop_back();

                    std::cout << "Получено: \"" << line << "\"\n";
                    handle_request(line);
                }
            });
    }

    void handle_request(const std::string& line) {
        auto self(shared_from_this());

        // Парсим: "ping" или "ping 5"
        int delay = 3; // дефолт 3 сек
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        if (cmd == "ping") {
            int n;
            if (iss >> n && n > 0)
                delay = n;

            std::cout << "Запускаю таймер на " << delay << " сек...\n";
            timer_.expires_after(boost::asio::chrono::seconds(delay));
            timer_.async_wait([this, self](boost::system::error_code ec) {
                if (!ec) {
                    do_write("pong\n");
                }
            });
        } else {
            do_write("Неизвестная команда. Используйте: ping [N]\n");
        }
    }

    void do_write(const std::string& response) {
        auto self(shared_from_this());
        auto buf = std::make_shared<std::string>(response);
        boost::asio::async_write(socket_, boost::asio::buffer(*buf),
            [this, self, buf](boost::system::error_code ec, std::size_t) {
                if (!ec)
                    do_read();
            });
    }

    tcp::socket socket_;
    boost::asio::streambuf buf_;
    boost::asio::steady_timer timer_;
};

class Server {
public:
    Server(boost::asio::io_context& io, short port)
        : acceptor_(io, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::cout << "Клиент подключён: " << socket.remote_endpoint() << "\n";
                    std::make_shared<Session>(std::move(socket))->start();
                }
                do_accept();
            });
    }

    tcp::acceptor acceptor_;
};

int main() {
    try {
        boost::asio::io_context io;
        Server s(io, 12347);
        std::cout << "Сервер (ping/pong) запущен на порту 12347\n";
        io.run();
    } catch (std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
    }
    return 0;
}
