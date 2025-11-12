#ifndef CLASS_H
#define CLASS_H

#include <graphics.h>
#include <vector>
#include <ctime>
#include <conio.h>
#include <Windows.h>
#include <iostream>
#include <random>
#include <functional>
using namespace std;

// ==================== 交通规则常量（符合实际情况）====================
namespace TrafficRules
{
    // 全局帧率
    const double FRAME_RATE = 50.0; // Sleep(20) ≈ 50FPS

    // 速度阈值（km/h）
    const double FOLLOW_SPEED_DIFF = 5.0;    // ≤5 跟随
    const double OVERTAKE_SPEED_DIFF = 15.0; // 5~15 超车
    const double CRASH_SPEED_DIFF = 30.0;    // >30 易撞

    // 跟随时间头距
    const double SAFE_FOLLOW_TIME = 2.0; // s

    // 车型出现概率（总和=1）
    const double SPAWN_PROB_CAR   = 0.65;
    const double SPAWN_PRO_BUS    = 0.15;
    const double SPAWN_PRO_TRUCK  = 0.20;

    // 车型限速（km/h）
    const double CAR_MIN_KMH   = 110.0, CAR_MAX_KMH   = 120.0;
    const double BUS_MIN_KMH   =  90.0, BUS_MAX_KMH   = 120.0;
    const double TRUCK_MIN_KMH =  60.0, TRUCK_MAX_KMH =  90.0;

    // 速度波动（±20%）
    const double SPEED_VARIANCE_MIN = 0.8;
    const double SPEED_VARIANCE_MAX = 1.2;

    // 违章车辆（极小概率超速到上限的1.3倍）
    const double VIOLATION_PROB = 0.002; // 0.2%
    const double VIOLATION_OVERSPEED_FACTOR = 1.30;

    // 尺寸（单位：米），均值与方差（标准差）
    // 小客车
    const double CAR_LEN_MEAN = 4.5,  CAR_LEN_STD = 0.4;
    const double CAR_WID_MEAN = 1.8,  CAR_WID_STD = 0.15;
    // 客车（公交/大客）
    const double BUS_LEN_MEAN = 12.0, BUS_LEN_STD = 0.8;
    const double BUS_WID_MEAN = 2.5,  BUS_WID_STD = 0.1;
    // 客货车（卡车/牵引）
    const double TRUCK_LEN_MEAN = 16.0, TRUCK_LEN_STD = 1.5;
    const double TRUCK_WID_MEAN = 2.5,  TRUCK_WID_STD = 0.1;

    // km/h -> 像素/帧（scale：米->像素）
    inline double kmhToPixelsPerFrame(double kmh, double scale)
    {
        // 1 km/h ≈ 0.278 m/s
        return (kmh * 0.278 * scale) / FRAME_RATE;
    }
}

// ==================== 随机数生成器 ====================
class RandomGenerator
{
private:
    mt19937 gen;
    function<double()> generator;
public:
    template <typename Distribution>
    RandomGenerator(Distribution& dist)
    {
        random_device rd;
        gen.seed(rd());
        generator = [&](){ return dist(gen); };
    }
    double operator()(){ return generator(); }
};

// ============== 车型 ==============
enum class VehicleType
{
    Car,
    Bus,
    Truck
};

// ==================== 车辆结构体 ====================
struct Vehicle
{
    // 基本属性
    int lane;
    int carlength, carwidth; // 像素
    int x, y;
    int speed;               // 像素/帧
    int originalSpeed;       // 初始像素/帧
    double speedKmh;         // km/h（用于显示/判断）
    VehicleType type;        // 车型
    COLORREF color;

    // 状态标记
    bool isStalled;

    // 变道相关
    bool isChangingLane;
    int  targetLane;
    double laneChangeProgress;
    int  laneChangeStartY;
    int  laneChangeEndY;
    int  originalLane;

    // 绘制方法
    void draw() const;
    void getBounds(int& left, int& right, int& top, int& bottom) const;
};

