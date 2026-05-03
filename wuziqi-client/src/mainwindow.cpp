// ============================================================
#include <QDebug>
// mainwindow.cpp — 主窗口实现（页面调度）
// ============================================================
// 【Qt 学习笔记 — 对象树 / 信号-槽连接 / 页面栈管理】
//
// 1. Qt 对象树（Object Tree）
//    当创建一个 QObject 并传入 parent 指针时，父子关系建立。
//    父对象析构时自动删除其所有子对象，避免内存泄漏。
//    所以大多数 Qt 类只需 new，不需要手动 delete。
//
//    例如：m_stack = new QStackedWidget(this);
//          因为 this（MainWindow）是父对象，
//          所以在 ~MainWindow() 中不需要 delete m_stack。
//
// 2. setCentralWidget()
//    QMainWindow 特有的方法。将某控件设为主窗口的中央区域，
//    它会自动填充菜单栏和状态栏之间的区域。
//
// 3. connect() 的几种写法
//    - 信号→信号：connect(src, &A::sig, dst, &B::sig)
//    - 信号→槽：connect(src, &A::sig, dst, &B::slot)
//    - Lambda：connect(src, &A::sig, this, [this](){ ... })
//    - 带参数转换：有时需要 qOverload<int>(&A::sig) 消除重载歧义
//
// 4. QStackedWidget::addWidget()
//    将 QWidget 加入页面栈，返回一个索引 index。
//    setCurrentWidget() 按 widget 指针切换，
//    setCurrentIndex() 按索引切换。
// ============================================================

#include "mainwindow.h"
#include "networkmanager.h"  // 网络层
#include "loginwidget.h"     // 登录页
#include "lobbywidget.h"     // 大厅页
#include "gamewidget.h"      // 游戏页（棋盘）
#include <QMessageBox>       // 消息弹窗（未使用，但保留备用）

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)                      // 调用基类构造
    , m_net(new NetworkManager(this))          // 创建网络管理器，托管给 Qt 对象树
{
    // === 窗口基本设置 ===
    setWindowTitle("五子棋");                   // 窗口标题
    setMinimumSize(620, 680);                  // 最小尺寸
    resize(640, 700);                          // 初始尺寸

    // === 创建页面栈容器 ===
    // QStackedWidget 是"容器控件"——它本身没有视觉外观，
    // 但可以管理多个子 widget，并在它们之间切换。
    // 设为中央控件后，它会自动填充整个主窗口内容区。
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    // === 创建各页面 ===
    // 注意：m_lobbyWidget 延迟创建——因为创建大厅需要 userId，
    // 而 userId 在登录成功后才获取。这是 Qt 编程中常见的
    // "延迟初始化"模式，避免构造时依赖异步数据。

    // 登录页
    m_loginWidget = new LoginWidget(m_net, this);
    m_stack->addWidget(m_loginWidget);  // index 0

    // 游戏页（棋盘）
    m_gameWidget = new GameWidget(m_net, this);
    m_stack->addWidget(m_gameWidget);   // index 1

    // 大厅页（占位，登录成功后再创建）
    m_lobbyWidget = nullptr;

    // === 信号-槽连接 ===
    // 登录成功 → 切换到大厅
    connect(m_loginWidget, &LoginWidget::loginSuccess,
            this, &MainWindow::onLoginSuccess);

    // 游戏页返回 → 切换到大厅
    connect(m_gameWidget, &GameWidget::backToLobby,
            this, &MainWindow::onBackToLobby);

    // === 初始页面 ===
    m_stack->setCurrentWidget(m_loginWidget);  // 从登录页开始
}

MainWindow::~MainWindow()
{
    // 不需要手动 delete！Qt 对象树会自动处理：
    // m_net, m_stack, m_loginWidget, m_gameWidget
    // 都是 this 的子对象，MainWindow 析构时会自动 delete 它们。
}

// ========== 槽函数实现 ==========

void MainWindow::onLoginSuccess(const QString &token, int userId)
{
    qDebug() << "[MainWindow] 登录成功, userId=" << userId << "token=" << token.left(12);
    m_token = token;
    m_userId = userId;

    // 将 token 存入网络管理器——WS 连接成功后自动发送 wsLogin
    m_net->setToken(token);

    // === 延迟创建大厅 ===
    // 删除旧大厅（如果有，通常首次登录时没有）
    if (m_lobbyWidget) {
        m_stack->removeWidget(m_lobbyWidget);
        delete m_lobbyWidget;
    }

    // 创建新大厅，传入 userId 和 token
    m_lobbyWidget = new LobbyWidget(m_net, userId, token, this);
    m_stack->addWidget(m_lobbyWidget);          // 添加到页面栈
    m_stack->setCurrentWidget(m_lobbyWidget);   // 切换到大厅

    // 连接大厅页的信号
    connect(m_lobbyWidget, &LobbyWidget::gameStarted,
            this, &MainWindow::onGameStarted);
    connect(m_lobbyWidget, &LobbyWidget::logoutRequested,
            this, &MainWindow::onLogoutRequested);
}

void MainWindow::onLogoutRequested()
{
    // 断开 WebSocket 连接
    m_net->disconnectWebSocket();

    // 清空 token（下次 WS 重连不会再自动登录）
    m_net->setToken(QString());

    // 清空用户状态
    m_token.clear();
    m_userId = -1;

    // 删除大厅页
    if (m_lobbyWidget) {
        m_stack->removeWidget(m_lobbyWidget);
        delete m_lobbyWidget;
        m_lobbyWidget = nullptr;
    }

    // 回到登录页
    m_stack->setCurrentWidget(m_loginWidget);
}

void MainWindow::onGameStarted(const QString &roomId, const QString &myColor)
{
    qDebug() << "[MainWindow] 游戏开始, roomId=" << roomId << "color=" << myColor;

    // 颜色由大厅通过 room_ready 算出并传过来，直接设入游戏页
    m_gameWidget->startGame(m_userId, roomId, myColor);
    m_stack->setCurrentWidget(m_gameWidget);
}

void MainWindow::onBackToLobby()
{
    if (m_lobbyWidget) {
        m_stack->setCurrentWidget(m_lobbyWidget);
    }
}
