// ============================================================
// gamewidget.cpp — 棋盘游戏实现（核心：自定义绘制 + 事件处理）
// ============================================================
// 【Qt 学习笔记 — QPainter 高级绘制 / 事件系统 / 坐标转换】
//
// ═══════════════════════════════════════════════════════════
// 1. QPainter 绘制引擎
// ═══════════════════════════════════════════════════════════
//
// QPainter 是 Qt 的 2D 绘图引擎，必须在 paintEvent 中使用。
// 使用步骤：
//   ① QPainter painter(this);        // 创建画笔，绑定到 widget
//   ② painter.setRenderHint(...);    // 设置渲染质量（抗锯齿等）
//   ③ painter.setPen(pen);           // 设置"钢笔"（线条颜色/粗细）
//   ④ painter.setBrush(brush);       // 设置"画刷"（填充颜色/渐变）
//   ⑤ painter.drawLine/rect/ellipse  // 实际绘制
//   ⑥ painter 析构时自动结束绘制
//
// 重要：不要手动调用 paintEvent()！需要重绘时调用 update()。
//       Qt 会合并多次 update() 调用，避免频繁重绘。
//
// QPainter 渲染提示（RenderHint）：
// - Antialiasing:      抗锯齿（让曲线/斜线更平滑）
// - TextAntialiasing:  文字抗锯齿
// - SmoothPixmapTransform:  平滑图片缩放
// - LosslessImageRendering: 无损图片渲染
//
// ═══════════════════════════════════════════════════════════
// 2. 渐变色（Gradient）
// ═══════════════════════════════════════════════════════════
//
// Qt 支持三种渐变：
// (a) QLinearGradient — 线性渐变（沿直线方向渐变）
//     构造函数：QLinearGradient(startPoint, endPoint)
//     应用：金属拉丝、玻璃反光效果
//
// (b) QRadialGradient — 径向渐变（从中心向周围扩散）
//     构造函数：QRadialGradient(center, radius)
//     应用：围棋子的立体效果（我们用的就是这个！）
//
// (c) QConicalGradient — 锥形渐变（围绕中心旋转渐变）
//     构造函数：QConicalGradient(center, angle)
//     应用：仪表盘、色轮
//
// 使用方法示例（黑棋子立体效果）：
//   QRadialGradient grad(p.x()-3, p.y()-3, radius);
//   grad.setColorAt(0.0, QColor(0x66,0x66,0x66));  // 中心高光
//   grad.setColorAt(1.0, QColor(0x11,0x11,0x11));  // 边缘深色
//   painter.setBrush(grad);
//   painter.drawEllipse(p, radius, radius);
// 效果：光源在左上方，产生 3D 立体棋子效果
//
// ═══════════════════════════════════════════════════════════
// 3. 自定义 paintEvent 最佳实践
// ═══════════════════════════════════════════════════════════
//
// - 保持 paintEvent 轻盈：不要做复杂计算、网络请求、文件读写
// - 提前计算好，绘制时只做"画"的工作
// - 使用 Q_UNUSED(event) 忽略 paintEvent 参数
// - 所有绘制代码必须在 paintEvent 中，不要在其他地方绘制
// - 可以用 bool 标记控制哪些部分需要绘制（全量 vs 增量）
//
// ═══════════════════════════════════════════════════════════
// 4. 鼠标事件中的坐标转换
// ═══════════════════════════════════════════════════════════
//
// 像素坐标 → 棋盘坐标：
//   gridX = round((pixelX - MARGIN) / CELL_SIZE)
//   gridY = round((pixelY - MARGIN) / CELL_SIZE)
// 必须验证棋盘边界和点击精度（点击容差 ≤ CELL_SIZE/2）
//
// 棋盘坐标 → 像素坐标：
//   pixelX = MARGIN + gridX * CELL_SIZE
//   pixelY = MARGIN + gridY * CELL_SIZE
//
// 五子棋画的 intersection（交叉点），不是格子内部。
// 所以上面用的是"格数 × 格距"，而不是"格数 × 格距 + 半格"。
//
// ═══════════════════════════════════════════════════════════
// 5. QRadialGradient 实现立体棋子
// ═══════════════════════════════════════════════════════════
//
// 黑棋：灰色中心→黑色边缘（看起来凸起）
// 白棋：白色中心→浅灰边缘（看起来凸起）
// 光源偏移 (-3,-3) 模拟右上角光源
// ============================================================

