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

// 绘制变道轨迹（红色虚线）
void Vehicle::drawLaneChangePath() const
{
    if (isChangingLane)
    {
        setlinecolor(RED);
        setlinestyle(PS_DASH, 1);
        
        // 使用固定的渐入渐出贝塞尔曲线绘制轨迹
        int prevX = startX;
        int prevY = startY;
        
        for (float t = 0; t <= 1; t += 0.05)
        {
            // 使用相同的S型曲线计算垂直位置
            float verticalSpeed = 3 * t * t - 2 * t * t * t;
            float deltaY = (endY - startY) * verticalSpeed;
            int currentY = startY + (int)deltaY;
            
            // 水平方向是匀速的，所以x位置是线性变化的
            int currentX = startX + (int)((endX - startX) * t);
            
            if (t > 0)
            {
                line(prevX, prevY, currentX, currentY);
            }
            
            prevX = currentX;
            prevY = currentY;
        }
        
        setlinestyle(PS_SOLID, 1);
    }
}
// 平滑变道函数
bool Vehicle::smoothLaneChange(int laneHeight, const vector<Vehicle> &allVehicles)
{
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

    // 检查是否需要开始变道
    if (!haschanged &&
        ((y < laneHeight * 3 && x >= changeposition) ||
         (y > laneHeight * 3 && x <= changeposition)))
    {
        // 检查变道是否安全
        if (!isLaneChangeSafe(laneHeight, allVehicles)) {
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

    return false;
}

// 预测并绘制轨迹
void Vehicle::predictAndDrawTrajectory(int laneHeight, int middleY, int predictionSteps) const {
    // 创建虚拟车辆
    VirtualVehicle virtualCar(x, y, carlength, carwidth);
    
    // 添加当前位置
    virtualCar.addTrajectoryPoint(x, y);
    
    // 预测未来轨迹
    int currentX = x;
    int currentY = y;
    int currentSpeed = (y < middleY) ? speed : -speed;
    
    // 如果正在变道，预测变道轨迹
    if (isChangingLane) {
        // 复制当前变道状态
        float currentProgress = changeProgress;
        int currentStartX = startX;
        int currentStartY = startY;
        int currentEndX = endX;
        int currentEndY = endY;
        
        // 预测变道轨迹
        for (int i = 1; i <= predictionSteps; ++i) {
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
    } else {
        // 预测直线行驶轨迹
        for (int i = 1; i <= predictionSteps; ++i) {
            int newX = currentX + i * currentSpeed;
            virtualCar.addTrajectoryPoint(newX, currentY);
        }
    }
    
    // 绘制轨迹
    virtualCar.drawTrajectory();
}

// 检查变道是否安全
bool Vehicle::isLaneChangeSafe(int laneHeight, const vector<Vehicle>& allVehicles) const {
    // 如果已经变道或不在变道点，返回安全
    if (haschanged || 
        ((y < laneHeight * 3 && x < changeposition) || 
         (y > laneHeight * 3 && x > changeposition))) {
        return true;
    }
    
    // 确定目标车道
    int target = 0;
    if (lane == 0 || lane == 3) {
        target = lane + 1;
    } else if (lane == 2 || lane == 5) {
        target = lane - 1;
    } else if (lane == 1 || lane == 4) {
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
    for (int i = 1; i <= 30; ++i) {
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
    for (const auto& other : allVehicles) {
        if (&other == this) continue; // 跳过自己
        
        // 为其他车辆创建虚拟车辆
        VirtualVehicle otherVirtual(other.x, other.y, other.carlength, other.carwidth);
        
        // 预测其他车辆的直线行驶轨迹
        int otherSpeed = (other.y < laneHeight * 3) ? other.speed : -other.speed;
        for (int i = 1; i <= 30; ++i) {
            int newX = other.x + i * otherSpeed;
            otherVirtual.addTrajectoryPoint(newX, other.y);
        }
        
        // 检查轨迹是否相交
        if (virtualCar.isTrajectoryIntersecting(otherVirtual, 30)) {
            return false; // 轨迹相交，变道不安全
        }
    }
    
    return true; // 轨迹不相交，变道安全
}

// 检查与前车距离
void Vehicle::checkFrontVehicleDistance(const vector<Vehicle>& allVehicles, int safeDistance, void (*callback)(Vehicle&, const Vehicle&)) {
    // 遍历所有车辆，寻找同一车道的前方车辆
    for (const auto& other : allVehicles) {
        // 跳过自己
        if (&other == this) continue;
        
        // 检查是否在同一车道
        if (other.lane != lane) continue;
        
        // 判断方向：上方车道(0,1,2)向右行驶，下方车道(3,4,5)向左行驶
        bool isMovingRight = (lane < 3);
        
        // 检查是否为前方车辆
        bool isFrontVehicle = false;
        if (isMovingRight) {
            // 向右行驶，x坐标更大的车是前车
            isFrontVehicle = (other.x > x);
        } else {
            // 向左行驶，x坐标更小的车是前车
            isFrontVehicle = (other.x < x);
        }
        
        if (!isFrontVehicle) continue;
        
        // 计算两车之间的距离
        int distance = abs(other.x - x) - (other.carlength / 2 + carlength / 2);
        
        // 如果距离小于等于安全距离，调用回调函数
        if (distance <= safeDistance) {
            callback(*this, other);
            return; // 找到最近的前车后即可返回
        }
    }
}
