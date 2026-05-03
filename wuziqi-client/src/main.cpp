// ============================================================
// main.cpp — 五子棋客户端入口
// ============================================================
// 【Qt 学习笔记 — 应用程序入口】
//
// 1. QApplication
//    Qt 程序的起点。管理全局资源：字体、样式表、剪贴板、
//    事件循环等。每个 Qt GUI 程序有且只有一个 QApplication
//    （或 QGuiApplication）实例，必须在创建任何 QWidget 之前构造。
//
// 2. QApplication(argc, argv)
//    构造函数解析命令行参数，Qt 自身会处理一些参数
//    （如 -style= 切换界面风格，-qmljsdebugger= 等）。
//    通常直接传入 main() 的参数即可。
//
// 3. app.setApplicationName()
//    设置应用程序名称。影响 QSettings 存储路径、
//    窗口管理器标题栏提示等。
//
// 4. window.show()
//    使窗口可见。QWidget 默认是隐藏的，必须显式调用 show()。
//
// 5. app.exec()
//    进入 Qt 事件循环（event loop）。这是一个永不返回的循环，
//    不断处理：用户输入（鼠标/键盘）、定时器（QTimer）、
//    网络事件（信号/槽）、重绘事件等。
//    当最后一个窗口关闭时（或调用 QApplication::quit()），
//    exec() 返回。
//
// 关键概念：事件循环（Event Loop）
//   ┌──────────────┐
//   │ 事件队列       │ ← 鼠标点击、键盘、定时器、网络
//   └──────┬───────┘
//          ▼
//   ┌──────────────┐
//   │ 事件分发器     │ → 将事件派发给对应的 QObject
//   └──────┬───────┘
//          ▼
//   ┌──────────────┐
//   │ 事件处理器     │ → event() → 具体 handler（如 paintEvent）
//   └──────────────┘
// ============================================================

#include <QApplication>   // QApplication 类
#include "mainwindow.h"   // 我们自定义的主窗口

int main(int argc, char *argv[])
{
    // 1. 创建 QApplication 实例 —— 每个 Qt GUI 程序必须且只有一个
    QApplication app(argc, argv);

    // 2. 设置应用程序元信息（用于 QSettings 等）
    app.setApplicationName("五子棋");

    // 3. 创建并显示主窗口
    MainWindow window;
    window.show();   // QWidget 默认隐藏，show() 使其可见

    // 4. 进入事件循环 —— 直到程序退出才返回
    return app.exec();
}