// ==================== 桥梁结构体 ====================
struct Bridge
{
    double bridgeLength, bridgeWidth, widthScale;
    void calculateWindowSize(int& windowWidth, int& windowHeight, double& scale) const;
};

// ==================== 核心功能模块声明 ====================

// 模块1：车辆移动管理
namespace VehicleMovement
{
    void updatePosition(Vehicle& v, int windowHeight);
    void updateLaneChange(Vehicle& v, int windowHeight);
}

// 模块2：碰撞检测系统
namespace CollisionSystem
{
    bool checkCollision(const Vehicle& a, const Vehicle& b);
    bool checkWillCollide(const Vehicle& front, const Vehicle& back, int windowHeight);
    void handleCollisions(vector<Vehicle>& vehicles, int windowHeight);
}

// 模块3：变道决策与执行
namespace LaneChangeSystem
{
    bool requestLaneChange(Vehicle& v, int targetLane, int laneHeight, 
                          int windowHeight, const vector<Vehicle>& vehicles);
    
    int decideTargetLane(const Vehicle& v, int windowHeight, int laneHeight,
                        const vector<Vehicle>& vehicles);
    
    bool isLaneChangeSafe(const Vehicle& v, int targetLane, int laneHeight,
                         int windowHeight, const vector<Vehicle>& vehicles);
}

// === 模块4：车辆行为管理 ===
namespace VehicleBehavior
{
    enum class BehaviorType { FREE_DRIVING, FOLLOWING, OVERTAKING, EMERGENCY_BRAKE };

    BehaviorType decideBehavior(const Vehicle& back, const Vehicle& front, 
                               int windowHeight, double& speedDiff);
    void adjustSpeed(Vehicle& v, BehaviorType behavior, const Vehicle* frontVehicle,
                    double speedDiff, int distance);
    double calculateSpeedDiff(const Vehicle& back, const Vehicle& front);
    int    calculateSafeFollowDistance(const Vehicle& v);
}

// 模块5：工具函数
void drawDashedLine(int x1, int y1, int x2, int y2);
double bezierCurve(double t, double p0, double p1, double p2, double p3);

#endif // CLASS_H

