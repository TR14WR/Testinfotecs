#pragma once

#include <boost/asio.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <cstdint>

/**
 * @brief Отправляет сериализованные данные по сокету.
 * 
 * Сначала отправляет размер данных (4 байта), затем сами данные.
 * 
 * @tparam T Тип отправляемых данных.
 * @param socket Ссылка на сокет Boost.Asio.
 * @param data Данные для отправки.
 */
template<typename T>
void send_data(boost::asio::ip::tcp::socket& socket, const T& data) {
    // Сериализуем данные в строку
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << data;
    
    std::string outbound_data = archive_stream.str();
    
    // Отправляем размер данных
    uint32_t size = static_cast<uint32_t>(outbound_data.size());
    boost::asio::write(socket, boost::asio::buffer(&size, sizeof(size)));
    
    // Отправляем сами данные
    boost::asio::write(socket, boost::asio::buffer(outbound_data));
}

/**
 * @brief Получает и десериализует данные из сокета.
 * 
 * Сначала читает размер данных (4 байта), затем сами данные.
 * 
 * @tparam T Тип получаемых данных.
 * @param socket Ссылка на сокет Boost.Asio.
 * @param data Ссылка, куда будут десериализованы данные.
 */
template<typename T>
void receive_data(boost::asio::ip::tcp::socket& socket, T& data) {
    // Читаем размер данных
    uint32_t size = 0;
    boost::asio::read(socket, boost::asio::buffer(&size, sizeof(size)));
    
    // Читаем сами данные
    boost::asio::streambuf response_buf;
    boost::asio::read(socket, response_buf, boost::asio::transfer_exactly(size));
    
    // Десериализуем данные
    std::istream archive_stream(&response_buf);
    boost::archive::text_iarchive archive(archive_stream);
    archive >> data;
}
