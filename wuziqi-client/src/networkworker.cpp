// ============================================================
// networkworker.cpp — 网络工作线程实现
// ============================================================
// 【Qt 学习笔记 — 异步 HTTP + 跨线程信号】
//
// 核心变化（对比 QEventLoop 同步版）：
//   每个 doXxx() 方法发送 HTTP POST 后不阻塞等待，
//   而是连接 reply->finished 到 lambda，请求完成后自动解析。
//   解析结果通过信号 emit 回到主线程。
//
//   这样：
//   - 后台线程不会阻塞（虽然它在单独线程里，但请求是异步的）
//   - 主线程永远不会被网络 I/O 卡住
//   - QWebSocket 也在后台线程，不影响主线程 UI
// ============================================================

#include "networkworker.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

NetworkWorker::NetworkWorker(const QString &baseUrl, const QString &wsUrl, QObject *parent)
    : QObject(parent)
    , m_baseUrl(baseUrl)
    , m_wsUrl(wsUrl)
{
    m_wsConnectedFlag.storeRelaxed(0);
}

NetworkWorker::~NetworkWorker()
{
    // m_http 和 m_ws 没有 parent，需要手动清理
    if (m_ws) {
        m_ws->close();
        delete m_ws;
        m_ws = nullptr;
    }
    delete m_http;
    m_http = nullptr;
}

// ========== HTTP POST 辅助 ==========

QNetworkReply* NetworkWorker::sendPost(const QString &path, const QJsonObject &body)
{
    // 延迟创建 QNetworkAccessManager（在后台线程首次使用时创建）
    // 不能在构造函数中创建，因为构造函数在主线程执行
    if (!m_http) {
        m_http = new QNetworkAccessManager();
    }

    // 每次调用创建新的 request，因为 QNetworkRequest 是一次性的
    QNetworkRequest request(QUrl(m_baseUrl + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(5000);  // 5s 超时

    QByteArray data = QJsonDocument(body).toJson(QJsonDocument::Compact);
    qDebug() << "[Worker] HTTP POST" << path;
    return m_http->post(request, data);
}

// ========== HTTP 请求槽函数（后台线程） ==========

void NetworkWorker::doRegisterUser(const QString &username, const QString &password)
{
    QJsonObject body;
    body["username"] = username;
    body["password"] = password;

    QNetworkReply *reply = sendPost("/api/register", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QString resp;
        if (reply->error() == QNetworkReply::NoError) {
            resp = QString::fromUtf8(reply->readAll());
        qDebug() << "[Worker] RAW response:" << resp;
            QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8());
            QJsonObject json = doc.object();
            bool ok = json["success"].toBool();
            QString msg = json["message"].toString(ok ? "注册成功" : "注册失败");
            emit registerResult(ok, msg);
        } else {
            emit registerResult(false, reply->errorString());
        }
        reply->deleteLater();
    });
}

void NetworkWorker::doLoginUser(const QString &username, const QString &password)
{
    QJsonObject body;
    body["username"] = username;
    body["password"] = password;

    QNetworkReply *reply = sendPost("/api/login", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit loginResult(false, "", -1, reply->errorString());
        } else {
            QString resp = QString::fromUtf8(reply->readAll());
        qDebug() << "[Worker] RAW response:" << resp;
            QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8());
            QJsonObject json = doc.object();
            bool ok = json["success"].toBool();
            if (ok) {
                QJsonObject data = json["data"].toObject();
                QString token = data["token"].toString();
                int userId = data["user_id"].toInt();
                emit loginResult(true, token, userId, "登录成功");
            } else {
                QString msg = json["message"].toString("登录失败");
                emit loginResult(false, "", -1, msg);
            }
        }
        reply->deleteLater();
    });
}

void NetworkWorker::doVerifyToken(const QString &token)
{
    QJsonObject body;
    body["token"] = token;
    QNetworkReply *reply = sendPost("/api/verify", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit verifyResult(false, -1);
        } else {
            QString resp = QString::fromUtf8(reply->readAll());
        qDebug() << "[Worker] RAW response:" << resp;
            QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8());
            QJsonObject json = doc.object();
            bool ok = json["success"].toBool();
            int userId = -1;
            if (ok) {
                userId = json["data"].toObject()["user_id"].toInt();
            }
            emit verifyResult(ok, userId);
        }
        reply->deleteLater();
    });
}

