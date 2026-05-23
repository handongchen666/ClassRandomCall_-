# ClassRandomCall
# 随机类调用
A lightweight, always-on-top floating ball random name picker for Windows classrooms. Features weighted probability, password-protected settings, whitelist support, and repeat-pick toggle.
轻量级的 Windows 课堂随机点名软件，支持置顶悬浮球设计。功能包括加权概率、密码保护设置、白名单支持和重复抽取开关。
***default password:123456***
***默认密码:123456***

编译说明：
VS: 直接 Ctrl+F5 编译运行
MinGW: g++ -o ClassRandomCall.exe ClassRandomCall.cpp -mwindows -luser32 -lgdi32 -lshell32 -ladvapi32 -lcomctl32 -static