#include "gamewidget.h"
#include "networkmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>     // 鼠标事件
#include <QFont>
#include <QMessageBox>     // 消息弹窗
#include <QtMath>          // qRound() 四舍五入

GameWidget::GameWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent)
    , m_net(net)
{
    // === 主布局 ===
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // 顶部信息栏：显示棋子颜色 + 退出按钮
    auto *infoBar = new QHBoxLayout();

    m_infoLabel = new QLabel("等待开始...", this);
    QFont infoFont;
    infoFont.setPointSize(16);
    infoFont.setBold(true);
    m_infoLabel->setFont(infoFont);
    infoBar->addWidget(m_infoLabel);

    infoBar->addStretch();

    m_quitBtn = new QPushButton("退出房间", this);
    connect(m_quitBtn, &QPushButton::clicked, this, &GameWidget::onQuitClicked);
    infoBar->addWidget(m_quitBtn);
    mainLayout->addLayout(infoBar);

    // 状态提示行
    m_statusLabel = new QLabel("", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #666; font-size: 13px;");
    mainLayout->addWidget(m_statusLabel);

    // 弹性空白：棋盘绘制区域占满剩余空间
    // 棋盘本身在 paintEvent 中通过 QPainter 画的，不是控件
    mainLayout->addStretch(1);

    // 计算最小窗口尺寸：棋盘大小 + 顶部 UI 高度
    int boardPixels = (BOARD_SIZE - 1) * CELL_SIZE + MARGIN * 2;
    setMinimumSize(boardPixels, boardPixels + 80);

    // === WebSocket 事件绑定 ===
    connect(m_net, &NetworkManager::wsMoveReceived, this, &GameWidget::onWsMoveReceived);
    connect(m_net, &NetworkManager::wsOpponentMove, this, &GameWidget::onWsOpponentMove);
    connect(m_net, &NetworkManager::wsGameOver, this, &GameWidget::onWsGameOver);
    connect(m_net, &NetworkManager::wsOpponentQuit, this, &GameWidget::onWsOpponentQuit);
}

void GameWidget::startGame(int userId, const QString &roomId, const QString &myColor)
{
    // 颜色由大厅通过 room_ready 算出并传过来，不会有时序问题
    m_userId = userId;
    m_roomId = roomId;
    m_myColor = myColor;
    resetGame();
}

void GameWidget::resetGame()
{
    // memset: 将棋盘全部清空（0=空）
    // 注意：memset 只适合 POD 类型，int 数组没问题
    memset(m_board, 0, sizeof(m_board));

    m_hoverX = m_hoverY = -1;
    m_turnColor = "black";  // 五子棋黑方先走

    // 还没分配颜色时显示"等待"
    if (m_myColor.isEmpty()) {
        m_infoLabel->setText("等待分配棋子...");
        m_statusLabel->setText("");
    } else {
        QString colorText = (m_myColor == "black") ? "黑棋" : "白棋";
        bool myTurn = (m_myColor == "black");
        m_infoLabel->setText(QString("你是 %1").arg(colorText));
        setMyTurn(myTurn);
    }

    // 请求 Qt 重绘（update() 触发异步 paintEvent 调用）
    update();
}

// ========== 坐标转换 ==========

QPoint GameWidget::boardToPixel(int x, int y) const
{
    // 棋盘坐标 (x,y) → 屏幕像素坐标 (px,py)
    // 例如 (0,0) → (MARGIN, MARGIN)，(7,7) → 棋盘中心
    return QPoint(MARGIN + x * CELL_SIZE, MARGIN + y * CELL_SIZE);
}

bool GameWidget::pixelToBoard(int px, int py, int &x, int &y) const
{
    // 屏幕像素坐标 → 棋盘坐标
    // 计算相对棋盘原点的偏移
    int cx = px - MARGIN;
    int cy = py - MARGIN;

    // 太靠近边缘则返回 false
    if (cx < -CELL_SIZE/2 || cy < -CELL_SIZE/2) return false;

    // 四舍五入取最近的交叉点
    // qRound() 来自 <QtMath>，比 (int)(cx/CELL_SIZE + 0.5) 更精确
    int ix = qRound(cx / (double)CELL_SIZE);
    int iy = qRound(cy / (double)CELL_SIZE);

    // 边界检查
    if (ix < 0 || ix >= BOARD_SIZE || iy < 0 || iy >= BOARD_SIZE) return false;

    // 点击精度校验：必须在交叉点半格距离内
    int dx = qAbs(px - (MARGIN + ix * CELL_SIZE));
    int dy = qAbs(py - (MARGIN + iy * CELL_SIZE));
    if (dx > CELL_SIZE / 2 || dy > CELL_SIZE / 2) return false;

    x = ix;
    y = iy;
    return true;
}

// ========== paintEvent — 核心绘制方法 ==========

void GameWidget::paintEvent(QPaintEvent *event)
{
    // Q_UNUSED: 避免"未使用参数"警告
    Q_UNUSED(event);

    // 1. 创建 QPainter 对象（构造时自动开始绘制，析构时自动结束）
    QPainter painter(this);

    // 2. 开启抗锯齿（让棋子和线条边缘更平滑）
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int boardW = (BOARD_SIZE - 1) * CELL_SIZE;  // 棋盘总宽度（像素）

    // ---- (1) 绘制棋盘背景 ----
    // QColor(r,g,b) 或 QColor("#DEB887")
    // fillRect(x, y, w, h, color) 填充整个矩形区域
    painter.fillRect(0, 0, w, height(), QColor(0xDE, 0xB8, 0x87));  // 木色（#DEB887）

    // ---- (2) 绘制网格线 ----
    // QPen(color, width, style) — 设置线条属性
    // style: SolidLine(实线), DashLine(虚线), DotLine(点线) ...
    painter.setPen(QPen(QColor(0x55, 0x3A, 0x1F), 1));  // 深褐色细线

    // 画横线和竖线（15×15 棋盘 = 14条横线 + 14条竖线）
    for (int i = 0; i < BOARD_SIZE; i++) {
        // 横线
        int y = MARGIN + i * CELL_SIZE;
        painter.drawLine(MARGIN, y, MARGIN + boardW, y);

        // 竖线
        int x = MARGIN + i * CELL_SIZE;
        painter.drawLine(x, MARGIN, x, MARGIN + boardW);
    }

    // ---- (3) 绘制星位（天元 + 四个星位） ----
    QVector<QPair<int,int>> stars = {
        {7,7},      // 天元（棋盘正中央）
        {3,3},      // 左上星位
        {3,11},     // 左下星位
        {11,3},     // 右上星位
        {11,11}     // 右下星位
    };
    painter.setBrush(Qt::black);    // 实心黑色
    // drawEllipse(center, rx, ry) 画实心圆点
    for (auto &s : stars) {
        QPoint p = boardToPixel(s.first, s.second);
        painter.drawEllipse(p, 4, 4);   // 半径 4px 的圆点
    }

    // ---- (4) 绘制棋子 ----
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            if (m_board[x][y] == 0) continue;  // 空位跳过

            QPoint p = boardToPixel(x, y);
            int radius = CELL_SIZE / 2 - 3;     // 棋子半径（比半格略小）

            if (m_board[x][y] == 1) {
                // === 黑棋：径向渐变实现立体效果 ===
                // 光源点偏移 (-3,-3)，模拟左上光
                QRadialGradient grad(p.x() - 3, p.y() - 3, radius);
                grad.setColorAt(0, QColor(0x66, 0x66, 0x66));  // 中心：亮灰
                grad.setColorAt(1, QColor(0x11, 0x11, 0x11));  // 边缘：暗黑
                painter.setBrush(grad);
            } else {
                // === 白棋 ===
                QRadialGradient grad(p.x() - 3, p.y() - 3, radius);
                grad.setColorAt(0, Qt::white);                   // 中心：纯白
                grad.setColorAt(1, QColor(0xCC, 0xCC, 0xCC));   // 边缘：浅灰
                painter.setBrush(grad);
            }
            painter.setPen(QPen(Qt::black, 1));
            painter.drawEllipse(p, radius, radius);
        }
    }

    // ---- (5) 鼠标悬停预览 ----
    // 当鼠标在合法空位悬停时，显示半透明棋子预览
    if (m_hoverX >= 0 && m_hoverY >= 0 &&
        m_board[m_hoverX][m_hoverY] == 0 &&  // 位置为空
        !m_myColor.isEmpty() &&               // 已经分配颜色
        m_myColor == m_turnColor) {           // 确实轮到我

        QPoint p = boardToPixel(m_hoverX, m_hoverY);
        int radius = CELL_SIZE / 2 - 3;
        // QColor(r,g,b,alpha) — alpha=60 表示半透明（0=完全透明, 255=不透明）
        painter.setBrush(QColor(0, 0, 0, 60));    // 半透明黑色
        painter.setPen(Qt::NoPen);                  // 无边框
        painter.drawEllipse(p, radius, radius);
    }

    // ---- (6) 绘制坐标标注 ----
    painter.setPen(QColor(0x55, 0x3A, 0x1F));
    QFont coordFont;
    coordFont.setPointSize(8);
    painter.setFont(coordFont);

    // 行：A B C D E F G H J K L M N O P (跳过 I)
    // 列：15 14 13 ... 1
    for (int i = 0; i < BOARD_SIZE; i++) {
        // 列标：A~O，跳过 I（传统五子棋约定）
        QString label = QString(QChar('A' + i));
        if (i >= 8) label = QString(QChar('A' + i + 1));  // 跳过 I
        QPoint p = boardToPixel(i, BOARD_SIZE - 1);
        painter.drawText(p.x() - 6, p.y() + 18, label);

        // 行号：15 14 13 ... 1
        QString num = QString::number(BOARD_SIZE - i);
        QPoint p2 = boardToPixel(0, i);
        painter.drawText(p2.x() - 20, p2.y() + 5, num);
    }
}

