// ============================================================
#include <QDebug>
// networkmanager.cpp — 网络管理器代理层实现
// ============================================================
// 【Qt 学习笔记 — QThread + QMetaObject::invokeMethod 跨线程调度】
//
// invokeMethod 的三种用法：
//   (1) QMetaObject::invokeMethod(obj, "slotName", Qt::QueuedConnection, args)
//       — 老方式，参数需为 QGenericArgument，繁琐
//   (2) QMetaObject::invokeMethod(obj, [lambda], Qt::QueuedConnection)
//       — Qt5.10+，可用 lambda 调任意方法
//   (3) 直接连接一个内部信号到 Worker 的槽
//       — 需要额外定义信号，但连接后使用简单
//
// 本项目使用方式 (2)，因为简洁且类型安全。
//
// 关键要点：
//   - QThread::start() 启动线程的事件循环
//   - moveToThread() 必须在 start() 之前调用
//   - Worker 的 QWebSocket / QNetworkAccessManager 在 Worker
//     的构造函数中创建，但构造函数在 moveToThread 之前执行？
//     不是——Worker 的构造函数在主线程执行！
//     所以 QWebSocket 和 QNetworkAccessManager 必须延迟创建，
//     或者在 Worker 的 doConnectWebSocket() 中首次使用时创建。
// ============================================================

#include "networkmanager.h"
#include "networkworker.h"
#include <QMetaObject>
#include <QDebug>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
{
    m_wsConnected.storeRelaxed(0);

    // 1. 创建 Worker（此时还在主线程）
    QString httpHost = qEnvironmentVariable("WUZIQI_HTTP", "http://127.0.0.1:7999");
    QString wsHost  = qEnvironmentVariable("WUZIQI_WS",   "ws://127.0.0.1:7999/ws");
    m_worker = new NetworkWorker(httpHost, wsHost);
    // Worker 中 m_http / m_ws 会在 doConnectWebSocket 等首次调用时创建

    // 2. 创建线程并将 Worker 移过去
    m_workerThread = new QThread(this);
    m_worker->moveToThread(m_workerThread);

    // 当线程结束时自动清理 Worker
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // ----- 转发 Worker 的所有信号到本层的同名信号 -----
    // HTTP 响应
    connect(m_worker, &NetworkWorker::registerResult,
            this, &NetworkManager::registerResult);
    connect(m_worker, &NetworkWorker::loginResult,
            this, &NetworkManager::loginResult);
    connect(m_worker, &NetworkWorker::verifyResult,
            this, &NetworkManager::verifyResult);
    connect(m_worker, &NetworkWorker::randomMatchResult,
            this, &NetworkManager::randomMatchResult);
    connect(m_worker, &NetworkWorker::cancelMatchResult,
            this, &NetworkManager::cancelMatchResult);
    connect(m_worker, &NetworkWorker::createRoomResult,
            this, &NetworkManager::createRoomResult);
    connect(m_worker, &NetworkWorker::joinRoomResult,
            this, &NetworkManager::joinRoomResult);
    connect(m_worker, &NetworkWorker::matchStatusResult,
            this, &NetworkManager::matchStatusResult);
    connect(m_worker, &NetworkWorker::quitRoomResult,
            this, &NetworkManager::quitRoomResult);

    // WebSocket 事件
    connect(m_worker, &NetworkWorker::wsConnectedChanged,
            this, &NetworkManager::onWsConnectedChanged);
    connect(m_worker, &NetworkWorker::wsLoginResult,
            this, &NetworkManager::wsLoginResult);
    connect(m_worker, &NetworkWorker::wsRoomReady,
            this, &NetworkManager::wsRoomReady);
    connect(m_worker, &NetworkWorker::wsMoveReceived,
            this, &NetworkManager::wsMoveReceived);
    connect(m_worker, &NetworkWorker::wsOpponentMove,
            this, &NetworkManager::wsOpponentMove);
    connect(m_worker, &NetworkWorker::wsGameOver,
            this, &NetworkManager::wsGameOver);
    connect(m_worker, &NetworkWorker::wsOpponentQuit,
            this, &NetworkManager::wsOpponentQuit);
    connect(m_worker, &NetworkWorker::wsError,
            this, &NetworkManager::wsError);

    // 3. 启动线程
    m_workerThread->start();
}

