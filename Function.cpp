#include "Class.h"
#include <cmath>
using namespace std;

// ==================== 工具函数 ====================

void drawDashedLine(int x1, int y1, int x2, int y2)
{
    int dashLen = 10, gapLen = 10;
    int dx = x2 - x1, dy = y2 - y1;
    int steps = max(abs(dx), abs(dy));
    if (steps == 0) return;
    
    float xi = (float)dx / steps, yi = (float)dy / steps;
    for (int i = 0; i < steps; i += dashLen + gapLen)
    {
        int sx = (int)(x1 + i * xi), sy = (int)(y1 + i * yi);
        int ex = (int)(sx + dashLen * xi), ey = (int)(sy + dashLen * yi);
        if (ex > x2 || ey > y2) break;
        line(sx, sy, ex, ey);
    }
}

double bezierCurve(double t, double p0, double p1, double p2, double p3)
{
    double u = 1.0 - t;
    return u*u*u*p0 + 3*u*u*t*p1 + 3*u*t*t*p2 + t*t*t*p3;
}

// ==================== 模块1：车辆移动管理 ====================

namespace VehicleMovement
{
    void updatePosition(Vehicle& v, int windowHeight)
    {
        if (v.isStalled) return;
        
        int middleY = windowHeight / 2;
        bool isUpperLane = v.y < middleY;
        
        // 水平移动
        v.x += isUpperLane ? v.speed : -v.speed;
    }
    
    void updateLaneChange(Vehicle& v, int windowHeight)
    {
        if (!v.isChangingLane) return;
        
        // 每帧进度增量（约30帧完成变道）
        v.laneChangeProgress += 1.0 / 30.0;
        
        if (v.laneChangeProgress >= 1.0)
        {
            // 变道完成
            v.y = v.laneChangeEndY;
            v.lane = v.targetLane;
            v.isChangingLane = false;
            v.laneChangeProgress = 0.0;
        }
        else
        {
            // 贝塞尔曲线插值Y坐标
            double t = v.laneChangeProgress;
            double sy = v.laneChangeStartY;
            double ey = v.laneChangeEndY;
            double c1 = sy + (ey - sy) * 0.3;
            double c2 = sy + (ey - sy) * 0.7;
            
            v.y = (int)bezierCurve(t, sy, c1, c2, ey);
            
            // 50%进度时切换车道编号（用于碰撞检测）
            if (t >= 0.5 && v.lane != v.targetLane)
            {
                v.lane = v.targetLane;
            }
        }
    }
}

// ==================== 模块2：碰撞检测系统 ====================

namespace CollisionSystem
{
    bool checkCollision(const Vehicle& a, const Vehicle& b)
    {
        if (abs(a.lane - b.lane) > 1) return false;
        
        int al, ar, at, ab, bl, br, bt, bb;
        a.getBounds(al, ar, at, ab);
        b.getBounds(bl, br, bt, bb);
        
        // === 关键修改：添加安全边距，任何接触都算碰撞 ===
        int margin = 2; // 2像素安全边距
        return !(ar + margin < bl || al - margin > br || 
                 ab + margin < bt || at - margin > bb);
    }
    
    bool checkWillCollide(const Vehicle& front, const Vehicle& back, int windowHeight)
    {
        if (front.lane != back.lane) return false;
        
        int middleY = windowHeight / 2;
        bool isUpperLane = back.y < middleY;
        bool isFrontAhead = isUpperLane ? (front.x > back.x) : (front.x < back.x);
        
        if (!isFrontAhead) return false;
        
        int dist = abs(front.x - back.x);
        // === 修改：降低安全距离，减少误判 ===
        int safetyFactor = front.isStalled ? 18 : 8; // 从 20/10 降低到 18/8
        int safeDist = (front.carlength + back.carlength) / 2 + back.speed * safetyFactor;
        
        return dist < safeDist;
    }
    
