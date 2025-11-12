#include "Class.h"
#include <algorithm>
#include <random>
using namespace std;

int main()
{
    Bridge bridge;
    cout << "请输入桥长（m）: ";
    cin >> bridge.bridgeLength;
    cout << "请输入桥宽（m）: ";
    cin >> bridge.bridgeWidth;
    cout << "请输入桥宽放大率: ";
    cin >> bridge.widthScale;

    int windowWidth, windowHeight;
    double scale;
    bridge.calculateWindowSize(windowWidth, windowHeight, scale);

    vector<Vehicle> vehicles;
    srand(unsigned int(time(0)));
    double simTime = 0;

    int laneCount = 6;
    int laneHeight = windowHeight / laneCount;

    // 正态分布用于尺寸扰动（米）
    normal_distribution<double> carLenDist(TrafficRules::CAR_LEN_MEAN, TrafficRules::CAR_LEN_STD);
    normal_distribution<double> carWidDist(TrafficRules::CAR_WID_MEAN, TrafficRules::CAR_WID_STD);
    normal_distribution<double> busLenDist(TrafficRules::BUS_LEN_MEAN, TrafficRules::BUS_LEN_STD);
    normal_distribution<double> busWidDist(TrafficRules::BUS_WID_MEAN, TrafficRules::BUS_WID_STD);
    normal_distribution<double> truckLenDist(TrafficRules::TRUCK_LEN_MEAN, TrafficRules::TRUCK_LEN_STD);
    normal_distribution<double> truckWidDist(TrafficRules::TRUCK_WID_MEAN, TrafficRules::TRUCK_WID_STD);

    // 修复点：准备一个持久的随机引擎，供所有分布复用
    std::random_device rd;
    std::mt19937 rng(rd());

    while (!_kbhit())
    {
        cleardevice();

        // 顶部信息
        wchar_t info[256];
        swprintf_s(info, L"桥长: %.0fm  桥宽: %.0fm  放大率: %.1f  时间: %.0fs",
                   bridge.bridgeLength, bridge.bridgeWidth, bridge.widthScale, simTime);
        settextstyle(20, 0, L"Arial");
        outtextxy(10, 10, info);

        int stalledCount = (int)count_if(vehicles.begin(), vehicles.end(),
                            [](const Vehicle& v){ return v.isStalled; });
        swprintf_s(info, L"抛锚: %d", stalledCount);
        outtextxy(windowWidth - 150, 10, info);

        // 画车道
        setlinecolor(WHITE);
        settextcolor(WHITE);
        for (int i = 0; i < laneCount - 1; ++i)
            drawDashedLine(0, (i + 1) * laneHeight, windowWidth, (i + 1) * laneHeight);
        for (int i = 0; i < laneCount; ++i)
        {
            settextstyle(laneHeight / 2, 0, L"Arial");
            outtextxy(5, laneHeight * i + laneHeight / 2 - laneHeight / 4, i < 3 ? L"→" : L"←");
        }

        // 生成新车（概率可调）
        if (rand() % 25 == 0)
        {
            // 决定车型
            double r = (rand() % 1000) / 1000.0;
            VehicleType type = VehicleType::Car;
            if (r < TrafficRules::SPAWN_PROB_CAR) type = VehicleType::Car;
            else if (r < TrafficRules::SPAWN_PROB_CAR + TrafficRules::SPAWN_PRO_BUS) type = VehicleType::Bus;
            else type = VehicleType::Truck;

            // 尺寸（米）
            double lenM = 4.5, widM = 1.8;
            switch (type)
            {
            case VehicleType::Car:
                lenM = max(3.6, carLenDist(rng));
                widM = max(1.5, carWidDist(rng));
                break;
            case VehicleType::Bus:
                lenM = max(8.0, busLenDist(rng));
                widM = max(2.2, busWidDist(rng));
                break;
            case VehicleType::Truck:
                lenM = max(9.0, truckLenDist(rng));
                widM = max(2.2, truckWidDist(rng));
                break;
            }

            // 速度（km/h）
            auto rand01 = [](){ return (rand() % 10000) / 10000.0; };
            double vmin = TrafficRules::CAR_MIN_KMH, vmax = TrafficRules::CAR_MAX_KMH;
            switch (type)
            {
            case VehicleType::Car:   vmin = TrafficRules::CAR_MIN_KMH;   vmax = TrafficRules::CAR_MAX_KMH;   break;
            case VehicleType::Bus:   vmin = TrafficRules::BUS_MIN_KMH;   vmax = TrafficRules::BUS_MAX_KMH;   break;
            case VehicleType::Truck: vmin = TrafficRules::TRUCK_MIN_KMH; vmax = TrafficRules::TRUCK_MAX_KMH; break;
            }
            double baseKmh = vmin + rand01() * (vmax - vmin);
            double variance = TrafficRules::SPEED_VARIANCE_MIN + rand01() * (TrafficRules::SPEED_VARIANCE_MAX - TrafficRules::SPEED_VARIANCE_MIN);
            double kmh = baseKmh * variance;

            // 违章（小概率超速至上限的1.3倍）
            bool violation = (rand() % 100000) < (int)(TrafficRules::VIOLATION_PROB * 100000.0);
            if (!violation) kmh = min(kmh, vmax);
            else kmh = min(kmh * 1.15, vmax * TrafficRules::VIOLATION_OVERSPEED_FACTOR); // 适度超速

            int pixelSpeed = max(1, (int)round(TrafficRules::kmhToPixelsPerFrame(kmh, scale)));

            // 尺寸（像素）
            int carlength = (int)round(lenM * scale);
            int carwidth  = (int)round(widM * scale * bridge.widthScale);

            // 起点、车道
            int lane = rand() % 6;
            int x0 = (lane < 3) ? 0 : windowWidth;
            int y0 = laneHeight * lane + laneHeight / 2;

            // 颜色（按车型倾向）
            COLORREF color = RGB(rand()%256, rand()%256, rand()%256);
            if (type == VehicleType::Bus)   color = RGB(60 + rand()%40, 120 + rand()%80, 220);
            if (type == VehicleType::Truck) color = RGB(180, 160, 120 + rand()%100);

            vehicles.push_back(Vehicle{
                lane,
                carlength,
                carwidth,
                x0,
                y0,
                pixelSpeed,
                pixelSpeed,
                kmh,
                type,
                color,
                false,          // isStalled
                false,          // isChangingLane
                lane,           // targetLane
                0.0,            // laneChangeProgress
                0,              // laneChangeStartY
                0,              // laneChangeEndY
                lane            // originalLane
            });
        }

        // === 核心逻辑：移动、变道、碰撞 ===

        // 1. 碰撞检测（在移动前）
        CollisionSystem::handleCollisions(vehicles, windowHeight);

        // 2. 更新车辆状态
        for (auto& v : vehicles)
        {
            if (v.isStalled) continue;

            // 变道决策、行为与速度调整请保留你当前实现（略）

            // 水平移动
            VehicleMovement::updatePosition(v, windowHeight);
            // 更新变道
            VehicleMovement::updateLaneChange(v, windowHeight);
        }

        // 再次碰撞检测
        CollisionSystem::handleCollisions(vehicles, windowHeight);

        // 移除离开车辆
        vehicles.erase(remove_if(vehicles.begin(), vehicles.end(),
            [windowWidth](const Vehicle& v){ return v.x < -50 || v.x > windowWidth + 50; }),
            vehicles.end());

        // 绘制
        for (const auto& v : vehicles) v.draw();

        Sleep(20);
        simTime += 0.02;
    }

    closegraph();
    return 0;
}
