#include <random>
#include <functional>
using namespace std;

class RandomGenerator
{
private:
    mt19937 gen;                  // 随机数引擎
    function<double()> generator; // 生成函数

public:
    // 模板构造函数：接受任何分布类型
    template <typename Distribution>
    RandomGenerator(Distribution &dist)
    {
        random_device rd;
        gen.seed(rd()); // 初始化引擎

        // 创建生成函数，绑定分布和引擎
        generator = [&]()
        { return dist(gen); };
    }
    // 生成随机数
    double generate()
    {
        return generator();
    }
    // 重载函数调用操作符
    double operator()()
    {
        return generate();
    }
};

#pragma once
