// ============================================================
// gamewidget.h — 棋盘游戏页面头文件
// ============================================================
// 【Qt 学习笔记 — 自定义绘制 / 事件重写 / 前向声明】
//
// 1. 自定义绘制（Custom Painting）
//    Qt 中自定义绘制需要：
//    (1) 继承 QWidget（或 QGLWidget/QOpenGLWidget）
//    (2) 重写 paintEvent(QPaintEvent*) 方法
//    (3) 使用 QPainter 在 paintEvent 中绘制
//
//    QPainter 提供丰富的 2D 绘图 API：
//    - 画线/点/矩形/椭圆/圆弧
//    - 渐变色（线性/径向/锥形）
//    - 文字渲染
//    - 图片/像素图绘制
//    - 坐标变换（平移/旋转/缩放）
//
//    重要规则：paintEvent 由 Qt 自动调用，不要手动调用！
//    需要触发重绘时调用 update()，Qt 会在合适时机合并重绘请求。
//
// 2. 事件重写（Event Override）
//    Qt 的事件系统基于虚函数重写：
//    - mousePressEvent(QMouseEvent*) — 鼠标按下
//    - mouseReleaseEvent(QMouseEvent*) — 鼠标释放
//    - mouseMoveEvent(QMouseEvent*) — 鼠标移动
//    - mouseDoubleClickEvent(QMouseEvent*) — 鼠标双击
//    - keyPressEvent(QKeyEvent*) — 键盘按下
//    - resizeEvent(QResizeEvent*) — 窗口大小变化
//    - paintEvent(QPaintEvent*) — 重绘
//
//    注意：mouseMoveEvent 默认只在按下时触发。
//    如果需要无按键状态跟踪鼠标，需要调用 setMouseTracking(true)。
//
// 3. static const int 成员变量
//    类的静态常量整数成员，在头文件中直接定义。
//    用于定义"全局常量"（棋盘大小、格子尺寸、边距等）。
//    比 #define BOARD_SIZE 15 更类型安全。
// ============================================================

#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>         // QWidget 基类（可以重写 paintEvent）
#include <QPainter>        // 2D 绘图引擎
#include <QVector>         // Qt 的泛型数组容器
#include <QPair>           // 键值对容器
#include <QLabel>
#include <QPushButton>

class NetworkManager;

class GameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget(NetworkManager *net, QWidget *parent = nullptr);

    // 开始游戏（由 MainWindow 或 WebSocket 回调调用）
    void startGame(int userId, const QString &roomId, const QString &myColor);

    // 重置棋盘
    void resetGame();

signals:
    void gameLeft();           // 离开游戏
    void backToLobby();        // 返回大厅

// ========== 事件重写（Qt 事件系统） ==========
protected:
    // paintEvent：核心绘制方法（QPainter 画棋盘、棋子）
    void paintEvent(QPaintEvent *event) override;

    // mousePressEvent：检测格子点击 → 落子
    void mousePressEvent(QMouseEvent *event) override;

    // mouseMoveEvent：跟踪鼠标 → 显示悬停预览
    void mouseMoveEvent(QMouseEvent *event) override;

// ========== WebSocket 回调槽 ==========
private slots:
    
    void onWsMoveReceived(bool success, int x, int y);
    void onWsOpponentMove(int x, int y);
    void onWsGameOver(const QString &winner, const QString &reason);
    void onWsOpponentQuit(const QString &roomId, int userId);
    void onQuitClicked();

private:
    NetworkManager *m_net;

    // --- 游戏状态 ---
    int m_userId = -1;
    QString m_roomId;
    QString m_myColor;     // "black" 或 "white"
    QString m_turnColor;   // 当前轮到谁

    // --- 棋盘数据 ---
    // 15×15 标准五子棋盘：0=空, 1=黑棋, 2=白棋
    static const int BOARD_SIZE = 15;   // 棋盘大小（15路）
    static const int CELL_SIZE = 36;    // 每格像素
    static const int MARGIN = 30;       // 棋盘边距

    int m_board[BOARD_SIZE][BOARD_SIZE] = {{0}};
    int m_hoverX = -1, m_hoverY = -1;   // 鼠标悬停位置

    // --- UI 控件 ---
    QLabel *m_infoLabel;       // 显示：你的颜色/胜负
    QLabel *m_statusLabel;     // 显示：轮到谁/提示信息
    QPushButton *m_quitBtn;    // 退出按钮
    QWidget *m_overlay;        // 游戏结束覆盖层（未使用，预留）

    // --- 辅助方法 ---
    QPoint boardToPixel(int x, int y) const;       // 棋盘坐标 → 像素坐标
    bool pixelToBoard(int px, int py, int &x, int &y) const;  // 像素坐标 → 棋盘坐标
    bool checkWin(int x, int y, int player);       // 检查是否五子连珠
    void placeStone(int x, int y, int player);     // 放置棋子
    void setMyTurn(bool myTurn);                   // 设置轮到谁
};

#endif // GAMEWIDGET_H
