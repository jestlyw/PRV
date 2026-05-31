// server1.cpp — Задача 1: синхронный TCP-сервер, возвращает строку в верхнем регистре
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <algorithm>
#include <locale>
#include <codecvt>

using boost::asio::ip::tcp;


std::string to_upper_utf8(const std::string& s) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::wstring ws = conv.from_bytes(s);
    std::locale loc("ru_RU.UTF-8");
    for (auto& c : ws)
        c = std::toupper(c, loc);
    return conv.to_bytes(ws);
}

int main() {
    try {
        boost::asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "Сервер запущен на порту 12345\n";

        for (;;) {
            tcp::socket socket(io);
            acceptor.accept(socket);
            std::cout << "Клиент подключён: " << socket.remote_endpoint() << "\n";

            boost::asio::streambuf buf;
            boost::system::error_code ec;
            boost::asio::read_until(socket, buf, '\n', ec);

            if (ec && ec != boost::asio::error::eof) {
                std::cerr << "Ошибка чтения: " << ec.message() << "\n";
                continue;
            }

            std::istream is(&buf);
            std::string msg;
            std::getline(is, msg);

            if (!msg.empty() && msg.back() == '\r')
                msg.pop_back();

            std::cout << "Получено: " << msg << "\n";

            std::string upper = to_upper_utf8(msg);

            std::string response = upper + "\n";
            boost::asio::write(socket, boost::asio::buffer(response), ec);

            std::cout << "Отправлено: " << upper << "\n";
        }
    } catch (std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
    }
    return 0;
}
