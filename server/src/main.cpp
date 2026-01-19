#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <thread>
#include <map>
#include <mutex>
#include <future>
#include <atomic>
#include <condition_variable>
#include <cmath>
#include <sstream>
#include <chrono>
#include <functional>

#include <boost/asio.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "../../common/DataStructures.h"
#include "../../common/Logger.h"
#include "../../common/Utils.h"

/**
 * @brief Класс для управления сессией клиента.
 * 
 * Обрабатывает подключение клиента, отправку задач и получение результатов.
 */
class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    /**
     * @brief Конструктор сессии клиента.
     * 
     * @param socket Сокет для связи с клиентом.
     * @param id Уникальный идентификатор сессии.
     */
    ClientSession(boost::asio::ip::tcp::socket socket, size_t id)
        : socket_(std::move(socket)), id_(id), num_cores_(0) {
        LOG_INFO << "Сессия клиента " << id_ << " создана.";
    }

    /**
     * @brief Запускает сессию клиента.
     * 
     * Отправляет клиенту его ID, получает количество ядер CPU от клиента,
     * затем начинает ожидание результатов.
     */
    void start() {
        try {
            // Отправляем клиенту его ID сессии
            send_data(socket_, id_);
            
            // Получаем количество ядер CPU от клиента
            receive_data(socket_, num_cores_);

            if (num_cores_ == 0) {
                num_cores_ = std::thread::hardware_concurrency();
                LOG_WARNING << "Клиент " << id_ << " сообщил 0 ядер, используем значение по умолчанию: " << num_cores_;
            } else {
                LOG_INFO << "Клиент " << id_ << " сообщил количество ядер CPU: " << num_cores_;
            }

            // Начинаем асинхронное чтение результатов от клиента
            do_read_result();
        } catch (const std::exception& e) {
            LOG_ERROR << "Ошибка при инициализации сессии клиента " << id_ << ": " << e.what();
        }
    }

    /**
     * @brief Получает идентификатор сессии.
     * 
     * @return Идентификатор сессии.
     */
    size_t get_id() const {
        return id_;
    }

    /**
     * @brief Получает количество ядер CPU клиента.
     * 
     * @return Количество ядер CPU.
     */
    size_t get_num_cores() const {
        return num_cores_;
    }

    /**
     * @brief Получает ссылку на сокет клиента.
     * 
     * @return Ссылка на сокет.
     */
    boost::asio::ip::tcp::socket& get_socket() {
        return socket_;
    }

    /**
     * @brief Отправляет задачу клиенту.
     * 
     * @param task Задача для отправки.
     */
    void send_task(const IntegrationTask& task) {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        try {
            send_data(socket_, task);
            LOG_INFO << "Задача " << task.task_id << " отправлена клиенту " << id_;
        } catch (const std::exception& e) {
            LOG_ERROR << "Ошибка при отправке задачи клиенту " << id_ << ": " << e.what();
            throw;
        }
    }

    /**
     * @brief Устанавливает callback для обработки результатов.
     * 
     * @param callback Функция, которая будет вызвана при получении результата.
     */
    void set_result_callback(std::function<void(const IntegrationResult&)> callback) {
        result_callback_ = callback;
    }

private:
    /**
     * @brief Асинхронно читает результат от клиента.
     */
    void do_read_result() {
        auto self = shared_from_this();
        
        // Используем отдельный поток для синхронного чтения
        std::thread([this, self]() {
            try {
                while (true) {
                    IntegrationResult result;
                    receive_data(socket_, result);
                    
                    LOG_INFO << "Получен результат от клиента " << id_ << " для задачи " << result.task_id 
                             << ": " << result.result;
                    
                    if (result_callback_) {
                        result_callback_(result);
                    }
                }
            } catch (const std::exception& e) {
                LOG_INFO << "Клиент " << id_ << " отключился: " << e.what();
            }
        }).detach();
    }

    boost::asio::ip::tcp::socket socket_;
    size_t id_;
    size_t num_cores_;
    std::function<void(const IntegrationResult&)> result_callback_;
    std::mutex socket_mutex_; ///< Мьютекс для синхронизации доступа к сокету
};