NetworkManager::~NetworkManager()
{
    // 通知 Worker 断开 WS
    disconnectWebSocket();

    // 退出线程事件循环并等待结束
    m_workerThread->quit();
    m_workerThread->wait(3000);
}

// ========== HTTP API 转发 ==========

void NetworkManager::registerUser(const QString &username, const QString &password)
{
    QMetaObject::invokeMethod(m_worker, [this, username, password]() {
        m_worker->doRegisterUser(username, password);
    }, Qt::QueuedConnection);
}

void NetworkManager::loginUser(const QString &username, const QString &password)
{
    qDebug() << "[NetMgr] 转发 loginUser 到 Worker";
    QMetaObject::invokeMethod(m_worker, [this, username, password]() {
        m_worker->doLoginUser(username, password);
    }, Qt::QueuedConnection);
}

void NetworkManager::verifyToken(const QString &token)
{
    QMetaObject::invokeMethod(m_worker, [this, token]() {
        m_worker->doVerifyToken(token);
    }, Qt::QueuedConnection);
}

void NetworkManager::randomMatch(int userId)
{
    qDebug() << "[NetMgr] 转发 randomMatch 到 Worker, userId=" << userId;
    QMetaObject::invokeMethod(m_worker, [this, userId]() {
        m_worker->doRandomMatch(userId);
    }, Qt::QueuedConnection);
}

void NetworkManager::cancelMatch(int userId)
{
    QMetaObject::invokeMethod(m_worker, [this, userId]() {
        m_worker->doCancelMatch(userId);
    }, Qt::QueuedConnection);
}

void NetworkManager::createRoom(int userId)
{
    QMetaObject::invokeMethod(m_worker, [this, userId]() {
        m_worker->doCreateRoom(userId);
    }, Qt::QueuedConnection);
}

void NetworkManager::joinRoom(int userId, const QString &inviteCode)
{
    QMetaObject::invokeMethod(m_worker, [this, userId, inviteCode]() {
        m_worker->doJoinRoom(userId, inviteCode);
    }, Qt::QueuedConnection);
}

void NetworkManager::getMatchStatus(int userId)
{
    qDebug() << "[NetMgr] 转发 getMatchStatus 到 Worker, userId=" << userId;
    QMetaObject::invokeMethod(m_worker, [this, userId]() {
        m_worker->doGetMatchStatus(userId);
    }, Qt::QueuedConnection);
}

void NetworkManager::quitRoom(int userId, const QString &roomId)
{
    QMetaObject::invokeMethod(m_worker, [this, userId, roomId]() {
        m_worker->doQuitRoom(userId, roomId);
    }, Qt::QueuedConnection);
}

// ========== WebSocket API 转发 ==========

void NetworkManager::connectWebSocket()
{
    QMetaObject::invokeMethod(m_worker, [this]() {
        m_worker->doConnectWebSocket();
    }, Qt::QueuedConnection);
}

void NetworkManager::disconnectWebSocket()
{
    QMetaObject::invokeMethod(m_worker, [this]() {
        m_worker->doDisconnectWebSocket();
    }, Qt::QueuedConnection);
}

void NetworkManager::wsSendMove(const QString &roomId, int x, int y)
{
    QMetaObject::invokeMethod(m_worker, [this, roomId, x, y]() {
        m_worker->doWsSendMove(roomId, x, y);
    }, Qt::QueuedConnection);
}

void NetworkManager::wsQuitRoom(const QString &roomId)
{
    QMetaObject::invokeMethod(m_worker, [this, roomId]() {
        m_worker->doWsQuitRoom(roomId);
    }, Qt::QueuedConnection);
}

// ========== 内部槽 ==========

void NetworkManager::onWsConnectedChanged(bool connected)
{
    m_wsConnected.storeRelaxed(connected ? 1 : 0);
    emit wsConnectedChanged(connected);

    // WebSocket 连接成功后，如果已有 token，自动发送 WS 登录
    if (connected) {
        // 复制 token 到栈上再传给 Worker（避免跨线程访问 m_storedToken 的竞态）
        QString tokenCopy = m_storedToken;
        if (!tokenCopy.isEmpty()) {
            QMetaObject::invokeMethod(m_worker, [this, tokenCopy]() {
                qDebug() << "[Worker] 自动发送 WS 登录...";
                m_worker->doWsLogin(tokenCopy);
            }, Qt::QueuedConnection);
        }
    }
}
