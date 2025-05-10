#pragma once
#include <chrono>
#include <string>
#include <limits>
#include <vector>
#include <functional>
#include <ctime>
#include <cstdio>

namespace Cat {

/**
 * @brief 测试结果实体类
 * 
 * 存储单次测试的所有相关信息，包括：
 * - 测试名称和时间戳
 * - 性能指标（平均/最小/最大/总时间）
 * - 迭代次数
 */
class TestResult {
public:
    // 构造函数
    TestResult(std::string name, double avgTime, double minTime, 
              double maxTime, double totalTime, int iterations)
        : name_(std::move(name))
        , avgTime_(avgTime)
        , minTime_(minTime)
        , maxTime_(maxTime)
        , totalTime_(totalTime)
        , iterations_(iterations)
        , timestamp_(getCurrentTimestamp()) {}

    // 获取器
    const std::string& name() const { return name_; }
    double avgTime() const { return avgTime_; }
    double minTime() const { return minTime_; }
    double maxTime() const { return maxTime_; }
    double totalTime() const { return totalTime_; }
    int iterations() const { return iterations_; }
    const std::string& timestamp() const { return timestamp_; }

    /**
     * @brief 打印测试结果
     */
    void print() const {
        printf("\n=== Test Results ===\n");
        printf("Test: %s\n", name_.c_str());
        printf("Time: %s\n", timestamp_.c_str());
        printf("Iterations: %d\n", iterations_);
        printf("Average: %.3f ms\n", avgTime_);
        printf("Min: %.3f ms\n", minTime_);
        printf("Max: %.3f ms\n", maxTime_);
        printf("Total: %.3f ms\n", totalTime_);
        printf("===================\n\n");
    }

private:
    std::string name_;        // 测试名称
    double avgTime_;          // 平均执行时间（毫秒）
    double minTime_;          // 最小执行时间（毫秒）
    double maxTime_;          // 最大执行时间（毫秒）
    double totalTime_;        // 总执行时间（毫秒）
    int iterations_;          // 迭代次数
    std::string timestamp_;   // 测试执行时间戳

    /**
     * @brief 获取当前时间戳
     */
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_c));
        return std::string(buffer);
    }
};

/**
 * @brief 性能测试方法类
 * 
 * 提供性能测试的基础功能：
 * - 时间测量
 * - 多次迭代
 * - 结果统计
 * - 进度显示
 */
class PerformanceTest {
public:
    using TimePoint = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::nanoseconds;
    
    /**
     * @brief 运行性能测试
     * @param name 测试名称
     * @param iterations 迭代次数
     * @return 测试结果
     */
    TestResult runBenchmark(const std::string& name, int iterations = 100) {
        printf("\nRunning benchmark: %s\n", name.c_str());
        printf("Iterations: %d\n", iterations);
        
        std::vector<double> times;
        times.reserve(iterations);
        
        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::steady_clock::now();
            runTest();
            auto end = std::chrono::steady_clock::now();
            times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
            
            if ((i + 1) % 10 == 0) {
                printf("Progress: %d/%d\n", i + 1, iterations);
            }
        }
        
        return calculateResult(name, times, iterations);
    }
    
protected:
    /**
     * @brief 执行具体的测试内容
     */
    virtual void runTest() = 0;

private:
    /**
     * @brief 计算测试结果
     */
    static TestResult calculateResult(const std::string& name, 
                                    const std::vector<double>& times,
                                    int iterations) {
        double totalTime = 0;
        double minTime = std::numeric_limits<double>::max();
        double maxTime = 0;
        
        for (double time : times) {
            totalTime += time;
            minTime = std::min(minTime, time);
            maxTime = std::max(maxTime, time);
        }
        
        return TestResult(
            name,                           // 测试名称
            totalTime / iterations,         // 平均时间
            minTime,                        // 最小时间
            maxTime,                        // 最大时间
            totalTime,                      // 总时间
            iterations                      // 迭代次数
        );
    }
};

/**
 * @brief 函数式性能测试包装器
 */
class FunctionTest : public PerformanceTest {
public:
    /**
     * @brief 构造函数
     * @param testFunc 测试函数
     */
    explicit FunctionTest(std::function<void()> testFunc) 
        : testFunc_(std::move(testFunc)) {}
    
protected:
    void runTest() override {
        testFunc_();
    }
    
private:
    std::function<void()> testFunc_;
};

} // namespace Cat

// Cat::FunctionTest test([](){});
// test.runBenchmark("SumTest", 5);