    void handleCollisions(vector<Vehicle>& vehicles, int windowHeight)
    {
        for (size_t i = 0; i < vehicles.size(); ++i)
        {
            if (vehicles[i].isStalled) continue;
            
            for (size_t j = 0; j < vehicles.size(); ++j)
            {
                if (i == j) continue;
                
                // === 1. 直接碰撞检测（任何接触都触发抛锚）===
                if (checkCollision(vehicles[i], vehicles[j]))
                {
                    // 只要有接触就抛锚
                    if (!vehicles[i].isStalled)
                    {
                        vehicles[i].isStalled = true;
                        vehicles[i].speed = 0;
                        vehicles[i].color = RGB(128, 128, 128);
                        vehicles[i].isChangingLane = false;
                    }
                }
                
                // === 2. 即将碰撞检测（强制减速或停车）===
                if (checkWillCollide(vehicles[j], vehicles[i], windowHeight))
                {
                    // === 修改：只在距离很近时才强制减速 ===
                    int dist = abs(vehicles[j].x - vehicles[i].x);
                    int criticalDist = (vehicles[i].carlength + vehicles[j].carlength) / 2 + 
                                      vehicles[i].speed * 3; // 从 5 降低到 3
                    
                    if (dist < criticalDist)
                    {
                        // 距离极近，减速
                        vehicles[i].speed = max(1, vehicles[i].speed - 1); // 逐渐减速而非停车
                    }
                }
            }
        }
    }
}

// ==================== 模块3：变道决策与执行 ====================

namespace LaneChangeSystem
{
    // === 核心安全检查：变道是否安全 ===
    bool isLaneChangeSafe(const Vehicle& v, int targetLane, int laneHeight,
                         int windowHeight, const vector<Vehicle>& vehicles)
    {
        int middleY = windowHeight / 2;
        bool isUpperLane = v.y < middleY;
        
        // 边界检查
        if (targetLane < 0 || targetLane > 5) return false;
        
        // 防止逆行
        if (isUpperLane && targetLane >= 3) return false;
        if (!isUpperLane && targetLane < 3) return false;
        
        // 计算变道需要的时间和距离
        double laneChangeTime = 30.0;
        int horizontalDist = (int)(v.speed * laneChangeTime);
        
        // === 修改：降低最小变道速度要求，增加超车机会 ===
        if (v.speed < 1)  // 从 2 降低到 1
        {
            return false;
        }
        
        // 检查变道路径和目标车道的所有车辆
        for (const auto& other : vehicles)
        {
            if (&v == &other) continue;
            
            int minLane = min(v.lane, targetLane);
            int maxLane = max(v.lane, targetLane);
            if (other.lane < minLane || other.lane > maxLane) continue;
            
            int xDiff = other.x - v.x;
            int hGap = abs(xDiff);
            
            bool dangerous = false;
            if (isUpperLane)
            {
                // === 修改：缩小危险区域范围 ===
                if (xDiff > -v.carlength * 2 && xDiff < horizontalDist + other.carlength * 2)
                    dangerous = true;
            }
            else
            {
                if (xDiff < v.carlength * 2 && xDiff > -(horizontalDist + other.carlength * 2))
                    dangerous = true;
            }
            
            if (dangerous)
            {
                // === 关键修改：大幅降低安全距离要求（针对超车）===
                int safetyFactor;
                if (other.isStalled)
                {
                    safetyFactor = 25; // 抛锚车保持高安全系数
                }
                else
                {
                    safetyFactor = 12; // 正常车从 20 降低到 12
                }
                
                int safeDist = (v.carlength + other.carlength) / 2 + 
                              max(v.speed, other.speed) * safetyFactor;
                
                if (hGap < safeDist)
                    return false;
            }
        }
        
        return true;
    }
    
