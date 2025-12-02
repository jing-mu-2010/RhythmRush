
#include "common.h"

int main() {
    // 1. 初始化窗口
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    setbkmode(TRANSPARENT); // 文字背景透明

    // 2. 初始化数据 (模拟后台组的工作)
    GameConfig* configList = InitList(); 
    
    // 初始状态设为菜单
    GameState currentState = STATE_MENU;

    // 3. 主循环
    while (currentState != STATE_EXIT) {
        cleardevice(); // 每次循环清屏

        switch (currentState) {
            case STATE_MENU:
                DrawMenu(); // 调用UI组的函数
                // 这里暂时模拟点击，实际要写鼠标判断
                // 按 '1' 进游戏，按 '2' 进管理，按 'Esc' 退出
                if (GetAsyncKeyState('1') & 0x8000) currentState = STATE_GAME;
                else if (GetAsyncKeyState('2') & 0x8000) currentState = STATE_MANAGER;
                else if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) currentState = STATE_EXIT;
                break;

            case STATE_GAME:
                // 假装传第一个配置进去玩
                RunGame(configList); 
                currentState = STATE_MENU; // 游戏结束回到菜单
                break;

            case STATE_MANAGER:
                DrawManager(configList);
                // 按空格返回菜单
                if (GetAsyncKeyState(VK_SPACE) & 0x8000) currentState = STATE_MENU;
                break;
        }
        Sleep(50); // 防止CPU占用过高
    }

    closegraph();
    return 0;
}