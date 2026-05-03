// ============================================================
#include <QDebug>
// loginwidget.cpp — 登录/注册页面实现
// ============================================================
// 【Qt 学习笔记 — 布局系统 / 样式表 / Lambda 槽】
//
// 1. 布局管理器（Layout Managers）
//    自动管理子控件的位置和大小，窗口缩放时自动调整。
//    Qt 提供了几种布局：
//    - QVBoxLayout:   垂直排列（从上到下）
//    - QHBoxLayout:   水平排列（从左到右）
//    - QGridLayout:   网格排列（类似表格）
//    - QFormLayout:   表单排列（标签+输入框对）
//    - QStackedLayout: 堆叠布局（一次只显示一个，与 QStackedWidget 类似）
//
// 2. 布局的 addWidget() 与 addLayout()
//    - addWidget(widget) — 向布局添加控件
//    - addWidget(widget, stretch, align) — stretch=拉伸系数，align=对齐方式
//    - addLayout(layout) — 嵌套子布局
//    - addSpacing(n) — 固定间距
//    - addStretch(n) — 弹性空白（重要！用于将控件"推"到一端）
//
// 3. setAlignment(Qt::AlignCenter)
//    Qt 命名空间提供了大量对齐/方向常量：
//    - Qt::AlignLeft / AlignRight / AlignHCenter
//    - Qt::AlignTop / AlignBottom / AlignVCenter
//    - Qt::AlignCenter (= AlignHCenter | AlignVCenter)
//
// 4. 样式表（StyleSheet）
//    Qt 支持类似 CSS 的样式表，可美化控件外观：
//    widget->setStyleSheet("color: red; background: #eee; font-size: 14px;");
//    支持：颜色（color, background-color）、边距（margin, padding）、
//    边框（border）、字体（font-size）、选择器（QPushButton:hover）等。
//    但注意：Qt 样式表是 CSS2.1 子集，并非完整 CSS。
//
// 5. QFont 设置
//    QFont font;
//    font.setPointSize(32);    // 字号（点=1/72英寸）
//    font.setBold(true);       // 粗体
//    widget->setFont(font);
//    注意：setPointSize 在不同平台 DPI 下表现一致，
//    而 setPixelSize(32) 则固定像素值。
//
// 6. connect + Lambda
//    connect(regBtn, &QPushButton::clicked, this, [this]() {
//        m_stack->setCurrentIndex(1);
//    });
//    Lambda 槽的优点是简洁，不需要额外写一个成员函数。
//    但复杂逻辑建议写成命名槽函数，可读性更好。
//    注意：Lambda 中捕获 [this] 时务必确认 this 不会在回调前被销毁。
//
// 7. setContentsMargins(left, top, right, bottom)
//    布局的外边距，即布局边缘与父控件边缘之间的空白。
//
// 8. setFixedWidth(width)
//    固定控件宽度（不随窗口缩放变化）。
//    也有 setFixedHeight()、setFixedSize()。
//    如果控件在布局中，固定宽度后布局仍会考虑该约束。
// ============================================================

#include "loginwidget.h"
#include "networkmanager.h"  // 网络管理器完整定义
#include <QVBoxLayout>       // 垂直布局
#include <QHBoxLayout>       // 水平布局
#include <QFont>             // 字体

LoginWidget::LoginWidget(NetworkManager *net, QWidget *parent)
    : QWidget(parent)          // QWidget 构造
    , m_net(net)               // 保存网络管理器引用
{
    qDebug() << "[LoginWidget] 构造";
    // === 创建页面栈容器 ===
    // m_stack 里放两页：index 0 = 登录页，index 1 = 注册页
    m_stack = new QStackedWidget(this);

    // === 主布局：垂直居中 ===
    // QVBoxLayout 垂直排列子控件
    auto *mainLayout = new QVBoxLayout(this);   // this=LoginWidget 是布局的父对象
    mainLayout->setAlignment(Qt::AlignCenter);   // 子控件居中对齐

    // --- 大标题 "五子棋" ---
    auto *title = new QLabel("五子棋", this);
    title->setAlignment(Qt::AlignCenter);
    QFont titleFont;
    titleFont.setPointSize(32);     // 32 磅字体
    titleFont.setBold(true);        // 粗体
    title->setFont(titleFont);
    mainLayout->addWidget(title);

    mainLayout->addSpacing(30);     // 固定间距 30px
    mainLayout->addWidget(m_stack); // 页面栈（登录/注册）
    mainLayout->addStretch();       // 弹性空白，把内容推到上方区域

    // === 构建子页面 ===
    buildLoginPage();     // index 0
    buildRegisterPage();  // index 1

    // === 连接网络信号 ===
    // 网络请求完成后，NetworkManager 发射对应信号，
    // LoginWidget 接收并处理结果
    connect(m_net, &NetworkManager::loginResult,
            this, &LoginWidget::onLoginResult);
    connect(m_net, &NetworkManager::registerResult,
            this, &LoginWidget::onRegisterResult);
}