    // === 统一的变道请求接口 ===
    bool requestLaneChange(Vehicle& v, int targetLane, int laneHeight, 
                          int windowHeight, const vector<Vehicle>& vehicles)
    {
        // 已在变道中
        if (v.isChangingLane) return false;
        
        // 目标车道与当前相同
        if (targetLane == v.lane) return false;
        
        // === 关键：严格的安全检查 ===
        if (!isLaneChangeSafe(v, targetLane, laneHeight, windowHeight, vehicles))
            return false;
        
        // 启动变道
        v.isChangingLane = true;
        v.targetLane = targetLane;
        v.laneChangeStartY = v.y;
        v.laneChangeEndY = laneHeight * targetLane + laneHeight / 2;
        v.laneChangeProgress = 0.0;
        
        return true;
    }
    
    // === 智能决策：是否需要变道 ===
    int decideTargetLane(const Vehicle& v, int windowHeight, int laneHeight,
                        const vector<Vehicle>& vehicles)
    {
        int middleY = windowHeight / 2;
        bool isUpperLane = v.y < middleY;
        
        // === 关键修改：增加提前检测距离，更早发现超车机会 ===
        int detectionDist = v.carlength * 8 + v.speed * 40; // 从 6 车长 + 30 倍速度提升到 8 车长 + 40 倍速度
        
        const Vehicle* closestObstacle = nullptr;
        int minDistance = INT_MAX;
        
        for (const auto& other : vehicles)
        {
            if (&v == &other) continue;
            if (other.lane != v.lane) continue;
            
            bool isAhead = isUpperLane ? (other.x > v.x) : (other.x < v.x);
            if (!isAhead) continue;
            
            int dist = abs(other.x - v.x);
            if (dist > detectionDist) continue;
            
            if (dist < minDistance)
            {
                minDistance = dist;
                closestObstacle = &other;
            }
        }
        
        // 如果发现障碍物
        if (closestObstacle != nullptr)
        {
            if (closestObstacle->isStalled)
            {
                // 抛锚车：必须绕行
                int bypassLane = isUpperLane ? v.lane + 1 : v.lane - 1;
                
                if (bypassLane < 0 || bypassLane > 5 || 
                    (isUpperLane && bypassLane >= 3) || 
                    (!isUpperLane && bypassLane < 3))
                {
                    bypassLane = isUpperLane ? v.lane - 1 : v.lane + 1;
                }
                
                if (bypassLane >= 0 && bypassLane <= 5)
                {
                    if (isUpperLane && bypassLane < 3) return bypassLane;
                    if (!isUpperLane && bypassLane >= 3) return bypassLane;
                }
            }
            // === 关键修改：大幅降低超车速度差阈值 ===
            else if (closestObstacle->speed < v.speed - 1) // 从 -2 降低到 -1，只要稍慢就超车
            {
                // 正常慢车：左侧超车
                int overtakeLane = isUpperLane ? v.lane - 1 : v.lane + 1;
                if (overtakeLane >= 0 && overtakeLane <= 5)
                {
                    if (isUpperLane && overtakeLane < 3) return overtakeLane;
                    if (!isUpperLane && overtakeLane >= 3) return overtakeLane;
                }
                
                // === 新增：如果左侧不行，尝试右侧超车（在距离较远时）===
                if (minDistance > v.carlength * 5)
                {
                    int rightLane = isUpperLane ? v.lane + 1 : v.lane - 1;
                    if (rightLane >= 0 && rightLane <= 5)
                    {
                        if (isUpperLane && rightLane < 3) return rightLane;
                        if (!isUpperLane && rightLane >= 3) return rightLane;
                    }
                }
            }
            // === 新增：即使速度相同，距离很近时也尝试超车 ===
            else if (closestObstacle->speed <= v.speed && minDistance < v.carlength * 4)
            {
                int overtakeLane = isUpperLane ? v.lane - 1 : v.lane + 1;
                if (overtakeLane >= 0 && overtakeLane <= 5)
                {
                    if (isUpperLane && overtakeLane < 3) return overtakeLane;
                    if (!isUpperLane && overtakeLane >= 3) return overtakeLane;
                }
            }
            else
            {
                // === 修改：只有距离很近时才减速，其他情况尝试超车 ===
                if (minDistance < v.carlength * 3)
                {
                    return -2; // 需要减速
                }
                else
                {
                    // 距离较远，尝试超车而不是减速
                    int overtakeLane = isUpperLane ? v.lane - 1 : v.lane + 1;
                    if (overtakeLane >= 0 && overtakeLane <= 5)
                    {
                        if (isUpperLane && overtakeLane < 3) return overtakeLane;
                        if (!isUpperLane && overtakeLane >= 3) return overtakeLane;
                    }
                }
            }
        }
        
        return -1; // 无需变道
    }
}