/**
 * @brief Класс сервера для распределенного интегрирования.
 * 
 * Управляет подключениями клиентов, распределяет задачи между ними
 * и собирает результаты вычислений.
 */
class Server {
public:
    /**
     * @brief Конструктор сервера.
     * 
     * @param io_context Контекст ввода-вывода Boost.Asio.
     * @param port Порт для прослушивания подключений.
     */
    Server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
          next_client_id_(0),
          next_task_id_(0),
          total_cores_(0),
          results_received_(0),
          expected_results_(0),
          final_result_(0.0),
          results_ready_(false) {
        LOG_INFO << "Сервер запущен на порту " << port;
        do_accept();
    }

    /**
     * @brief Обрабатывает запрос на интегрирование.
     * 
     * Разделяет задачу на подзадачи и распределяет их между подключенными клиентами
     * пропорционально количеству ядер CPU каждого клиента.
     * 
     * @param lower_bound Нижний предел интегрирования.
     * @param upper_bound Верхний предел интегрирования.
     * @param step Шаг интегрирования.
     * @return Результат интегрирования.
     */
    double handle_integration_request(double lower_bound, double upper_bound, double step) {
        LOG_INFO << "Получен запрос на интегрирование: [" << lower_bound << ", " << upper_bound 
                 << "] с шагом " << step;

        std::lock_guard<std::mutex> lock(clients_mutex_);
        
        if (clients_.empty()) {
            LOG_WARNING << "Нет подключенных клиентов для выполнения задачи.";
            return 0.0;
        }

        // Подсчитываем общее количество ядер CPU
        total_cores_ = 0;
        for (const auto& pair : clients_) {
            total_cores_ += pair.second->get_num_cores();
        }

        if (total_cores_ == 0) {
            LOG_WARNING << "Общее количество ядер CPU равно нулю.";
            return 0.0;
        }

        LOG_INFO << "Общее количество ядер CPU всех клиентов: " << total_cores_;

        // Сбрасываем счетчики результатов
        {
            std::lock_guard<std::mutex> results_lock(results_mutex_);
            results_.clear();
            results_received_ = 0;
            expected_results_ = 0;
            final_result_ = 0.0;
            results_ready_ = false;
        }

        // Разделяем задачу на подзадачи
        std::vector<IntegrationTask> tasks = divide_task(lower_bound, upper_bound, step);
        {
            std::lock_guard<std::mutex> results_lock(results_mutex_);
            expected_results_ = tasks.size();
        }

        LOG_INFO << "Задача разделена на " << tasks.size() << " подзадач";

        // Устанавливаем callback для обработки результатов
        for (auto& pair : clients_) {
            pair.second->set_result_callback(
                [this](const IntegrationResult& result) {
                    handle_result(result);
                });
        }

        // Распределяем задачи между клиентами
        size_t task_index = 0;
        for (const auto& pair : clients_) {
            auto client = pair.second;
            size_t client_cores = client->get_num_cores();
            
            // Вычисляем количество задач для этого клиента пропорционально его ядрам
            size_t tasks_for_client = (client_cores * tasks.size() + total_cores_ - 1) / total_cores_;
            
            LOG_INFO << "Клиенту " << client->get_id() << " назначено " << tasks_for_client 
                     << " задач (ядер: " << client_cores << ")";
            
            // Отправляем задачи клиенту
            for (size_t i = 0; i < tasks_for_client && task_index < tasks.size(); ++i, ++task_index) {
                tasks[task_index].task_id = next_task_id_++;
                client->send_task(tasks[task_index]);
            }
        }

        // Ждем получения всех результатов
        std::unique_lock<std::mutex> results_lock(results_mutex_);
        results_cv_.wait(results_lock, [this] { return results_ready_; });

        LOG_INFO << "Все результаты получены. Итоговый результат: " << final_result_;
        return final_result_;
    }

private:
    /**
     * @brief Разделяет задачу интегрирования на подзадачи.
     * 
     * @param lower_bound Нижний предел интегрирования.
     * @param upper_bound Верхний предел интегрирования.
     * @param step Шаг интегрирования.
     * @return Вектор подзадач.
     */
    std::vector<IntegrationTask> divide_task(double lower_bound, double upper_bound, double step) {
        std::vector<IntegrationTask> tasks;
        
        // Вычисляем количество интервалов
        double range = upper_bound - lower_bound;
        if (range <= 0 || step <= 0 || total_cores_ == 0) {
            return tasks;
        }
        
        // Разделяем на подзадачи пропорционально общему количеству ядер
        // Создаем одну задачу на каждое ядро CPU для равномерного распределения нагрузки
        double task_range = range / total_cores_;
        double current_lower = lower_bound;
        size_t task_counter = 0;
        
        for (size_t i = 0; i < total_cores_ && current_lower < upper_bound; ++i) {
            double current_upper = (i == total_cores_ - 1) ? upper_bound : current_lower + task_range;
            
            IntegrationTask task;
            task.lower_bound = current_lower;
            task.upper_bound = current_upper;
            task.step = step;
            task.task_id = task_counter++;
            
            tasks.push_back(task);
            current_lower = current_upper;
        }
        
        return tasks;
    }

