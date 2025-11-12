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

// 绘制变道轨迹（红色虚线）
// 平滑变道函数
bool Vehicle::smoothLaneChange(int laneHeight, const vector<Vehicle> &allVehicles)
{
    // 如果车辆已抛锚，不能变道
    if (isBrokenDown)
    {
        return false;
    }

    if (isChangingLane)
    {
        // 更新变道进度
        changeProgress += 0.02f;

        if (changeProgress >= 1.0f)
        {
            // 变道完成
            changeProgress = 1.0f;
            isChangingLane = false;
            lane = targetLane;
            return true;
        }

        // 使用固定的渐入渐出贝塞尔曲线计算垂直方向速度
        // y(t) = 3t² - 2t³，这是一个平滑的S型曲线，在t=0和t=1处导数为0
        float t = changeProgress;
        float verticalSpeed = 3 * t * t - 2 * t * t * t;

        // 计算垂直方向上的位置变化
        float deltaY = (endY - startY) * verticalSpeed;
        y = startY + (int)deltaY;

        // 水平方向保持原有速度
        // 注意：x的更新在moveForward函数中处理

        return false;
    }

    // 检查变道是否安全
    if (!isLaneChangeSafe(laneHeight, allVehicles))
    {
        return false; // 变道不安全，取消变道
    }

    // 确定目标车道
    if (lane == 0 || lane == 3)
    {
        targetLane = lane + 1;
    }
    else if (lane == 2 || lane == 5)
    {
        targetLane = lane - 1;
    }
    else if (lane == 1 || lane == 4)
    {
        targetLane = lane + (rand() % 2 ? 1 : -1);
    }

    // 计算变道参数
    startX = x;
    startY = y;
    endX = x + 50; // 向前移动50像素
    endY = laneHeight * targetLane + (int)(0.5 * laneHeight);

    // 开始变道
    isChangingLane = true;
    changeProgress = 0.0f;
    return false;
}

// 预测并绘制轨迹
void Vehicle::predictAndDrawTrajectory(int laneHeight, int middleY, int predictionSteps) const
{
    // 创建虚拟车辆
    VirtualVehicle virtualCar(x, y, carlength, carwidth);

    // 添加当前位置
    virtualCar.addTrajectoryPoint(x, y);

    // 预测未来轨迹
    int currentX = x;
    int currentY = y;
    int currentSpeed = (y < middleY) ? speed : -speed;

    // 如果正在变道，预测变道轨迹
    if (isChangingLane)
    {
        // 复制当前变道状态
        float currentProgress = changeProgress;
        int currentStartX = startX;
        int currentStartY = startY;
        int currentEndX = endX;
        int currentEndY = endY;

        // 预测变道轨迹
        for (int i = 1; i <= predictionSteps; ++i)
        {
            // 更新进度
            float t = min(1.0f, currentProgress + i * 0.02f);

            // 计算垂直位置
            float verticalSpeed = 3 * t * t - 2 * t * t * t;
            float deltaY = (currentEndY - currentStartY) * verticalSpeed;
            int newY = currentStartY + (int)deltaY;

            // 计算水平位置（保持原有速度）
            int newX = currentX + i * currentSpeed;

            // 添加到轨迹
            virtualCar.addTrajectoryPoint(newX, newY);
        }
    }
    else
    {
        // 预测直线行驶轨迹
        for (int i = 1; i <= predictionSteps; ++i)
        {
            int newX = currentX + i * currentSpeed;
            virtualCar.addTrajectoryPoint(newX, currentY);
        }
    }

    // 绘制轨迹
    virtualCar.drawTrajectory();
}

