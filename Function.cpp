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