    /**
     * @brief Обрабатывает результат от клиента.
     * 
     * @param result Результат интегрирования.
     */
    void handle_result(const IntegrationResult& result) {
        std::lock_guard<std::mutex> lock(results_mutex_);
        
        results_[result.task_id] = result.result;
        results_received_++;
        
        LOG_INFO << "Получен результат для задачи " << result.task_id 
                 << " (получено: " << results_received_ << "/" << expected_results_ << ")";
        
        // Если получены все результаты, суммируем их
        if (results_received_ >= expected_results_) {
            final_result_ = 0.0;
            for (const auto& pair : results_) {
                final_result_ += pair.second;
            }
            results_ready_ = true;
            results_cv_.notify_one();
        }
    }

    /**
     * @brief Принимает новые подключения клиентов.
     */
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                if (!ec) {
                    next_client_id_++;
                    std::shared_ptr<ClientSession> new_session = 
                        std::make_shared<ClientSession>(std::move(socket), next_client_id_);
                    
                    {
                        std::lock_guard<std::mutex> lock(clients_mutex_);
                        clients_[next_client_id_] = new_session;
                    }
                    
                    new_session->start();
                    LOG_INFO << "Новое соединение от " << new_session->get_socket().remote_endpoint() 
                             << ", ID клиента: " << next_client_id_;
                } else {
                    LOG_ERROR << "Ошибка при установке соединения: " << ec.message();
                }
                do_accept();
            });
    }

    boost::asio::ip::tcp::acceptor acceptor_;
    std::map<size_t, std::shared_ptr<ClientSession>> clients_;
    std::mutex clients_mutex_;
    size_t next_client_id_;
    size_t next_task_id_;
    size_t total_cores_;

    // Синхронизация результатов
    std::map<size_t, double> results_;
    std::mutex results_mutex_;
    std::condition_variable results_cv_;
    std::atomic<size_t> results_received_;
    size_t expected_results_;
    double final_result_;
    bool results_ready_;
};

int main() {
    init_logging();
    LOG_INFO << "Приложение сервера запущено.";

    try {
        boost::asio::io_context io_context;
        Server server(io_context, 12345);

        // Запускаем io_context в отдельном потоке
        std::thread io_thread([&io_context]() {
            io_context.run();
        });

        // Даем время клиентам подключиться
        std::cout << "Ожидание подключения клиентов... (нажмите Enter для продолжения)" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cin.ignore();

        // Пример: запрашиваем у пользователя параметры интегрирования
        double lower_bound, upper_bound, step;
        std::cout << "Введите нижний предел интегрирования: ";
        std::cin >> lower_bound;
        std::cout << "Введите верхний предел интегрирования: ";
        std::cin >> upper_bound;
        std::cout << "Введите шаг интегрирования: ";
        std::cin >> step;

        double result = server.handle_integration_request(lower_bound, upper_bound, step);
        std::cout << "Результат интегрирования: " << result << std::endl;

        // Даем время для завершения операций
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        io_context.stop();
        io_thread.join();
    } catch (std::exception& e) {
        LOG_FATAL << "Исключение в приложении сервера: " << e.what();
    }

    LOG_INFO << "Приложение сервера завершено.";
    return 0;
}
