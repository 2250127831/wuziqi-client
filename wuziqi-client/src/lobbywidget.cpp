// ============================================================
// lobbywidget.cpp — 游戏大厅实现
// ============================================================
// 【Qt 学习笔记 — 嵌套布局 / QGroupBox 使用 / 信号队列】
//
// 1. 嵌套布局（Nested Layouts）
//    QVBoxLayout 里可以嵌套 QHBoxLayout，反之亦然。
//    例如：
//    mainLayout (VBox)
//      ├── topBar (HBox)
//      │     ├── title (QLabel)
//      │     ├── stretch
//      │     └── logoutBtn (QPushButton)
//      ├── statusLabel
//      ├── randGroup (QGroupBox)
//      │     └── randLayout (VBox)
//      │           ├── randDesc (QLabel)
//      │           ├── randBtnRow (HBox)
//      │           │     ├── m_randMatchBtn
//      │           │     └── m_cancelBtn
//      │           └── ...
//      ├── roomGroup (QGroupBox)
//      └── stretch
//
//    这种嵌套可以做出复杂的布局，而且都是自动适应的。
//
// 2. addStretch() 的妙用
//    - 在 HBoxLayout 中，addStretch() 在中间，会将左右两个控件推至两端
//    - 在 VBoxLayout 底部的 addStretch() 会将上方内容"顶"到顶部
//    - stretch 参数表示弹性系数，addStretch(2) 比 addStretch(1) 占两倍弹性空间
//
// 3. QTimer 的使用场景
//    - 匹配轮询：每隔 1 秒发送状态查询请求
//    - 游戏计时器（如倒计时）
//    - 定时保存、自动刷新等
//    注意：QTimer 依赖事件循环。如果主线程被 long-running 操作阻塞，
//    定时器不会触发。因此不要在主线程中做耗时操作。
//
// 4. setTextInteractionFlags(Qt::TextSelectableByMouse)
//    默认 QLabel 文字不可选中。设置后用户可复制邀请码。
//    这是很实用的设计细节——用户可以直接选中邀请码 Ctrl+C 发给好友。
//
// 5. Q_UNUSED(x) 宏
//    当函数签名中有未使用的参数时，用 Q_UNUSED(x) 避免编译器警告。
//    这是 Qt 项目的常见实践。
// ============================================================

#include "lobbywidget.h"
#include "networkmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>    // 分组框控件
#include <QFont>
#include <QDebug>       // 调试输出

