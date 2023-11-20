#include <stdio.h>
#include <stdlib.h>
#include <math.h>

double generate_normal_distribution(double mean, double std_dev) {
    double u1, u2;
    double z;

    // 生成两个均匀分布的随机数 u1, u2 在 (0, 1) 之间
    u1 = rand() / (RAND_MAX + 1.0);
    u2 = rand() / (RAND_MAX + 1.0);

    // 利用 Box-Muller 转换生成正态分布的随机数
    z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);

    // 根据均值和标准差进行缩放和平移
    double value = mean + std_dev * z;

    // 将生成的值映射到 0 到 1 的范围内
    if (value < 0.0) {
        return 0.0;
    } else if (value > 1.0) {
        return 1.0;
    } else {
        return value;
    }
}

int main() {
    // 设置均值和标准差
    double mean = 0.65;
    double std_dev = 0.1;  // 标准差为1，可以根据需要调整

    // 设置随机数种子
    srand(999);

    // 生成并输出正态分布的随机数
    for (int i = 0; i < 100; ++i) {
        double value = generate_normal_distribution(mean, std_dev);
        printf("%f\n", value);
    }

    return 0;
}