void NetworkWorker::doRandomMatch(int userId)
{
    qDebug() << "[Worker] doRandomMatch, userId=" << userId;
    QJsonObject body;
    body["user_id"] = userId;
    QNetworkReply *reply = sendPost("/api/match/random_match", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit randomMatchResult(false, reply->errorString(), "");
        } else {
            QString resp = QString::fromUtf8(reply->readAll());
        qDebug() << "[Worker] RAW response:" << resp;
            QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8());
            QJsonObject json = doc.object();
            bool ok = json["success"].toBool();
            QString msg = json["message"].toString();
            QString roomId;
            if (ok && json.contains("data")) {
                roomId = json["data"].toObject()["room_id"].toString();
            }
            emit randomMatchResult(ok, msg, roomId);
        }
        reply->deleteLater();
    });
}

void NetworkWorker::doCancelMatch(int userId)
{
    QJsonObject body;
    body["user_id"] = userId;
    QNetworkReply *reply = sendPost("/api/match/cancel_match", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QString resp = QString::fromUtf8(reply->readAll());
        qDebug() << "[Worker] RAW response:" << resp;
        QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8());
        QJsonObject json = doc.object();
        bool ok = json["success"].toBool();
        QString msg = json["message"].toString("已取消匹配");
        emit cancelMatchResult(ok, msg);
        reply->deleteLater();
    });
}

void NetworkWorker::doCreateRoom(int userId)
{
    QJsonObject body;
    body["user_id"] = userId;
    QNetworkReply *reply = sendPost("/api/match/create_room", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit createRoomResult(false, "", "");
        } else {
            QString resp = QString::fromUtf8(reply->readAll());
        qDebug() << "[Worker] RAW response:" << resp;
            QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8());
            QJsonObject json = doc.object();
            bool ok = json["success"].toBool();
            QString roomId, inviteCode;
            if (ok && json.contains("data")) {
                QJsonObject data = json["data"].toObject();
                roomId = data["room_id"].toString();
                inviteCode = data["invite_code"].toString();
            }
            emit createRoomResult(ok, roomId, inviteCode);
        }
        reply->deleteLater();
    });
}

void NetworkWorker::doJoinRoom(int userId, const QString &inviteCode)
{
    QJsonObject body;
    body["user_id"] = userId;
    body["invite_code"] = inviteCode;
    QNetworkReply *reply = sendPost("/api/match/join_room", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit joinRoomResult(false, "", reply->errorString());
        } else {
            QString resp = QString::fromUtf8(reply->readAll());
        qDebug() << "[Worker] RAW response:" << resp;
            QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8());
            QJsonObject json = doc.object();
            bool ok = json["success"].toBool();
            QString msg = json["message"].toString();
            QString roomId;
            if (ok && json.contains("data")) {
                roomId = json["data"].toObject()["room_id"].toString();
            }
            emit joinRoomResult(ok, roomId, msg);
        }
        reply->deleteLater();
    });
}

void NetworkWorker::doGetMatchStatus(int userId)
{
    qDebug() << "[Worker] doGetMatchStatus, userId=" << userId;
    QJsonObject body;
    body["user_id"] = userId;
    QNetworkReply *reply = sendPost("/api/match/match_status", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit matchStatusResult(false, false, "");
        } else {
            QString resp = QString::fromUtf8(reply->readAll());
        qDebug() << "[Worker] RAW response:" << resp;
            QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8());
            QJsonObject json = doc.object();
            bool ok = json["success"].toBool();
            if (ok) {
                QJsonObject data = json["data"].toObject();
                bool inRoom = data["in_room"].toBool();
                QString roomId = data["room_id"].toString();
                emit matchStatusResult(true, inRoom, roomId);
            } else {
                emit matchStatusResult(true, false, "");
            }
        }
        reply->deleteLater();
    });
}

void NetworkWorker::doQuitRoom(int userId, const QString &roomId)
{
    QJsonObject body;
    body["user_id"] = userId;
    body["room_id"] = roomId;
    QNetworkReply *reply = sendPost("/api/match/quit_room", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit quitRoomResult(false, reply->errorString());
        } else {
            QString resp = QString::fromUtf8(reply->readAll());
        qDebug() << "[Worker] RAW response:" << resp;
            QJsonDocument doc = QJsonDocument::fromJson(resp.toUtf8());
            QJsonObject json = doc.object();
            bool ok = json["success"].toBool();
            QString msg = json["message"].toString();
            emit quitRoomResult(ok, msg);
        }
        reply->deleteLater();
    });
}

// ========== WebSocket（后台线程） ==========

