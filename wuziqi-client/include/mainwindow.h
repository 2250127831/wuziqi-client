// ============================================================
// mainwindow.h — 主窗口（页面调度器）
// ============================================================
// 【Qt 学习笔记 — QMainWindow / QStackedWidget / Q_OBJECT】
//
// 1. QMainWindow
//    提供了主窗口框架：菜单栏（menuBar）、工具栏（toolBar）、
//    状态栏（statusBar）、停靠窗口（dockWidgets）和
//    中央部件（centralWidget）。
//    我们只用了中央部件功能。
//
// 2. Q_OBJECT 宏
//    必须放在声明了信号（signals）或槽（slots）的 QObject
//    子类的 private/protected/public 段中。
//    Q_OBJECT 让 MOC（元对象编译器）为该类生成：
//    - 信号/槽的连接（connect/disconnect）
//    - 运行时类型信息（qobject_cast、className）
//    - 属性系统（Q_PROPERTY）
//    - 反射/内省能力
//    注意：如果漏写 Q_OBJECT，信号/槽会连接失败（编译无错但运行时报错）。
//
// 3. QStackedWidget
//    容器控件，内部有多个"页面"（QWidget），同时只显示一个。
//    类似于 Android 的 ViewPager 或 iOS 的 UIPageViewController。
//    通过 setCurrentIndex() / setCurrentWidget() 切换页面。
//    适用于：登录→大厅→游戏 的页面流。
//
// 4. connect(信号, 槽)
//    信号-槽（Signals & Slots）是 Qt 的核心机制，用于对象间通信。
//    发送者发出信号（signal），接收者的槽（slot）被自动调用。
//    信号-槽是类型安全的，支持：一对多、多对一、跨线程。
//    老语法：connect(sender, SIGNAL(xx), receiver, SLOT(xx))
//    新语法：connect(sender, &Class::signal, receiver, &Class::slot)
//    本项目中全部使用新语法（编译期检查，更安全）。
//
// 5. explicit 构造函数
//    防止隐式类型转换。explicit QWidget(parent = nullptr) 避免
//    意外将 nullptr 或 int 转换为 MainWindow。
// ============================================================

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>    // 主窗口基类
#include <QStackedWidget> // 页面堆栈容器

// 前向声明（forward declaration），减少头文件依赖，加速编译
class NetworkManager;
class LoginWidget;
class LobbyWidget;
class GameWidget;

class MainWindow : public QMainWindow
{
    // Q_OBJECT 宏：启用 Qt 元对象系统（信号/槽/反射）
    // 任何自定义信号或槽的类都必须有此宏
    Q_OBJECT

public:
    // explicit 禁止隐式转换，parent 参数用于 Qt 对象树内存管理
    // 父对象析构时会自动析构子对象
    explicit MainWindow(QWidget *parent = nullptr);

    // override 关键字：重写虚析构函数
    // QMainWindow 的析构函数是虚函数，加 override 可让编译器检查
    ~MainWindow() override;

    // --- 槽函数（Slots） ---
    // 槽就是普通的成员函数，可以被信号触发，也可以直接调用。
    // private slots 表示这些槽只能由内部信号或内部代码触发，
    // 外部类不能直接 connect 到这些槽（但从 Qt5 新语法看，
    // 这只是编译期的限制，实际上仍可通过函数指针连接）。
private slots:
    void onLoginSuccess(const QString &token, int userId);
    void onLogoutRequested();
    void onGameStarted(const QString &roomId, const QString &myColor);
    void onBackToLobby();

private:
    // --- 核心对象 ---
    NetworkManager   *m_net;          // 网络管理器（HTTP + WebSocket）
    QStackedWidget   *m_stack;        // 页面容器（登录/大厅/游戏）

    LoginWidget      *m_loginWidget;  // 登录页
    LobbyWidget      *m_lobbyWidget;  // 大厅页（登录成功后创建）
    GameWidget       *m_gameWidget;   // 游戏页（棋盘）

    // --- 用户状态 ---
    QString           m_token;        // 登录令牌（JWT token）
    int               m_userId = -1;  // 用户 ID
};

#endif // MAINWINDOW_H
