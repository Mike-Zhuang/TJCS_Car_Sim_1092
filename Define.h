void drawDashedLine(int x1, int y1, int x2, int y2);
// 定义安全距离和回调函数
const int SAFE_DISTANCE = 50; // 安全距离（像素）
const int WAIT = 5;          // 等待速度差阈值
const int CRASH = 15;        // 危险速度差阈值
// 车辆长宽的分布，随机数取值
normal_distribution<> normalwidth(3, 0.1);  // 车的宽度  这里用了正态分布
normal_distribution<> normallength(6, 0.1); // 车辆长度  这里用了正态分布

#pragma once
