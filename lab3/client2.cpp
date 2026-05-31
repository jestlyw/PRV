
#include <boost/asio.hpp>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io;
        tcp::socket socket(io);
        socket.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 12346));
        std::cout << "Подключено к серверу\n";

        std::cout << "Введите числа через пробел: ";
        std::string msg;
        std::getline(std::cin, msg);

        boost::asio::write(socket, boost::asio::buffer(msg + "\n"));

        boost::asio::streambuf buf;
        boost::asio::read_until(socket, buf, '\n');

        std::istream is(&buf);
        std::string response;
        std::getline(is, response);
        std::cout << "Ответ сервера: " << response << "\n";
    } catch (std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
    }
    return 0;
}
