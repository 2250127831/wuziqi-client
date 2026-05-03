// ============================================================
// lobbywidget.h — 游戏大厅头文件
// ============================================================
// 【Qt 学习笔记 — QGroupBox / QTimer / 文本交互标志】
//
// 1. QGroupBox
//    带标题的分组框控件，用于视觉上将相关控件组合在一起。
//    自带标题文字和边框线，比 QWidget 加 QLabel 更美观。
//    常用 API：
//    - 构造函数传入标题文字
//    - setCheckable(true) — 可选：使标题变成 checkbox
//    - setFlat(true) — 扁平风格
//
// 2. QTimer
//    定时器控件，可以在指定时间间隔后触发事件。
//    - setInterval(ms) — 设置间隔（毫秒）
//    - start() / stop() — 启动/停止
//    - timeout() — 信号：每次定时器到时发射
//    - setSingleShot(true) — 仅触发一次（类似 setTimeout）
//    - 注意：QTimer 依赖事件循环，如果主线程阻塞，定时器不会触发
//
// 3. Qt::TextSelectableByMouse
//    设置 QLabel 的文字可以被鼠标选中/复制。
//    默认 QLabel 文字不可选中（TextSelectableByMouse 未设置）。
//    设置为这个标志后，用户可复制邀请码。
//    其他文本交互标志：
//    - Qt::TextSelectableByKeyboard — 键盘选中
//    - Qt::LinksAccessibleByMouse — 可点击链接
// ============================================================

#ifndef LOBBYWIDGET_H
#define LOBBYWIDGET_H

#include <QWidget>       // QWidget 基类
#include <QLabel>        // 标签
#include <QPushButton>   // 按钮
#include <QLineEdit>     // 输入框
#include <QTimer>        // 定时器

class NetworkManager;

class LobbyWidget : public QWidget
{
    Q_OBJECT

public:
    // 构造函数需要 userId 和 token，因为大厅功能需要已登录状态
    explicit LobbyWidget(NetworkManager *net, int userId, const QString &token, QWidget *parent = nullptr);

signals:
    // 信号：游戏开始（参数是 roomId）
    void gameStarted(const QString &roomId, const QString &myColor);
    // 信号：用户请求退出登录
    void logoutRequested();

private slots:
    void onRandomMatch();                    // 点击"随机匹配"
    void onCancelMatch();                    // 点击"取消匹配"
    void onCreateRoom();                     // 点击"创建房间"
    void onJoinRoom();                       // 点击"加入房间"

    // 网络回调
    void onRandomMatchResult(bool success, const QString &msg, const QString &roomId);
    void onCancelMatchResult(bool success, const QString &msg);
    void onCreateRoomResult(bool success, const QString &roomId, const QString &inviteCode);
    void onJoinRoomResult(bool success, const QString &roomId, const QString &msg);

    // 匹配轮询
    void onMatchPoll();

private:
    NetworkManager *m_net;
    int m_userId;
    QString m_token;

    QLabel *m_statusLabel;           // 状态提示文字
    QPushButton *m_randMatchBtn;     // 开始匹配按钮
    QPushButton *m_cancelBtn;        // 取消匹配按钮
    QPushButton *m_createRoomBtn;    // 创建房间按钮
    QLineEdit *m_inviteInput;        // 邀请码输入框
    QPushButton *m_joinRoomBtn;      // 加入房间按钮
    QLabel *m_inviteCodeLabel;       // 显示生成的邀请码

    QTimer *m_pollTimer;             // 匹配轮询定时器
    bool m_isMatching = false;       // 是否正在匹配中
};

#endif // LOBBYWIDGET_H