// ========== 构建登录页 ==========

void LoginWidget::buildLoginPage()
{
    // QWidget 作为容器页（类似 Android 的 ViewGroup）
    auto *page = new QWidget(this);

    // 登录页的垂直布局
    auto *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(12);                        // 控件间距 12px
    layout->setContentsMargins(40, 20, 40, 20);    // 外边距

    // 子标题 "登录"
    auto *label = new QLabel("登录", page);
    QFont f;
    f.setPointSize(18);
    label->setFont(f);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    // 用户名输入框
    m_loginUsername = new QLineEdit(page);
    m_loginUsername->setPlaceholderText("用户名");    // 灰色提示文字
    m_loginUsername->setFixedWidth(240);            // 固定宽度
    layout->addWidget(m_loginUsername, 0, Qt::AlignCenter);

    // 密码输入框
    m_loginPassword = new QLineEdit(page);
    m_loginPassword->setPlaceholderText("密码");
    m_loginPassword->setEchoMode(QLineEdit::Password);  // 密码模式（显示圆点）
    m_loginPassword->setFixedWidth(240);
    layout->addWidget(m_loginPassword, 0, Qt::AlignCenter);

    // 登录按钮
    m_loginBtn = new QPushButton("登录", page);
    m_loginBtn->setFixedWidth(240);
    layout->addWidget(m_loginBtn, 0, Qt::AlignCenter);

    // "去注册"文字链接（扁平按钮 = 像超链接一样）
    auto *regBtn = new QPushButton("没有账号？去注册", page);
    regBtn->setFlat(true);      // 扁平模式（无边框/背景，类似 HTML 链接）
    regBtn->setFixedWidth(240);
    layout->addWidget(regBtn, 0, Qt::AlignCenter);

    // 错误/状态提示信息
    m_loginMsg = new QLabel("", page);
    m_loginMsg->setAlignment(Qt::AlignCenter);
    m_loginMsg->setStyleSheet("color: red;");       // 红色文字（错误信息）
    layout->addWidget(m_loginMsg);

    // 将登录页加入页面栈（index 0）
    m_stack->addWidget(page);

    // === 信号连接 ===
    // 点击登录按钮 → 执行 onLoginClicked()
    connect(m_loginBtn, &QPushButton::clicked,
            this, &LoginWidget::onLoginClicked);

    // 点击"去注册" → Lambda 切换到注册页（index 1）
    connect(regBtn, &QPushButton::clicked, this, [this]() {
        m_stack->setCurrentIndex(1);    // 切换到注册页
    });

    // 在密码框按回车 → 触发登录（用户体验优化）
    connect(m_loginPassword, &QLineEdit::returnPressed,
            this, &LoginWidget::onLoginClicked);
}

// ========== 构建注册页 ==========