// ========== 鼠标事件 ==========

void GameWidget::mousePressEvent(QMouseEvent *event)
{
    // 只处理左键点击
    if (event->button() != Qt::LeftButton) return;

    int x, y;
    // 检查是否点到了棋盘交叉点
    if (!pixelToBoard(event->pos().x(), event->pos().y(), x, y)) return;

    // 三种拒绝落子的情况
    if (m_board[x][y] != 0) {
        m_statusLabel->setText("该位置已有棋子");
        return;
    }
    if (m_myColor.isEmpty() || m_myColor != m_turnColor) {
        m_statusLabel->setText("还没轮到你");
        return;
    }
    if (m_roomId.isEmpty()) return;

    // 发送落子到服务器（通过 WebSocket）
    m_net->wsSendMove(m_roomId, x, y);
    m_statusLabel->setText("等待服务器确认...");
}

void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    int x, y;
    if (pixelToBoard(event->pos().x(), event->pos().y(), x, y)) {
        // 鼠标在棋盘上 → 更新悬停位置并重绘
        if (x != m_hoverX || y != m_hoverY) {
            m_hoverX = x;
            m_hoverY = y;
            update();   // 触发 paintEvent，更新悬停预览
        }
    } else {
        // 鼠标移出棋盘 → 清除悬停
        if (m_hoverX >= 0 || m_hoverY >= 0) {
            m_hoverX = m_hoverY = -1;
            update();
        }
    }
}

