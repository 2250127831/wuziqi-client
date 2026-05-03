// ============================================================
// networkworker.h — 网络工作线程（后台执行 HTTP + WebSocket）
// ============================================================
// 【Qt 学习笔记 — QThread + QObject::moveToThread】
//
// 设计思路：
//   NetworkWorker 是一个纯 QObject（无 parent），通过
//   moveToThread() 移入 QThread 后台线程。它拥有所有网络
//   对象（QNetworkAccessManager、QWebSocket），这些对象
//   必须和 Worker 在同一线程（否则信号/槽连接不安全）。
//
//   主线程通过 QMetaObject::invokeMethod(..., Qt::QueuedConnection)
//   调用 Worker 的槽函数；Worker 的 emit 信号自动以
//   Qt::QueuedConnection 方式投递到主线程（因为发送者和
//   接收者不在同一线程）。
// ============================================================

#ifndef NETWORKWORKER_H
#define NETWORKWORKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QAtomicInt>

class NetworkWorker : public QObject
{
    Q_OBJECT

public:
    explicit NetworkWorker(const QString &baseUrl, const QString &wsUrl, QObject *parent = nullptr);
    ~NetworkWorker() override;

signals:
    // --- HTTP 响应信号（跨线程投递到主线程） ---
    void registerResult(bool success, const QString &msg);
    void loginResult(bool success, const QString &token, int userId, const QString &msg);
    void verifyResult(bool success, int userId);
    void randomMatchResult(bool success, const QString &msg, const QString &roomId);
    void cancelMatchResult(bool success, const QString &msg);
    void createRoomResult(bool success, const QString &roomId, const QString &inviteCode);
    void joinRoomResult(bool success, const QString &roomId, const QString &msg);
    void matchStatusResult(bool success, bool inRoom, const QString &roomId);
    void quitRoomResult(bool success, const QString &msg);

    // --- WebSocket 信号（跨线程投递） ---
    void wsConnectedChanged(bool connected);   // 连接状态变化
    void wsLoginResult(bool success);
    void wsRoomReady(const QString &roomId, int player1, int player2);
    void wsMoveReceived(bool success, int x, int y);
    void wsOpponentMove(int x, int y);
    void wsGameOver(const QString &winner, const QString &reason);
    void wsOpponentQuit(const QString &roomId, int userId);
    void wsError(const QString &msg);

public slots:
    // 这些槽在后台线程执行，不应直接调用——通过 invokeMethod 跨线程调度
    void doRegisterUser(const QString &username, const QString &password);
    void doLoginUser(const QString &username, const QString &password);
    void doVerifyToken(const QString &token);
    void doRandomMatch(int userId);
    void doCancelMatch(int userId);
    void doCreateRoom(int userId);
    void doJoinRoom(int userId, const QString &inviteCode);
    void doGetMatchStatus(int userId);
    void doQuitRoom(int userId, const QString &roomId);

    void doConnectWebSocket();
    void doDisconnectWebSocket();
    void doWsLogin(const QString &token);
    void doWsSendMove(const QString &roomId, int x, int y);
    void doWsQuitRoom(const QString &roomId);

private slots:
    void onWsConnected();
    void onWsDisconnected();
    void onWsTextMessage(const QString &message);
    void onWsError(QAbstractSocket::SocketError error);

private:
    // --- 网络对象（必须在 Worker 所属线程创建） ---
    QNetworkAccessManager *m_http = nullptr;
    QWebSocket *m_ws = nullptr;

    QString m_baseUrl;
    QString m_wsUrl;

    // WebSocket 连接状态（原子变量，跨线程安全直接读）
    QAtomicInt m_wsConnectedFlag;  // 0=断开, 1=已连接

public:
    bool isWsConnected() const { return m_wsConnectedFlag.loadRelaxed() != 0; }

    // 发送异步 HTTP POST，返回 QNetworkReply*（调用者连接 finished 信号解析）
    QNetworkReply* sendPost(const QString &path, const QJsonObject &body);

    // 解析 WebSocket 消息并分发
    void handleWsMessage(const QJsonObject &json);
};

#endif // NETWORKWORKER_H