void LoginWidget::buildRegisterPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(12);
    layout->setContentsMargins(40, 20, 40, 20);

    // 子标题 "注册"
    auto *label = new QLabel("注册", page);
    QFont f;
    f.setPointSize(18);
    label->setFont(f);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    m_regUsername = new QLineEdit(page);
    m_regUsername->setPlaceholderText("用户名");
    m_regUsername->setFixedWidth(240);
    layout->addWidget(m_regUsername, 0, Qt::AlignCenter);

    m_regPassword = new QLineEdit(page);
    m_regPassword->setPlaceholderText("密码");
    m_regPassword->setEchoMode(QLineEdit::Password);
    m_regPassword->setFixedWidth(240);
    layout->addWidget(m_regPassword, 0, Qt::AlignCenter);

    m_regConfirm = new QLineEdit(page);
    m_regConfirm->setPlaceholderText("确认密码");
    m_regConfirm->setEchoMode(QLineEdit::Password);
    m_regConfirm->setFixedWidth(240);
    layout->addWidget(m_regConfirm, 0, Qt::AlignCenter);

    m_regBtn = new QPushButton("注册", page);
    m_regBtn->setFixedWidth(240);
    layout->addWidget(m_regBtn, 0, Qt::AlignCenter);

    m_backBtn = new QPushButton("返回登录", page);
    m_backBtn->setFlat(true);           // 扁平化（像链接）
    m_backBtn->setFixedWidth(240);
    layout->addWidget(m_backBtn, 0, Qt::AlignCenter);

    m_regMsg = new QLabel("", page);
    m_regMsg->setAlignment(Qt::AlignCenter);
    m_regMsg->setStyleSheet("color: red;");  // 默认为红色（错误信息）
    layout->addWidget(m_regMsg);

    // 加入页面栈（index 1）
    m_stack->addWidget(page);

    // === 信号连接 ===
    connect(m_regBtn, &QPushButton::clicked,
            this, &LoginWidget::onRegisterClicked);

    // 点击"返回登录" → Lambda 切回登录页（index 0）并清除消息
    connect(m_backBtn, &QPushButton::clicked, this, [this]() {
        m_stack->setCurrentIndex(0);
        m_regMsg->clear();     // QLabel::clear() 清空文本
    });
}

// ========== 槽函数 ==========

void LoginWidget::onLoginClicked()
{
    qDebug() << "[LoginWidget] 点击登录按钮";
    // trimmed() 去除首尾空格
    QString user = m_loginUsername->text().trimmed();
    QString pass = m_loginPassword->text();

    // 客户端校验：空字段检查
    if (user.isEmpty() || pass.isEmpty()) {
        m_loginMsg->setText("请输入用户名和密码");
        return;
    }

    // 显示"登录中..."提示，灰色表示正在处理
    m_loginMsg->setStyleSheet("color: gray;");
    m_loginMsg->setText("登录中...");

    // 禁用按钮防止重复提交
    m_loginBtn->setEnabled(false);

    // 发起网络请求（异步，结果通过信号返回）
    m_net->loginUser(user, pass);
}

void LoginWidget::onRegisterClicked()
{
    QString user = m_regUsername->text().trimmed();
    QString pass = m_regPassword->text();
    QString confirm = m_regConfirm->text();

    // 客户端校验
    if (user.isEmpty() || pass.isEmpty()) {
        m_regMsg->setStyleSheet("color: red;");
        m_regMsg->setText("请填写所有字段");
        return;
    }
    if (pass != confirm) {
        m_regMsg->setStyleSheet("color: red;");
        m_regMsg->setText("两次密码不一致");
        return;
    }
    // 密码长度校验（注册时最小长度要求）
    if (pass.length() < 4) {
        m_regMsg->setStyleSheet("color: red;");
        m_regMsg->setText("密码至少4位");
        return;
    }

    m_regMsg->setStyleSheet("color: gray;");
    m_regMsg->setText("注册中...");
    m_regBtn->setEnabled(false);

    m_net->registerUser(user, pass);
}

// ---- 网络回调 ----

void LoginWidget::onLoginResult(bool success, const QString &token, int userId, const QString &msg)
{
    qDebug() << "[LoginWidget] 登录结果:" << success << msg;
    // 恢复按钮状态
    m_loginBtn->setEnabled(true);

    if (success) {
        // 登录成功 → 发射 loginSuccess 信号
        // MainWindow 收到后会切换到大厅页
        emit loginSuccess(token, userId);
    } else {
        // 登录失败 → 显示错误消息
        m_loginMsg->setStyleSheet("color: red;");
        m_loginMsg->setText(msg);
    }
}

void LoginWidget::onRegisterResult(bool success, const QString &msg)
{
    m_regBtn->setEnabled(true);

    if (success) {
        // 注册成功 → 提示并自动跳转回登录页
        m_regMsg->setStyleSheet("color: green;");  // 绿色表示成功
        m_regMsg->setText("注册成功，请返回登录");
        m_stack->setCurrentIndex(0);                 // 切回登录页

        // 自动填入刚注册的用户名，让用户只需输入密码
        m_loginUsername->setText(m_regUsername->text());
        m_loginPassword->setFocus();                  // 密码框获取焦点
    } else {
        m_regMsg->setStyleSheet("color: red;");
        m_regMsg->setText(msg);
    }
}