// 检查变道是否安全
bool Vehicle::isLaneChangeSafe(int laneHeight, const vector<Vehicle> &allVehicles) const
{
    // 如果已经变道或不在变道点，返回安全
    if (haschanged)
    {
        return true;
    }

    // 确定目标车道
    int target = 0;
    if (lane == 0 || lane == 3)
    {
        target = lane + 1;
    }
    else if (lane == 2 || lane == 5)
    {
        target = lane - 1;
    }
    else if (lane == 1 || lane == 4)
    {
        target = lane + (rand() % 2 ? 1 : -1);
    }

    // 创建虚拟车辆用于轨迹预测
    VirtualVehicle virtualCar(x, y, carlength, carwidth);

    // 添加当前位置
    virtualCar.addTrajectoryPoint(x, y);

    // 预测变道轨迹
    int currentX = x;
    int currentY = y;
    int targetY = laneHeight * target + (int)(0.5 * laneHeight);
    int currentSpeed = (y < laneHeight * 3) ? speed : -speed;

    // 预测变道轨迹
    for (int i = 1; i <= 30; ++i)
    {
        // 计算进度
        float t = min(1.0f, i * 0.02f);

        // 计算垂直位置
        float verticalSpeed = 3 * t * t - 2 * t * t * t;
        float deltaY = (targetY - currentY) * verticalSpeed;
        int newY = currentY + (int)deltaY;

        // 计算水平位置（保持原有速度）
        int newX = currentX + i * currentSpeed;

        // 添加到轨迹
        virtualCar.addTrajectoryPoint(newX, newY);
    }

    // 检查与其他车辆的轨迹是否相交
    for (const auto &other : allVehicles)
    {
        if (&other == this)
            continue; // 跳过自己

        // 为其他车辆创建虚拟车辆
        VirtualVehicle otherVirtual(other.x, other.y, other.carlength, other.carwidth);

        // 预测其他车辆的直线行驶轨迹
        int otherSpeed = (other.y < laneHeight * 3) ? other.speed : -other.speed;
        for (int i = 1; i <= 30; ++i)
        {
            int newX = other.x + i * otherSpeed;
            otherVirtual.addTrajectoryPoint(newX, other.y);
        }

        // 检查轨迹是否相交
        if (virtualCar.isTrajectoryIntersecting(otherVirtual, 30))
        {
            return false; // 轨迹相交，变道不安全
        }
    }

    return true; // 轨迹不相交，变道安全
}

// 检查与前车距离
void Vehicle::checkFrontVehicleDistance(vector<Vehicle> &allVehicles, int safeDistance)
{
    // 遍历所有车辆，寻找同一车道的前方车辆
    for (auto &other : allVehicles)
    {
        // 跳过自己
        if (&other == this)
            continue;

        // 检查是否在同一车道
        if (other.lane != lane)
            continue;

        // 判断方向：上方车道(0,1,2)向右行驶，下方车道(3,4,5)向左行驶
        bool isMovingRight = (lane < 3);

        // 检查是否为前方车辆
        bool isFrontVehicle = false;
        if (isMovingRight)
        {
            // 向右行驶，x坐标更大的车是前车
            isFrontVehicle = (other.x > x);
        }
        else
        {
            // 向左行驶，x坐标更小的车是前车
            isFrontVehicle = (other.x < x);
        }

        if (!isFrontVehicle)
            continue;

        // 计算两车之间的距离
        int distance = abs(other.x - x) - (other.carlength / 2 + carlength / 2);

        // 如果距离小于等于安全距离，进行进一步处理
        if (distance <= safeDistance)
        {
            showFlashingFrame();

            // 计算相对速度
            int relativeSpeed = abs(speed - other.speed);
            if (relativeSpeed != 0)
            {
                cout << "Relative Speed: " << relativeSpeed << endl;
            }
            // 如果车辆正在变道，则不处理
            if (isChangingLane)
                continue;

            // 根据相对速度采取不同措施
            if (relativeSpeed <= WAIT)
            {
                // 如果相对速度小于等于WAIT，将后车速度设为前车速度
                speed = other.speed;
                // 显示橘色线框
            }
            else if (relativeSpeed > WAIT && relativeSpeed <= CRASH)
            {
                // 如果相对速度大于WAIT小于CRASH，尝试变道
                bool laneChanged = smoothLaneChange(50, allVehicles);
                if (!laneChanged)
                {
                    // 如果无法变道，降低速度
                    speed = other.speed;
                }
            }
            else
            {
                // 如果相对速度大于CRASH，调用危险处理函数
                handleDangerousSituation(*this);
                handleDangerousSituation(other);
            }
            return; // 找到最近的前车后即可返回
        }
    }
}

// 显示橘色线框
void Vehicle::showFlashingFrame()
{
    // 保存当前线型和颜色
    LINESTYLE oldLineStyle;
    getlinestyle(&oldLineStyle);
    COLORREF oldLineColor = getlinecolor();

    // 设置橘色线框
    setlinecolor(RGB(255, 165, 0)); // 橙色
    setlinestyle(PS_SOLID, 2);      // 实线，线宽为2

    // 绘制车辆周围的橘色线框
    rectangle(x - carlength / 2 - 5, y - carwidth / 2 - 5,
              x + carlength / 2 + 5, y + carwidth / 2 + 5);

    // 恢复原来的线型和颜色
    setlinestyle(oldLineStyle.style, oldLineStyle.thickness);
    setlinecolor(oldLineColor);
}

// 处理危险情况
void Vehicle::handleDangerousSituation(Vehicle &self)
{
    // 设置车辆为抛锚状态
    self.isBrokenDown = true;
    // 将车辆速度设为0
    self.speed = 0;
}
