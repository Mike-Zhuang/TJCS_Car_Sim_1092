// 显示橘色线框
void Vehicle::showFlashingFrame(int flashCount) {
    // 保存当前线型和颜色
    LINESTYLE oldLineStyle ;
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