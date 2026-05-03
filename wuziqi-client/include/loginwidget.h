// ============================================================
// loginwidget.h — 登录/注册页面头文件
// ============================================================
// 【Qt 学习笔记 — 基本控件 / 前向声明 / signals】
//
// 1. QLineEdit
//    单行文本输入框。常用方法：
//    - setPlaceholderText("提示文字") — 输入框灰色占位文字
//    - setEchoMode(QLineEdit::Password) — 密码模式（显示圆点）
//    - text() — 获取当前文本
//    - setText("...") — 设置文本
//    - setFocus() — 获取键盘焦点
//    - returnPressed — 信号：用户按回车时触发
//
// 2. QPushButton
//    按钮控件。最常用的信号：
//    - clicked() — 用户点击时触发
//    - setEnabled(bool) — 启用/禁用（禁用后灰显且不可点击）
//    - setFlat(bool) — 扁平样式（无边框，适合"文字链接"风格）
//
// 3. QLabel
//    纯文本/富文本/RichText 显示控件。用于显示信息或错误消息。
//    - setText("...")
//    - setStyleSheet("color: red;") — CSS 风格样式
//    - setAlignment(Qt::AlignCenter) — 对齐方式
//    - setFont(QFont) — 设置字体
//    - setPixmap(QPixmap) — 显示图片
//
// 4. signals:
//    信号（Signal）定义段，只在头文件中声明，不需要实现。
//    Qt 的 MOC 会自动生成实现代码。
//    信号就像"广播"——任何绑定了这个信号的槽都会被触发。
//
// 5. 前向声明（Forward Declaration）
//    "class NetworkManager;" 告诉编译器这是一个类，
//    但不需要包含它的头文件。在头文件中尽量使用前向声明
//    而不是 #include，因为这可以：
//    - 减少编译依赖，加速编译
//    - 避免循环引用（A.h 和 B.h 互相包含）
//    - 只在使用指针/引用时有效（需要知道对象大小时不行）
// ============================================================

#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>        // QWidget 基类
#include <QLineEdit>      // 单行输入框
#include <QPushButton>    // 按钮
#include <QLabel>         // 文字标签
#include <QStackedWidget> // 页面堆栈（登录页/注册页切换）

// 前向声明，避免在头文件中包含 networkmanager.h
class NetworkManager;

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    // 构造函数接收 NetworkManager 指针，用于发送网络请求
    explicit LoginWidget(NetworkManager *net, QWidget *parent = nullptr);

signals:
    // 自定义信号：登录成功
    // 信号只需要声明，不需要实现（MOC 自动生成）
    // 参数是 token 和 userId，由 MainWindow 接收后做页面切换
    void loginSuccess(const QString &token, int userId);

    // 信号：前往注册页（当前未使用，保留作扩展）
    void gotoRegister();

private slots:
    // 点击"登录"按钮时的处理
    void onLoginClicked();
    // 点击"注册"按钮时的处理
    void onRegisterClicked();

    // 服务器返回登录结果
    // 注意：这些参数的顺序和类型必须与信号声明完全一致
    void onLoginResult(bool success, const QString &token, int userId, const QString &msg);
    // 服务器返回注册结果
    void onRegisterResult(bool success, const QString &msg);

private:
    NetworkManager *m_net;                    // 网络管理器

    QStackedWidget *m_stack;                  // 页面栈（0=登录页，1=注册页）

    // --- 登录页控件 ---
    QLineEdit *m_loginUsername;               // 用户名输入框
    QLineEdit *m_loginPassword;               // 密码输入框
    QPushButton *m_loginBtn;                  // 登录按钮
    QLabel *m_loginMsg;                       // 登录反馈消息

    // --- 注册页控件 ---
    QLineEdit *m_regUsername;                 // 注册用户名
    QLineEdit *m_regPassword;                 // 注册密码
    QLineEdit *m_regConfirm;                  // 确认密码
    QPushButton *m_regBtn;                    // 注册按钮
    QPushButton *m_backBtn;                   // 返回登录按钮
    QLabel *m_regMsg;                         // 注册反馈消息

    // 构建登录页和注册页的布局
    void buildLoginPage();
    void buildRegisterPage();
};

#endif // LOGINWIDGET_H