LobbyWidget::LobbyWidget(NetworkManager *net, int userId, const QString &token, QWidget *parent)
    : QWidget(parent)
    , m_net(net)
    , m_userId(userId)
    , m_token(token)
{
    // === 主布局 ===
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);               // 子控件间距 16px
    mainLayout->setContentsMargins(60, 20, 60, 20);  // 外边距

    // ======= 顶部栏：标题 + 退出按钮 =======
    auto *topBar = new QHBoxLayout();

    auto *title = new QLabel("五子棋 · 大厅", this);
    QFont tf;
    tf.setPointSize(22);
    tf.setBold(true);
    title->setFont(tf);
    topBar->addWidget(title);

    topBar->addStretch();   // 弹性空白：把退出按钮推到右侧

    auto *logoutBtn = new QPushButton("退出登录", this);
    connect(logoutBtn, &QPushButton::clicked, this, &LobbyWidget::logoutRequested);
    topBar->addWidget(logoutBtn);

    mainLayout->addLayout(topBar);

    // ======= 状态提示 =======
    m_statusLabel = new QLabel("选择一个模式开始游戏", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    // 样式表（CSS风格）—— 设置颜色、字号、内边距
    m_statusLabel->setStyleSheet("color: #555; font-size: 14px; padding: 8px;");
    mainLayout->addWidget(m_statusLabel);

    // ======= 随机匹配分组 =======
    // QGroupBox 带标题边框的分组框
    auto *randGroup = new QGroupBox("随机匹配", this);
    auto *randLayout = new QVBoxLayout(randGroup);

    auto *randDesc = new QLabel("系统自动匹配在线玩家", randGroup);
    randDesc->setStyleSheet("color: gray;");
    randLayout->addWidget(randDesc);

    // 按钮行
    auto *randBtnRow = new QHBoxLayout();
    m_randMatchBtn = new QPushButton("开始匹配", randGroup);
    m_randMatchBtn->setFixedHeight(35);
    m_randMatchBtn->setStyleSheet("font-size: 14px;");
    randBtnRow->addWidget(m_randMatchBtn);

    m_cancelBtn = new QPushButton("取消匹配", randGroup);
    m_cancelBtn->setFixedHeight(35);
    m_cancelBtn->setEnabled(false);    // 初始禁用（还没开始匹配）
    randBtnRow->addWidget(m_cancelBtn);

    randLayout->addLayout(randBtnRow);
    mainLayout->addWidget(randGroup);

    // ======= 口令房间分组 =======
    auto *roomGroup = new QGroupBox("口令房间", this);
    auto *roomLayout = new QVBoxLayout(roomGroup);

    // 创建房间 + 显示邀请码
    auto *createRow = new QHBoxLayout();
    m_createRoomBtn = new QPushButton("创建房间", roomGroup);
    m_createRoomBtn->setFixedHeight(35);
    createRow->addWidget(m_createRoomBtn);

    m_inviteCodeLabel = new QLabel("", roomGroup);
    m_inviteCodeLabel->setStyleSheet("color: #2a7; font-weight: bold; font-size: 16px; padding: 4px;");
    // 允许鼠标选中文字（方便复制邀请码发给好友）
    m_inviteCodeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    createRow->addWidget(m_inviteCodeLabel, 1);   // stretch=1 占满剩余空间
    roomLayout->addLayout(createRow);

    // 加入房间行
    auto *joinRow = new QHBoxLayout();
    joinRow->addWidget(new QLabel("邀请码:", roomGroup));
    m_inviteInput = new QLineEdit(roomGroup);
    m_inviteInput->setPlaceholderText("输入邀请码");
    m_inviteInput->setFixedWidth(180);
    joinRow->addWidget(m_inviteInput);

    m_joinRoomBtn = new QPushButton("加入房间", roomGroup);
    m_joinRoomBtn->setFixedHeight(35);
    joinRow->addWidget(m_joinRoomBtn);
    joinRow->addStretch();
    roomLayout->addLayout(joinRow);

    mainLayout->addWidget(roomGroup);
    mainLayout->addStretch();    // 弹性空白，把内容推到上方

    // ======= 匹配轮询定时器 =======
    // 每 1000ms（1 秒）查询一次匹配状态
    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(1000);     // 1 秒间隔
    connect(m_pollTimer, &QTimer::timeout, this, &LobbyWidget::onMatchPoll);

    // ======= 按钮信号连接 =======
    connect(m_randMatchBtn, &QPushButton::clicked, this, &LobbyWidget::onRandomMatch);
    connect(m_cancelBtn, &QPushButton::clicked, this, &LobbyWidget::onCancelMatch);
    connect(m_createRoomBtn, &QPushButton::clicked, this, &LobbyWidget::onCreateRoom);
    connect(m_joinRoomBtn, &QPushButton::clicked, this, &LobbyWidget::onJoinRoom);

    // ======= 网络回调连接 =======
    connect(m_net, &NetworkManager::randomMatchResult, this, &LobbyWidget::onRandomMatchResult);
    connect(m_net, &NetworkManager::cancelMatchResult, this, &LobbyWidget::onCancelMatchResult);
    connect(m_net, &NetworkManager::createRoomResult, this, &LobbyWidget::onCreateRoomResult);
    connect(m_net, &NetworkManager::joinRoomResult, this, &LobbyWidget::onJoinRoomResult);

    // WS 登录结果日志
    connect(m_net, &NetworkManager::wsLoginResult, this, [this](bool ok) {
        qDebug() << "[Lobby] WS 登录结果: success=" << ok;
    });

    // 匹配状态查询回调（轮询用）—— 发现已分配房间时，等待 WS room_ready
    connect(m_net, &NetworkManager::matchStatusResult, this, [this](bool success, bool inRoom, const QString &roomId) {
        if (m_isMatching && inRoom) {
            m_isMatching = false;
            m_pollTimer->stop();
            qDebug() << "[Lobby] 已分配房间 roomId=" << roomId << ", 等待 room_ready...";
            m_statusLabel->setText("匹配成功！等待对手确认...");
            m_randMatchBtn->setEnabled(true);
            m_cancelBtn->setEnabled(false);
            // 不立即启动游戏！等 WS room_ready 才真正开始
        }
    });

    // WS room_ready 到达 → 算出颜色，启动游戏
    connect(m_net, &NetworkManager::wsRoomReady, this, [this](const QString &roomId, int player1, int player2) {
        if (!roomId.isEmpty()) {
            // p1=黑棋(先手), p2=白棋(后手)
            QString color = (m_userId == player1) ? "black" : "white";
            qDebug() << "[Lobby] 收到 room_ready, roomId=" << roomId
                     << "myColor=" << color << "(p1=" << player1 << "p2=" << player2 << ")";
            m_statusLabel->setText("游戏开始！");
            // 直接传颜色，游戏页不需要再连 wsRoomReady
            emit gameStarted(roomId, color);
        }
    });

    // === 连接 WebSocket 并登录 ===
    m_net->connectWebSocket();
}