// ========== 游戏逻辑 ==========

void GameWidget::placeStone(int x, int y, int player)
{
    m_board[x][y] = player;
    update();   // 棋盘变化 → 重绘
}

void GameWidget::setMyTurn(bool myTurn)
{
    // 更新当前轮到谁
    m_turnColor = myTurn ? m_myColor : (m_myColor == "black" ? "white" : "black");

    QString turnText;
    if (m_turnColor == "black") {
        turnText = "黑棋走";
    } else {
        turnText = "白棋走";
    }

    if (myTurn) {
        m_statusLabel->setStyleSheet("color: #c33; font-weight: bold; font-size: 14px;");
        m_statusLabel->setText("轮到你了 (" + turnText + ")");
    } else {
        m_statusLabel->setStyleSheet("color: #666; font-size: 14px;");
        m_statusLabel->setText("等待对手 (" + turnText + ")");
    }
}

bool GameWidget::checkWin(int x, int y, int player)
{
    // 四方向向量：水平、垂直、主对角线、副对角线
    int dx[] = {1, 0, 1, 1};
    int dy[] = {0, 1, 1, -1};

    for (int d = 0; d < 4; d++) {
        int count = 1;  // 当前棋子自身

        // 正方向延伸（最多 4 步）
        for (int step = 1; step < 5; step++) {
            int nx = x + dx[d] * step;
            int ny = y + dy[d] * step;
            if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
            if (m_board[nx][ny] != player) break;
            count++;
        }

        // 反方向延伸（最多 4 步）
        for (int step = 1; step < 5; step++) {
            int nx = x - dx[d] * step;
            int ny = y - dy[d] * step;
            if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
            if (m_board[nx][ny] != player) break;
            count++;
        }

        // 任意方向连成 5 子即为胜利
        if (count >= 5) return true;
    }
    return false;
}