// ==================== 内联实现 ====================
inline void Vehicle::draw() const
{
    int left = x - carlength / 2;
    int right = x + carlength / 2;
    int top = y - carwidth / 2;
    int bottom = y + carwidth / 2;

    if (isStalled)
    {
        setfillcolor(RGB(100, 100, 100));
        setlinecolor(RGB(50, 50, 50));
        fillroundrect(left, top, right, bottom, 8, 8);
        setlinecolor(RED);
        setlinestyle(PS_SOLID, 3);
        line(left + carlength/4, top + carwidth/4, right - carlength/4, bottom - carwidth/4);
        line(left + carlength/4, bottom - carwidth/4, right - carlength/4, top + carwidth/4);
        setlinestyle(PS_SOLID, 1);
        return;
    }

    // 停车高亮
    if (speed == 0)
    {
        setlinecolor(RGB(255, 128, 0));
        setlinestyle(PS_SOLID, 3);
        rectangle(left - 3, top - 3, right + 3, bottom + 3);
        setlinestyle(PS_SOLID, 1);
    }

    switch (type)
    {
    case VehicleType::Car:
    {
        // 车身 + 阴影
        setfillcolor(RGB(50, 50, 50));
        fillroundrect(left + 2, top + 2, right + 2, bottom + 2, 6, 6);

        setfillcolor(color);
        setlinecolor(RGB(30, 30, 30));
        setlinestyle(PS_SOLID, 2);
        fillroundrect(left, top, right, bottom, 6, 6);

        // 前后窗
        setfillcolor(RGB(150, 200, 230));
        setlinecolor(RGB(80, 80, 80));
        int wm = carlength / 8, wh = carwidth / 3;
        fillrectangle(right - wm - carlength/6, top + wh, right - wm, bottom - wh);
        fillrectangle(left + wm, top + wh, left + wm + carlength/6, bottom - wh);

        // 车轮
        setfillcolor(BLACK);
        int wr = max(2, carwidth / 5), wo = max(4, carlength / 4);
        fillcircle(right - wo, top, wr);
        fillcircle(left + wo, top, wr);
        fillcircle(right - wo, bottom, wr);
        fillcircle(left + wo, bottom, wr);

        // 轮毂
        setfillcolor(RGB(180, 180, 180));
        int hr = max(1, wr / 2);
        fillcircle(right - wo, top, hr);
        fillcircle(left + wo, top, hr);
        fillcircle(right - wo, bottom, hr);
        fillcircle(left + wo, bottom, hr);
        break;
    }
    case VehicleType::Bus:
    {
        // 车身（更高更长）
        setfillcolor(color);
        setlinecolor(RGB(30, 30, 30));
        fillroundrect(left, top, right, bottom, 8, 8);

        // 侧窗带
        setfillcolor(RGB(180, 220, 240));
        int bandTop = top + carwidth / 5;
        int bandBot = bottom - carwidth / 5;
        fillrectangle(left + carlength/10, bandTop, right - carlength/10, bandBot);

        // 分隔窗格
        setlinecolor(RGB(120, 120, 120));
        int windows = max(4, carlength / 40);
        for (int i = 1; i < windows; ++i)
        {
            int wx = left + carlength/10 + i * (right - left - carlength/5) / windows;
            line(wx, bandTop, wx, bandBot);
        }

        // 车轮（较大）
        setfillcolor(BLACK);
        int wr = max(3, carwidth / 4);
        int w1x = left + carlength / 5;
        int w2x = right - carlength / 5;
        fillcircle(w1x, bottom, wr);
        fillcircle(w2x, bottom, wr);
        setfillcolor(RGB(180, 180, 180));
        fillcircle(w1x, bottom, wr/2);
        fillcircle(w2x, bottom, wr/2);
        break;
    }
    case VehicleType::Truck:
    {
        // 拖挂 + 车头
        int cabLen = max(10, carlength / 4);
        int trailerLeft = left;
        int trailerRight = right - cabLen;

        // 货厢
        setfillcolor(color);
        setlinecolor(RGB(30, 30, 30));
        fillrectangle(trailerLeft, top, trailerRight, bottom);

        // 车头
        setfillcolor(RGB(200, 200, 200));
        fillroundrect(trailerRight, top, right, bottom, 6, 6);

        // 货厢竖筋
        setlinecolor(RGB(100, 100, 100));
        int ribs = max(3, (trailerRight - trailerLeft) / 30);
        for (int i = 1; i < ribs; ++i)
        {
            int rx = trailerLeft + i * (trailerRight - trailerLeft) / ribs;
            line(rx, top, rx, bottom);
        }

        // 车轮（多轴）
        setfillcolor(BLACK);
        int wr = max(3, carwidth / 4);
        int ax1 = trailerLeft + (trailerRight - trailerLeft) * 2 / 3;
        int ax2 = trailerLeft + (trailerRight - trailerLeft) * 4 / 5;
        fillcircle(ax1, bottom, wr);
        fillcircle(ax2, bottom, wr);
        fillcircle(right - cabLen/2, bottom, wr - 1);
        break;
    }
    }

    setlinestyle(PS_SOLID, 1);
}

inline void Vehicle::getBounds(int& left, int& right, int& top, int& bottom) const
{
    left = x - carlength / 2;
    right = x + carlength / 2;
    top = y - carwidth / 2;
    bottom = y + carwidth / 2;
}

inline void Bridge::calculateWindowSize(int& windowWidth, int& windowHeight, double& scale) const
{
    int margin = 100;
    int maxW = GetSystemMetrics(SM_CXSCREEN) - margin;
    int maxH = GetSystemMetrics(SM_CYSCREEN) - margin;

    windowWidth = (int)bridgeLength;
    windowHeight = (int)(bridgeWidth * widthScale);

    double scaleX = (double)maxW / windowWidth;
    double scaleY = (double)maxH / windowHeight;
    scale = min(scaleX, scaleY);

    windowWidth = (int)(windowWidth * scale);
    windowHeight = (int)(windowHeight * scale);

    initgraph(windowWidth, windowHeight);
}