// ========== 槽函数 ==========

void LobbyWidget::onRandomMatch()
{
    qDebug() << "[Lobby] 点击匹配, userId=" << m_userId;
    m_statusLabel->setText("正在匹配中...");
    m_randMatchBtn->setEnabled(false);   // 禁用"开始匹配"按钮
    m_cancelBtn->setEnabled(true);       // 启用"取消匹配"按钮
    m_isMatching = true;                 // 标记匹配状态
    m_net->randomMatch(m_userId);        // 发送匹配请求
    m_pollTimer->start();                // 开始轮询匹配状态
}

void LobbyWidget::onCancelMatch()
{
    m_net->cancelMatch(m_userId);
}

void LobbyWidget::onCreateRoom()
{
    m_net->createRoom(m_userId);
}

void LobbyWidget::onJoinRoom()
{
    QString code = m_inviteInput->text().trimmed();
    if (code.isEmpty()) {
        m_statusLabel->setText("请输入邀请码");
        return;
    }
    m_net->joinRoom(m_userId, code);
}

// ---- 网络回调 ----

void LobbyWidget::onRandomMatchResult(bool success, const QString &msg, const QString &roomId)
{
    qDebug() << "[Lobby] 匹配结果: success=" << success << "roomId=" << roomId << "msg=" << msg;

    // 情况1: 直接匹配成功（服务器分配了 roomId）
    if (!roomId.isEmpty()) {
        qDebug() << "[Lobby] 直接匹配成功, roomId=" << roomId << "等待 room_ready...";
        m_isMatching = false;
        m_pollTimer->stop();
        m_randMatchBtn->setEnabled(true);
        m_cancelBtn->setEnabled(false);
        m_statusLabel->setText("匹配成功！等待对手确认...");
        // 不立即启动游戏！颜色由 room_ready 提供
        return;
    }

    // 情况2: 已加入匹配队列（服务器返回 success=false + 消息含"匹配队列"）
    // 这是服务器协议特点：入队成功也是 success=false，靠 msg 区分
    if (msg.contains("匹配队列") || msg.contains("正在匹配") || msg.contains("queue")) {
        qDebug() << "[Lobby] 已加入匹配队列, 等待轮询";
        m_statusLabel->setText("已加入匹配队列...");
        // 定时器已在 onRandomMatch 中启动，保持运行
        return;
    }

    // 情况3: 真正的匹配失败
    qDebug() << "[Lobby] 匹配请求失败:" << msg;
    m_statusLabel->setText("匹配失败: " + msg);
    m_randMatchBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
    m_isMatching = false;
    m_pollTimer->stop();
}

void LobbyWidget::onCancelMatchResult(bool success, const QString &msg)
{
    Q_UNUSED(msg);
    m_isMatching = false;
    m_pollTimer->stop();
    m_randMatchBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
    m_statusLabel->setText(success ? "已取消匹配" : "取消匹配失败");
}

void LobbyWidget::onCreateRoomResult(bool success, const QString &roomId, const QString &inviteCode)
{
    Q_UNUSED(roomId);
    if (success) {
        m_inviteCodeLabel->setText("房间已创建！邀请码: " + inviteCode);
        m_statusLabel->setText("等待对手加入...");
        // 对手加入后，服务器会推送 room_ready → 自动启动游戏
    } else {
        m_statusLabel->setText("创建房间失败");
    }
}

void LobbyWidget::onJoinRoomResult(bool success, const QString &roomId, const QString &msg)
{
    if (success && !roomId.isEmpty()) {
        qDebug() << "[Lobby] 加入房间成功, roomId=" << roomId << "等待 room_ready...";
        m_statusLabel->setText("加入成功！等待对手确认...");
        // 不立即启动游戏——等 room_ready
    } else {
        m_statusLabel->setText("加入失败: " + msg);
    }
}

void LobbyWidget::onMatchPoll()
{
    qDebug() << "[Lobby] 轮询匹配状态, m_isMatching=" << m_isMatching;
    if (m_isMatching) {
        // 定期向服务器查询匹配状态
        m_net->getMatchStatus(m_userId);
    }
}
