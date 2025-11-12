#include <graphics.h>
#include <vector>
#include <ctime>
#include <conio.h> // 需要包含此头文件_kbhit()函数需要
#include <Windows.h>
#include <sstream>
#include <string>
#include <iostream>

using namespace std;

// 定义车辆的类
struct Vehicle
{
    int lane, carlength, carwidth, x, y, speed, changeposition;
    bool haschanged;
    COLORREF color;

    // 新增成员变量用于变道
    bool isChangingLane;  // 是否正在变道
    int targetLane;       // 目标车道
    float changeProgress; // 变道进度 (0.0-1.0)
    int startX;           // 变道起始X坐标
    int startY;           // 变道起始Y坐标
    int endX;             // 变道结束X坐标
    int endY;             // 变道结束Y坐标
    
    // 距离警告相关成员
    bool isTooClose;      // 是否距离前车过近
    COLORREF originalColor; // 原始颜色
    void draw() const
    {
        setfillcolor(color); // 设置填充颜色
        setlinecolor(color); // 让边框也是同色
        fillrectangle(x - carlength / 2, y - carwidth / 2, x + carlength / 2, y + carwidth / 2);
    }
    // 绘制变道轨迹（红色虚线）
    void drawLaneChangePath() const;
    
    // 预测并绘制轨迹
    void predictAndDrawTrajectory(int laneHeight, int middleY, int predictionSteps = 30) const;
    
    // 检查变道是否安全
    bool isLaneChangeSafe(int laneHeight, const vector<Vehicle>& allVehicles) const;
    
    // 检查与前车距离
    void checkFrontVehicleDistance(const vector<Vehicle>& allVehicles, int safeDistance, void (*callback)(Vehicle&, const Vehicle&));
    // 前向运动函数
    void moveForward(int middleY)
    {
        x += (y < middleY) ? speed : -speed;
    }
    // 平滑变道函数
    bool smoothLaneChange(int laneHeight, const vector<Vehicle> &allVehicles);
};

// 虚拟车辆类，用于轨迹预测和相交检测
struct VirtualVehicle {
    int x, y;
    int carlength, carwidth;
    vector<pair<int, int>> trajectory; // 轨迹点集合
    
    VirtualVehicle(int startX, int startY, int length, int width) 
        : x(startX), y(startY), carlength(length), carwidth(width) {}
    
    // 添加轨迹点
    void addTrajectoryPoint(int pointX, int pointY) {
        trajectory.push_back(make_pair(pointX, pointY));
    }
    
    // 绘制轨迹（红色虚线）
    void drawTrajectory() const {
        if (trajectory.size() < 2) return;
        
        setlinecolor(RED);
        setlinestyle(PS_DASH, 1);
        
        for (size_t i = 1; i < trajectory.size(); ++i) {
            line(trajectory[i-1].first, trajectory[i-1].second, 
                 trajectory[i].first, trajectory[i].second);
        }
        
        setlinestyle(PS_SOLID, 1);
    }
    
    // 检查与另一车辆的轨迹是否相交
    bool isTrajectoryIntersecting(const VirtualVehicle& other, int futureSteps) const {
        // 检查当前和未来几个时间点的位置
        size_t checkSteps = min(futureSteps, min(trajectory.size(), other.trajectory.size()));
        
        for (size_t i = 0; i < checkSteps; ++i) {
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
                  myTop > otherBottom || myBottom < otherTop)) {
                return true; // 轨迹相交
            }
        }
        
        return false; // 轨迹不相交
    }
};

// 定义桥的类
struct Bridge
{
    double bridgeLength, bridgeWidth, widthScale;
    // 采用此函数计算适合屏幕的窗口尺寸和缩放比例，准备绘制桥梁、车道
    // 根据屏幕分辨率调整窗口大小
    void calculateWindowSize(int &windowWidth, int &windowHeight, double &scale) const
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
};
#pragma once
