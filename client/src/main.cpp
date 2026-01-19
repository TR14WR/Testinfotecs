#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <cmath>
#include <numeric>
#include <future>
#include <sstream>

#include <boost/asio.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "../../common/DataStructures.h"
#include "../../common/Logger.h"
#include "../../common/Utils.h"

/**
 * @brief Вычисляет значение функции 1/ln(x) для интегрирования.
 * 
 * @param x Точка, в которой вычисляется функция.
 * @return Значение функции или 0.0 для особых случаев.
 */
double integrate_function(double x) {
    if (x <= 1.0 || std::abs(std::log(x)) < 1e-10) {
        // Обработка особых случаев для 1/ln(x) при x <= 1 или ln(x) близко к 0
        // Возвращаем 0 для точек, где функция не определена
        return 0.0; 
    }
    return 1.0 / std::log(x);
}

/**
 * @brief Класс клиента для распределенного интегрирования.
 * 
 * Подключается к серверу, получает задачи и выполняет интегрирование
 * с использованием всех доступных ядер CPU.
 */
class Client {
public:
    /**
     * @brief Конструктор клиента.
     * 
     * @param io_context Контекст ввода-вывода Boost.Asio.
     * @param host Адрес сервера.
     * @param port Порт сервера.
     */
    Client(boost::asio::io_context& io_context, const std::string& host, short port)
        : socket_(io_context) {
        LOG_INFO << "Клиент пытается подключиться к " << host << ":" << port;
        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::connect(socket_, resolver.resolve(host, std::to_string(port)));
        LOG_INFO << "Клиент подключен к серверу.";

        try {
            // Получаем ID клиента от сервера
            receive_data(socket_, client_id_);
            
            // Отправляем серверу количество ядер CPU
            num_cores_ = std::thread::hardware_concurrency();
            if (num_cores_ == 0) {
                num_cores_ = 1; // Минимум одно ядро
                LOG_WARNING << "Не удалось определить количество ядер, используем 1";
            }
            send_data(socket_, num_cores_);
            
            LOG_INFO << "Клиент " << client_id_ << " получил ID сессии. Количество ядер CPU: " << num_cores_;
        } catch (const std::exception& e) {
            LOG_ERROR << "Ошибка при подключении к серверу: " << e.what();
            throw;
        }

        // Начинаем асинхронное чтение задач
        do_read_task();
    }

private:
    /**
     * @brief Асинхронно читает задачи от сервера.
     */
    void do_read_task() {
        // Используем отдельный поток для синхронного чтения
        std::thread([this]() {
            try {
                while (true) {
                    IntegrationTask task;
                    receive_data(socket_, task);

                    LOG_INFO << "Клиент " << client_id_ << " получил задачу " << task.task_id
                             << ": [" << task.lower_bound << ", " << task.upper_bound << "] с шагом " << task.step;

                    // Выполняем интегрирование в нескольких потоках
                    double partial_result = perform_integration(task);

                    // Отправляем результат обратно на сервер
                    IntegrationResult result = {partial_result, task.task_id};
                    send_data(socket_, result);

                    LOG_INFO << "Клиент " << client_id_ << " отправил результат " << result.task_id 
                             << ": " << result.result;
                }
            } catch (const std::exception& e) {
                LOG_INFO << "Сервер отключился: " << e.what();
            }
        }).detach();
    }

    /**
     * @brief Выполняет интегрирование задачи с использованием всех ядер CPU.
     * 
     * Разделяет задачу на подзадачи по количеству ядер и выполняет их параллельно.
     * 
     * @param task Задача интегрирования.
     * @return Результат интегрирования.
     */
    double perform_integration(const IntegrationTask& task) {
        double total_result = 0.0;
        std::vector<std::future<double>> futures;

        double range_size = task.upper_bound - task.lower_bound;
        if (range_size <= 0 || task.step <= 0) {
            return 0.0;
        }

        // Делим диапазон на поддиапазоны по количеству ядер
        double sub_range_length = range_size / num_cores_;

        // Создаем задачи для каждого ядра
        for (size_t i = 0; i < num_cores_; ++i) {
            double sub_lower_bound = task.lower_bound + i * sub_range_length;
            double sub_upper_bound = (i == num_cores_ - 1) ? task.upper_bound : sub_lower_bound + sub_range_length;

            // Запускаем вычисление в отдельном потоке
            futures.push_back(std::async(std::launch::async, [sub_lower_bound, sub_upper_bound, task]() {
                double sub_integral = 0.0;
                double x = sub_lower_bound;
                
                // Используем метод прямоугольников для интегрирования
                while (x < sub_upper_bound) {
                    double next_x = std::min(x + task.step, sub_upper_bound);
                    double mid_x = (x + next_x) / 2.0;
                    sub_integral += integrate_function(mid_x) * (next_x - x);
                    x = next_x;
                }
                
                return sub_integral;
            }));
        }

        // Собираем результаты от всех потоков
        for (auto& future : futures) {
            total_result += future.get();
        }

        return total_result;
    }

    boost::asio::ip::tcp::socket socket_;
    size_t client_id_;
    size_t num_cores_;
};

int main() {
    init_logging();
    LOG_INFO << "Приложение клиента запущено.";

    try {
        boost::asio::io_context io_context;
        Client client(io_context, "127.0.0.1", 12345);
        
        // Запускаем io_context (будет работать до закрытия соединения)
        io_context.run();
    } catch (std::exception& e) {
        LOG_FATAL << "Исключение в приложении клиента: " << e.what();
    }

    LOG_INFO << "Приложение клиента завершено.";
    return 0;
}