// ==================== 新增：模块4：车辆行为管理 ====================

namespace VehicleBehavior
{
    double calculateSpeedDiff(const Vehicle& back, const Vehicle& front)
    {
        // 返回后车相对前车的速度差（km/h）
        return back.speedKmh - front.speedKmh;
    }
    
    int calculateSafeFollowDistance(const Vehicle& v)
    {
        // 安全跟车距离 = 速度 × 反应时间
        return (int)(v.speed * TrafficRules::SAFE_FOLLOW_TIME * TrafficRules::FRAME_RATE);
    }
    
    BehaviorType decideBehavior(const Vehicle& back, const Vehicle& front, 
                               int windowHeight, double& speedDiff)
    {
        speedDiff = calculateSpeedDiff(back, front);
        
        if (front.isStalled)
        {
            return BehaviorType::EMERGENCY_BRAKE;
        }
        
        if (speedDiff > TrafficRules::CRASH_SPEED_DIFF)
        {
            // 速度差 > 30 km/h：危险，紧急刹车
            return BehaviorType::EMERGENCY_BRAKE;
        }
        else if (speedDiff > TrafficRules::OVERTAKE_SPEED_DIFF)
        {
            // 速度差 > 15 km/h：超车
            return BehaviorType::OVERTAKING;
        }
        else if (speedDiff > TrafficRules::FOLLOW_SPEED_DIFF)
        {
            // 速度差 5~15 km/h：尝试超车或跟随
            return BehaviorType::OVERTAKING;
        }
        else if (abs(speedDiff) <= TrafficRules::FOLLOW_SPEED_DIFF)
        {
            // 速度差 ≤ 5 km/h：跟随
            return BehaviorType::FOLLOWING;
        }
        
        return BehaviorType::FREE_DRIVING;
    }
    
    void adjustSpeed(Vehicle& v, BehaviorType behavior, const Vehicle* frontVehicle,
                    double speedDiff, int distance)
    {
        switch (behavior)
        {
        case BehaviorType::FREE_DRIVING:
            // 自由行驶：恢复到原始速度
            if (v.speed < v.originalSpeed)
            {
                v.speed = min(v.originalSpeed, v.speed + 1);
            }
            break;
            
        case BehaviorType::FOLLOWING:
            // 跟随：保持与前车相同速度
            if (frontVehicle != nullptr)
            {
                int targetSpeed = frontVehicle->speed;
                if (v.speed > targetSpeed)
                {
                    v.speed = max(targetSpeed, v.speed - 1);
                }
                else if (v.speed < targetSpeed && distance > v.carlength * 4)
                {
                    v.speed = min(targetSpeed, v.speed + 1);
                }
            }
            break;
            
        case BehaviorType::OVERTAKING:
            // 超车：加速（但不超过原始速度）
            if (v.speed < v.originalSpeed)
            {
                v.speed = min(v.originalSpeed, v.speed + 1);
            }
            break;
            
        case BehaviorType::EMERGENCY_BRAKE:
            // 紧急刹车：快速减速
            v.speed = max(0, v.speed - 2);
            break;
        }
    }
}
