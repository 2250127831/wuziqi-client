// ============================================================
// networkmanager.h — 网络管理器（主线程代理层）
// ============================================================
// 【Qt 学习笔记 — QThread + 跨线程代理模式】
//
// NetworkManager 现在是"代理"，不直接做任何网络操作。
// 它做的事情：
//   1. 创建 QThread + NetworkWorker，将 Worker moveToThread
//   2. 主线程调用 registerUser() 时，通过 invokeMethod
//      将 doRegisterUser() 调度到后台线程执行
//   3. Worker 的 emit 信号跨线程投递（Qt::QueuedConnection），
//      由 NetworkManager 原样转发给 Widget
//
// 优势：
//   - Widget 代码完全不用变（信号名、参数完全一致）
//   - 网络 I/O 在后台线程，主线程永不阻塞
//   - QWebSocket 也在后台线程，不影响 UI
//   - 简历可以写 "基于 QThread 实现网络层异步化，消除 UI 卡顿"
// ============================================================

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>
#include <QAtomicInt>

// 前向声明——Widget 代码不需要知道 NetworkWorker 的存在
class NetworkWorker;

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);

    // 析构时自动停止线程并清理 Worker
    ~NetworkManager() override;

    // ==================== HTTP API ====================
    void registerUser(const QString &username, const QString &password);
    void loginUser(const QString &username, const QString &password);
    void verifyToken(const QString &token);
    void randomMatch(int userId);
    void cancelMatch(int userId);
    void createRoom(int userId);
    void joinRoom(int userId, const QString &inviteCode);
    void getMatchStatus(int userId);
    void quitRoom(int userId, const QString &roomId);

    // 存储 token，WebSocket 连接成功后自动发送 wsLogin
    void setToken(const QString &token) { m_storedToken = token; }
    const QString& token() const { return m_storedToken; }

    // ==================== WebSocket API ====================
    void connectWebSocket();
    void disconnectWebSocket();
    void wsSendMove(const QString &roomId, int x, int y);
    void wsQuitRoom(const QString &roomId);

    // WebSocket 连接状态（跨线程安全——原子变量）
    bool isWsConnected() const { return m_wsConnected.loadRelaxed() != 0; }

signals:
    // ---------- HTTP 响应信号（与 Widget 连接的信号，原样） ----------
    void registerResult(bool success, const QString &msg);
    void loginResult(bool success, const QString &token, int userId, const QString &msg);
    void verifyResult(bool success, int userId);

    void randomMatchResult(bool success, const QString &msg, const QString &roomId);
    void cancelMatchResult(bool success, const QString &msg);
    void createRoomResult(bool success, const QString &roomId, const QString &inviteCode);
    void joinRoomResult(bool success, const QString &roomId, const QString &msg);
    void matchStatusResult(bool success, bool inRoom, const QString &roomId);
    void quitRoomResult(bool success, const QString &msg);

    // ---------- WebSocket 响应信号 ----------
    void wsConnectedChanged(bool connected);
    void wsLoginResult(bool success);
    void wsRoomReady(const QString &roomId, int player1, int player2);
    void wsMoveReceived(bool success, int x, int y);
    void wsOpponentMove(int x, int y);
    void wsGameOver(const QString &winner, const QString &reason);
    void wsOpponentQuit(const QString &roomId, int userId);
    void wsError(const QString &msg);

private:
    // 后台线程 + Worker
    QThread *m_workerThread;
    NetworkWorker *m_worker;

    // 跨线程安全的 WebSocket 状态
    QAtomicInt m_wsConnected;

    // 存储当前 token，WS 连接后自动发送 wsLogin
    QString m_storedToken;

private slots:
    // 从 Worker 接收 wsConnectedChanged，同步到本线程的原子变量
    void onWsConnectedChanged(bool connected);
};

#endif // NETWORKMANAGER_H