// ========== WebSocket 回调 ==========


void GameWidget::onWsMoveReceived(bool success, int x, int y)
{
    if (!success) {
        m_statusLabel->setText("落子被拒绝");
        return;
    }

    // 确认落子（服务器确认后，才在本地棋盘上显示）
    int player = (m_myColor == "black") ? 1 : 2;
    placeStone(x, y, player);

    // 检查是否赢
    if (checkWin(x, y, player)) {
        m_statusLabel->setStyleSheet("color: #2a7; font-weight: bold; font-size: 16px;");
        m_statusLabel->setText("你赢了！🎉");
        m_infoLabel->setText("胜利！");
        return;
    }

    // 换对手走
    setMyTurn(false);
}

void GameWidget::onWsOpponentMove(int x, int y)
{
    // 对手落子
    int player = (m_myColor == "black") ? 2 : 1;
    placeStone(x, y, player);

    if (checkWin(x, y, player)) {
        m_statusLabel->setStyleSheet("color: #c33; font-weight: bold; font-size: 16px;");
        m_statusLabel->setText("你输了...");
        m_infoLabel->setText("失败");
        return;
    }

    // 轮到我了
    setMyTurn(true);
}

void GameWidget::onWsGameOver(const QString &winner, const QString &reason)
{
    QString winnerText;
    if (winner == m_myColor) {
        winnerText = "你赢了！🎉";
        m_statusLabel->setStyleSheet("color: #2a7; font-weight: bold; font-size: 16px;");
    } else {
        winnerText = "你输了...";
        m_statusLabel->setStyleSheet("color: #c33; font-weight: bold; font-size: 16px;");
    }
    m_infoLabel->setText(winnerText);
    if (!reason.isEmpty()) {
        m_statusLabel->setText(reason);
    }
}

void GameWidget::onWsOpponentQuit(const QString &roomId, int userId)
{
    Q_UNUSED(roomId);
    Q_UNUSED(userId);
    m_statusLabel->setStyleSheet("color: #c33; font-weight: bold; font-size: 16px;");
    m_statusLabel->setText("对手退出了游戏");
    m_infoLabel->setText("对方逃跑");
}

void GameWidget::onQuitClicked()
{
    if (!m_roomId.isEmpty()) {
        // 通过 WebSocket 通知服务器退出房间
        m_net->wsQuitRoom(m_roomId);
        // HTTP 退出（双重确认，确保服务器收到）
        m_net->quitRoom(m_userId, m_roomId);
    }
    m_roomId.clear();
    emit backToLobby();
}
