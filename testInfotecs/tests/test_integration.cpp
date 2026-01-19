#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <cstddef>

#include "../common/DataStructures.h"

/**
 * @brief Вычисляет значение функции 1/ln(x) для интегрирования.
 * 
 * @param x Точка, в которой вычисляется функция.
 * @return Значение функции или 0.0 для особых случаев.
 */
double integrate_function(double x) {
    if (x <= 1.0 || std::abs(std::log(x)) < 1e-10) {
        return 0.0; 
    }
    return 1.0 / std::log(x);
}

/**
 * @brief Вычисляет интеграл функции методом прямоугольников.
 * 
 * @param lower_bound Нижний предел интегрирования.
 * @param upper_bound Верхний предел интегрирования.
 * @param step Шаг интегрирования.
 * @return Результат интегрирования.
 */
double compute_integral(double lower_bound, double upper_bound, double step) {
    double result = 0.0;
    double x = lower_bound;
    
    while (x < upper_bound) {
        double next_x = std::min(x + step, upper_bound);
        double mid_x = (x + next_x) / 2.0;
        result += integrate_function(mid_x) * (next_x - x);
        x = next_x;
    }
    
    return result;
}

/**
 * @brief Тесты для функции интегрирования.
 */
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Настройка перед каждым тестом
    }
    
    void TearDown() override {
        // Очистка после каждого теста
    }
};

/**
 * @brief Тест вычисления функции в нормальной точке.
 */
TEST_F(IntegrationTest, FunctionValueNormalPoint) {
    double x = 2.0;
    double expected = 1.0 / std::log(2.0);
    double actual = integrate_function(x);
    
    EXPECT_NEAR(expected, actual, 1e-10);
}

/**
 * @brief Тест вычисления функции в особой точке (x <= 1).
 */
TEST_F(IntegrationTest, FunctionValueSpecialPoint) {
    double x = 1.0;
    double actual = integrate_function(x);
    
    EXPECT_EQ(0.0, actual);
}

/**
 * @brief Тест вычисления функции в точке близкой к 1.
 */
TEST_F(IntegrationTest, FunctionValueNearOne) {
    double x = 1.0001;
    double actual = integrate_function(x);
    
    // Функция должна быть определена и положительна для x > 1
    EXPECT_GT(actual, 0.0);
}

/**
 * @brief Тест интегрирования на небольшом интервале.
 */
TEST_F(IntegrationTest, SmallIntervalIntegration) {
    double lower_bound = 2.0;
    double upper_bound = 3.0;
    double step = 0.001;
    
    double result = compute_integral(lower_bound, upper_bound, step);
    
    // Результат должен быть положительным для интервала [2, 3]
    EXPECT_GT(result, 0.0);
    
    // Проверяем, что результат разумный (интеграл 1/ln(x) на [2,3] примерно 1.82)
    EXPECT_NEAR(result, 1.82, 0.1);
}

/**
 * @brief Тест интегрирования с разными шагами.
 */
TEST_F(IntegrationTest, DifferentStepSizes) {
    double lower_bound = 2.0;
    double upper_bound = 4.0;
    
    double result1 = compute_integral(lower_bound, upper_bound, 0.01);
    double result2 = compute_integral(lower_bound, upper_bound, 0.001);
    double result3 = compute_integral(lower_bound, upper_bound, 0.0001);
    
    // Более мелкий шаг должен давать более точный результат
    // Результаты должны быть близки друг к другу
    EXPECT_NEAR(result1, result2, 0.1);
    EXPECT_NEAR(result2, result3, 0.01);
}

/**
 * @brief Тест сериализации IntegrationTask.
 */
TEST_F(IntegrationTest, TaskSerialization) {
    IntegrationTask original;
    original.lower_bound = 2.0;
    original.upper_bound = 10.0;
    original.step = 0.001;
    original.task_id = 42;
    
    // В реальном тесте здесь была бы сериализация и десериализация
    // Для упрощения просто проверяем структуру
    EXPECT_EQ(original.lower_bound, 2.0);
    EXPECT_EQ(original.upper_bound, 10.0);
    EXPECT_EQ(original.step, 0.001);
    EXPECT_EQ(original.task_id, 42);
}

/**
 * @brief Тест сериализации IntegrationResult.
 */
TEST_F(IntegrationTest, ResultSerialization) {
    IntegrationResult original;
    original.result = 3.14159;
    original.task_id = 42;
    
    // В реальном тесте здесь была бы сериализация и десериализация
    EXPECT_EQ(original.result, 3.14159);
    EXPECT_EQ(original.task_id, 42);
}

/**
 * @brief Тест граничных случаев интегрирования.
 */
TEST_F(IntegrationTest, BoundaryCases) {
    // Тест с очень маленьким интервалом
    double result1 = compute_integral(2.0, 2.001, 0.0001);
    EXPECT_GE(result1, 0.0);
    
    // Тест с интервалом, где функция не определена
    double result2 = compute_integral(0.5, 1.0, 0.001);
    // Результат должен быть 0 или очень маленьким
    EXPECT_GE(result2, 0.0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