void NetworkWorker::doConnectWebSocket()
{
    // WebSocket 对象必须在 Worker 线程创建
    if (!m_ws) {
        m_ws = new QWebSocket();   // 无 parent——手工管理生命周期
        connect(m_ws, &QWebSocket::connected, this, &NetworkWorker::onWsConnected);
        connect(m_ws, &QWebSocket::disconnected, this, &NetworkWorker::onWsDisconnected);
        connect(m_ws, &QWebSocket::textMessageReceived, this, &NetworkWorker::onWsTextMessage);
        connect(m_ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                this, &NetworkWorker::onWsError);
    }
    m_ws->open(QUrl(m_wsUrl));
}

void NetworkWorker::doDisconnectWebSocket()
{
    if (m_ws) {
        m_ws->close();
    }
}

void NetworkWorker::doWsLogin(const QString &token)
{
    if (!m_ws || m_ws->state() != QAbstractSocket::ConnectedState) return;

    QJsonObject msg, data;
    data["token"] = token;
    msg["type"] = "login";
    msg["data"] = data;

    QString jsonStr = QString::fromUtf8(
        QJsonDocument(msg).toJson(QJsonDocument::Compact));
    m_ws->sendTextMessage(jsonStr);
}

void NetworkWorker::doWsSendMove(const QString &roomId, int x, int y)
{
    if (!m_ws) return;
    QJsonObject msg, data;
    data["room_id"] = roomId;
    data["x"] = x;
    data["y"] = y;
    msg["type"] = "move";
    msg["data"] = data;
    QString jsonStr = QString::fromUtf8(
        QJsonDocument(msg).toJson(QJsonDocument::Compact));
    m_ws->sendTextMessage(jsonStr);
}

void NetworkWorker::doWsQuitRoom(const QString &roomId)
{
    if (!m_ws) return;
    QJsonObject msg, data;
    data["room_id"] = roomId;
    msg["type"] = "quit_room";
    msg["data"] = data;
    QString jsonStr = QString::fromUtf8(
        QJsonDocument(msg).toJson(QJsonDocument::Compact));
    m_ws->sendTextMessage(jsonStr);
}

// ========== WebSocket 事件 ==========

void NetworkWorker::onWsConnected()
{
    qDebug() << "[Worker] WebSocket 已连接";
    m_wsConnectedFlag.storeRelaxed(1);
    emit wsConnectedChanged(true);
}

void NetworkWorker::onWsDisconnected()
{
    qDebug() << "[Worker] WebSocket 已断开";
    m_wsConnectedFlag.storeRelaxed(0);
    emit wsConnectedChanged(false);
}

void NetworkWorker::onWsTextMessage(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull()) {
        qWarning() << "[Worker] WebSocket JSON 解析失败";
        return;
    }
    handleWsMessage(doc.object());
}

void NetworkWorker::onWsError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    qWarning() << "[Worker] WebSocket 错误:" << (m_ws ? m_ws->errorString() : "unknown");
    emit wsError(m_ws ? m_ws->errorString() : "unknown");
}

void NetworkWorker::handleWsMessage(const QJsonObject &json)
{
    QString type = json["type"].toString();

    if (type == "login_success") {
        bool ok = json["success"].toBool();
        qDebug() << "[Worker] WS 登录结果: success=" << ok;
        emit wsLoginResult(ok);
    } else if (type == "login_failed") {
        qDebug() << "[Worker] WS 登录失败";
        emit wsLoginResult(false);
    } else if (type == "room_ready") {
        QJsonObject data = json["data"].toObject();
        qDebug() << "[Worker] 收到 room_ready: room=" << data["room_id"].toString();
        emit wsRoomReady(
            data["room_id"].toString(),
            data["player1"].toInt(),
            data["player2"].toInt());
    } else if (type == "move_received") {
        int x = json["data"].toObject()["x"].toInt();
        int y = json["data"].toObject()["y"].toInt();
        emit wsMoveReceived(json["success"].toBool(), x, y);
    } else if (type == "move_refused") {
        emit wsMoveReceived(false, -1, -1);
    } else if (type == "opponent_move") {
        QJsonObject data = json["data"].toObject();
        emit wsOpponentMove(data["x"].toInt(), data["y"].toInt());
    } else if (type == "game_over") {
        QJsonObject data = json["data"].toObject();
        emit wsGameOver(data["winner"].toString(), data["reason"].toString());
    } else if (type == "opponent_quit") {
        QJsonObject data = json["data"].toObject();
        emit wsOpponentQuit(data["room_id"].toString(), data["user_id"].toInt());
    } else {
        qDebug() << "[Worker] 未处理的 WS 类型:" << type;
    }
}
