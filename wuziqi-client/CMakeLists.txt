# ============================================================
# CMakeLists.txt — 五子棋客户端 CMake 构建配置
# ============================================================
# 【Qt 学习笔记 — CMake 构建系统】
#
# CMake 是 Qt 官方推荐的构建系统（qmake 已逐渐被弃用）。
# 以下是关键 Qt-CMake 知识点：
#
# 1. CMAKE_AUTOMOC = ON
#    Qt 的元对象编译器（Meta-Object Compiler, MOC）。
#    凡是声明了 Q_OBJECT 宏的头文件，都需要通过 MOC 处理，
#    生成额外的 C++ 代码（信号/槽的运行时连接、反射信息等）。
#    开启 AUTOMOC 后，CMake 会自动扫描头文件并调用 MOC，
#    无需手动写 qt5_wrap_cpp()。
#
# 2. CMAKE_AUTORCC = ON
#    Qt 资源编译器（Resource Compiler, RCC）。
#    如果项目中有 .qrc 文件（Qt 资源文件，如图片/字体/qss），
#    RCC 会将其编译为二进制数据嵌入可执行文件。
#
# 3. CMAKE_AUTOUIC = ON
#    Qt UI 编译器（User Interface Compiler, UIC）。
#    如果项目中有 .ui 文件（Qt Designer 生成的 XML 界面），
#    UIC 会将其编译为 C++ 头文件。
#
# 4. find_package(Qt5 COMPONENTS ...)
#    查找已安装的 Qt 库。COMPONENTS 指定需要的模块：
#    - Widgets:   核心 UI 组件（QWidget, QPushButton 等）
#    - Network:   HTTP 网络请求（QNetworkAccessManager）
#    - WebSockets: WebSocket 通信（QWebSocket）
#
# 5. target_link_libraries
#    将 Qt 模块链接到目标。
#    Qt5::Widgets 是"导入目标"（imported target），
#    比直接写 -lQt5Widgets 更便携（自动包含头文件路径等）。
# ============================================================

cmake_minimum_required(VERSION 3.16)
project(WuziQiClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Qt 自动工具：MOC / RCC / UIC ---
set(CMAKE_AUTOMOC ON)   # 自动调用元对象编译器（处理 Q_OBJECT）
set(CMAKE_AUTORCC ON)   # 自动调用资源编译器（处理 .qrc 文件）
set(CMAKE_AUTOUIC ON)   # 自动调用 UI 编译器（处理 .ui 文件）

# --- 查找 Qt5（兼容 Qt6） ---
# find_package 会在系统路径（/usr/lib/cmake/）查找 Qt 的 CMake 配置模块。
# COMPONENTS 指定需要的 Qt 模块：
#   - Widgets:   界面组件（窗口、按钮、布局等）
#   - Network:   HTTP 请求（QNetworkAccessManager）
#   - WebSockets: WebSocket（QWebSocket）
find_package(Qt5 COMPONENTS Widgets Network WebSockets QUIET)
if(NOT Qt5_FOUND)
    find_package(Qt6 COMPONENTS Widgets Network WebSockets REQUIRED)
    set(QT_LIBS Qt6::Widgets Qt6::Network Qt6::WebSockets)
else()
    set(QT_LIBS Qt5::Widgets Qt5::Network Qt5::WebSockets)
endif()

# --- 源文件与头文件 ---
# 注意：头文件必须也列在这里！CMAKE_AUTOMOC 会扫描列表中的文件
# 来查找含有 Q_OBJECT 宏的 header，然后自动调用 MOC 生成
# 元对象代码（信号/槽的实现、vtable 等）。
# 如果只列 .cpp 而不列 .h，AUTOMOC 找不到头文件，
# 链接时会报 "undefined reference to vtable" 错误。

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/loginwidget.cpp
    src/lobbywidget.cpp
    src/gamewidget.cpp
    src/networkmanager.cpp
    src/networkworker.cpp
)

set(HEADERS
    include/mainwindow.h
    include/loginwidget.h
    include/lobbywidget.h
    include/gamewidget.h
    include/networkmanager.h
    include/networkworker.h
)

add_executable(wuziqi_client ${SOURCES} ${HEADERS})

# 添加头文件搜索路径，这样 .cpp 中的 #include "mainwindow.h" 就能找到
# PRIVATE 表示这个路径只对 wuziqi_client 生效
# 注意：仅靠 target_include_directories 不足以让 AUTOMOC 扫描头文件
# 必须同时在 add_executable 中列出头文件！
target_include_directories(wuziqi_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- 链接 Qt 库 ---
# Qt5::Widgets 是一个"导入目标"（IMPORTED target），
# 它封装了头文件路径、库文件、编译宏（如 QT_WIDGETS_LIB）等，
# 比手动设置 include_directories 更干净。
target_link_libraries(wuziqi_client PRIVATE ${QT_LIBS})
