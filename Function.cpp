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
using namespace std;

// 绘制虚线
void drawDashedLine(int x1, int y1, int x2, int y2)
{
    int dashLength = 10; // 虚线段长度
    int gapLength = 10;  // 空白段长度
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = max(abs(dx), abs(dy));
    float xIncrement = (float)dx / steps;
    float yIncrement = (float)dy / steps;

    for (int i = 0; i < steps; i += dashLength + gapLength)
    {
        int startX = round(x1 + i * xIncrement);
        int startY = round(y1 + i * yIncrement);
        int endX = round(startX + dashLength * xIncrement);
        int endY = round(startY + dashLength * yIncrement);

        if (endX > x2 || endY > y2)
            break; // 防止超出终点

        line(startX, startY, endX, endY); // 绘制虚线段
    }
}

// 绘制轨迹（红色虚线）
void VirtualVehicle::drawTrajectory() const
{
    if (trajectory.size() < 2)
        return;

    setlinecolor(RED);
    setlinestyle(PS_DASH, 1);

    for (size_t i = 1; i < trajectory.size(); ++i)
    {
        line(trajectory[i - 1].first, trajectory[i - 1].second,
             trajectory[i].first, trajectory[i].second);
    }

    setlinestyle(PS_SOLID, 1);
}
// 检查与另一车辆的轨迹是否相交
bool VirtualVehicle::isTrajectoryIntersecting(const VirtualVehicle &other, int futureSteps) const
{
    // 检查当前和未来几个时间点的位置
    size_t checkSteps = min(futureSteps, min(trajectory.size(), other.trajectory.size()));

    for (size_t i = 0; i < checkSteps; ++i)
    {
        // 获取当前时间点的位置
        int myX = i < trajectory.size() ? trajectory[i].first : x;
        int myY = i < trajectory.size() ? trajectory[i].second : y;

        // 获取另一车辆在对应时间点的位置
        int otherX = i < other.trajectory.size() ? other.trajectory[i].first : other.x;
        int otherY = i < other.trajectory.size() ? other.trajectory[i].second : other.y;

        // 检查两个矩形是否相交
        int myLeft = myX - carlength / 2;
        int myRight = myX + carlength / 2;
        int myTop = myY - carwidth / 2;
        int myBottom = myY + carwidth / 2;

        int otherLeft = otherX - other.carlength / 2;
        int otherRight = otherX + other.carlength / 2;
        int otherTop = otherY - other.carwidth / 2;
        int otherBottom = otherY + other.carwidth / 2;

        // 矩形相交检测
        if (!(myLeft > otherRight || myRight < otherLeft ||
              myTop > otherBottom || myBottom < otherTop))
        {
            return true; // 轨迹相交
        }
    }

    return false; // 轨迹不相交
}

// 根据屏幕分辨率调整窗口大小

void Bridge::calculateWindowSize(int &windowWidth, int &windowHeight, double &scale) const
{
    int margin = 100; // 边缘留白
    // 获取屏幕分辨率
    int maxscreenWidth = GetSystemMetrics(SM_CXSCREEN) - margin;
    int maxscreenHeight = GetSystemMetrics(SM_CYSCREEN) - margin;

    windowWidth = (int)bridgeLength;
    windowHeight = (int)(bridgeWidth * widthScale);
    // 确保窗口不超过屏幕分辨率
    double scaleX = static_cast<double>(maxscreenWidth) / windowWidth;
    double scaleY = static_cast<double>(maxscreenHeight) / windowHeight;
    double finalScaleFactor = min(scaleX, scaleY);

    scale = finalScaleFactor;
    windowWidth = int(windowWidth * finalScaleFactor);
    windowHeight = int(windowHeight * finalScaleFactor);

    initgraph(windowWidth, windowHeight);
}
    void Vehicle::draw() const
    {
        if (isBrokenDown)
        {
            // 抛锚车辆：灰色+红色X
            setfillcolor(RGB(100, 100, 100));
            setlinecolor(RGB(50, 50, 50));
            fillroundrect(x - carlength / 2, y - carwidth / 2, x + carlength / 2, y + carwidth / 2, 8, 8);

            setlinecolor(RED);
            setlinestyle(PS_SOLID, 3);
            line(x - carlength / 4, y - carwidth / 4, x + carlength / 4, y + carwidth / 4);
            line(x - carlength / 4, y + carwidth / 4, x + carlength / 4, y - carwidth / 4);
            setlinestyle(PS_SOLID, 1);
        }
        else
        {
            // 正常车辆
            setfillcolor(color); // 设置填充颜色
            setlinecolor(color); // 让边框也是同色
            fillrectangle(x - carlength / 2, y - carwidth / 2, x + carlength / 2, y + carwidth / 2);
        }

        // 在车辆上方显示速度
        wchar_t speedText[16];
        swprintf(speedText,16, L"%d", speed);
        setbkmode(TRANSPARENT);
        settextcolor(WHITE);
        settextstyle(20, 0, L"Arial");
        outtextxy(x - 10, y - carwidth / 2 - 25, speedText);
    }
