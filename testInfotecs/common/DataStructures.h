#pragma once

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <vector>
#include <string>
#include <cstddef>

/**
 * @brief Структура, представляющая задачу интегрирования.
 * 
 * Содержит нижний предел, верхний предел и шаг интегрирования.
 */
struct IntegrationTask {
    double lower_bound; ///< Нижний предел интегрирования
    double upper_bound; ///< Верхний предел интегрирования
    double step;        ///< Шаг интегрирования
    size_t task_id;     ///< Идентификатор задачи

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & lower_bound;
        ar & upper_bound;
        ar & step;
        ar & task_id;
    }
};

/**
 * @brief Структура, представляющая результат интегрирования.
 * 
 * Содержит вычисленное значение интеграла и идентификатор задачи.
 */
struct IntegrationResult {
    double result;      ///< Вычисленное значение интеграла
    size_t task_id;     ///< Идентификатор задачи

    template<class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar & result;
        ar & task_id;
    }
};
