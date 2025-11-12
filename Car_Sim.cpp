#include <graphics.h>
#include <vector>
#include <ctime>
#include <conio.h> // 需要包含此头文件_kbhit()函数需要
#include <Windows.h>
#include <sstream>
#include <string>
#include <iostream>

#include "Random.h"
#include "Class.h"
#include "Define.h"
using namespace std;
int main()
{

    Bridge bridge;
    // 输入桥梁参数
    //cout << "请输入桥长（m）: ";
    //cin >> bridge.bridgeLength;
    //cout << "请输入桥宽（m）: ";
    //cin >> bridge.bridgeWidth;
    //cout << "请输入桥宽放大率: ";
    //cin >> bridge.widthScale;
    bridge.bridgeLength = 100;
    bridge.bridgeWidth = 50;
    bridge.widthScale = 1;
    // 调用桥梁绘制函数计算窗口大小
    int windowWidth, windowHeight;
    double scale;
    bridge.calculateWindowSize(windowWidth, windowHeight, scale);

    vector<Vehicle> vehicles;
    srand(unsigned int(time(0)));
    double time = 0;
    // 车辆长宽的分布，随机数取值
    normal_distribution<> normalwidth(3, 0.1);  // 车的宽度  这里用了正态分布
    normal_distribution<> normallength(6, 0.1); // 车辆长度  这里用了正态分布
    uniform_int_distribution<int> int_dist(20, 120);
    RandomGenerator rng(int_dist);
    while (!_kbhit())
    {
        cleardevice();
        // 显示桥的参数信息
        wchar_t info[256];
        swprintf_s(info, L"桥长： %.0fm  桥宽：%.0fm  桥宽放大率： %.1f", bridge.bridgeLength, bridge.bridgeWidth, bridge.widthScale);
        settextstyle(20, 0, L"Arial");
        outtextxy(10, 10, info);
        // 显示时间
        wchar_t info2[256];
        swprintf_s(info2, L"时间： %.0fs", time);
        settextstyle(20, 0, L"Arial");
        outtextxy(windowWidth - 150, 10, info2);

        // 绘制车道
        setlinecolor(WHITE);                              // 设置线条为白色
        settextcolor(WHITE);                              // 设置文字为白色
        int laneCount = 6;                                // 车道数量
        int laneHeight = (int)(windowHeight / laneCount); // 车道像素宽度
        for (int i = 0; i < laneCount - 1; ++i)
        {
            drawDashedLine(0, (i + 1) * laneHeight, windowWidth, (i + 1) * laneHeight);
        }
        // 绘制箭头
        for (int i = 0; i < laneCount; ++i)
        {
            settextstyle((int)(laneHeight / 2), 0, L"Arial");
            outtextxy(5, laneHeight * i + (int)(0.5 * laneHeight) - (int)(laneHeight / 4), i < laneCount / 2 ? L"→" : L"←");
        }

        // 生成新车
        if (rand() % 10 == 0)
        {                          // 判断要不要产生新的一辆车
            int lane = rand() % 6; // 如果有车，车辆的随机位置

            // 随机的车辆长与宽
            int carwidth = RandomGenerator{normalwidth}() * scale * bridge.widthScale;
            int carlength = RandomGenerator{normallength}() * scale;

            vehicles.push_back(Vehicle{
                // 桥梁上所有车子存放在vehicles里面，
                // 添加行车
                lane,
                carlength,
                carwidth,
                lane < 3 ? 0 : windowWidth,
                laneHeight * lane + (int)(0.5 * laneHeight),
                (int)rng.generate(),
                false,
                RGB(rand() % 256, rand() % 256, rand() % 256),
                false,                                         // isChangingLane
                0,                                             // targetLane
                0.0f,                                          // changeProgress
                0,                                             // startX
                0,                                             // startY
                0,                                             // endX
                0,                                             // endY
                false,                                         // isTooClose
                RGB(rand() % 256, rand() % 256, rand() % 256), // originalColor
                false                                          // isBrokenDown
            });
        }

        // 更新车辆的位置
        int middleY = windowHeight / 2; // 桥面中心的位置

        for (auto &v : vehicles)
        {
            // 保存原始颜色（如果还没有被标记为警告）
            COLORREF originalColor = v.color;

            // 使用前向运动函数
            v.moveForward(middleY);

            // 检查与前车距离
            v.checkFrontVehicleDistance(vehicles, SAFE_DISTANCE);

            // 如果处于警告状态，检查是否需要恢复
            if (v.isTooClose)
            {
                // 检查当前是否仍然距离过近
                bool stillTooClose = false;
                for (const auto &other : vehicles)
                {
                    if (&other == &v)
                        continue;
                    if (other.lane != v.lane)
                        continue;

                    bool isMovingRight = (v.lane < 3);
                    bool isFrontVehicle = isMovingRight ? (other.x > v.x) : (other.x < v.x);

                    if (isFrontVehicle)
                    {
                        int distance = abs(other.x - v.x) - (other.carlength / 2 + v.carlength / 2);
                        if (distance <= SAFE_DISTANCE)
                        {
                            stillTooClose = true;
                            break;
                        }
                    }
                }

                if (!stillTooClose)
                {
                    // 恢复原始颜色
                    v.color = v.originalColor;
                    v.isTooClose = false;
                }
            }
        }
        // 移除离开车辆
        vehicles.erase(remove_if(vehicles.begin(), vehicles.end(),
                                 [windowWidth](const Vehicle &v)
                                 { return v.x < 0 || v.x > windowWidth; }),
                       vehicles.end()); // remove_if:遍历所有车辆，将不需要删除的车辆移至前方，
        // 将需要删除的移至后方，返回一个分界点值，erase删除从分界点到末尾的值

        // 绘制车辆
        for (const auto &v : vehicles)
        {
            v.predictAndDrawTrajectory(laneHeight, windowHeight / 2); // 预测并绘制轨迹
            v.draw();                                                 // 绘制车辆
        }

        Sleep(60); // ms
        time += 0.2;
    }
    closegraph();
    return 0;
}
