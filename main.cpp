/*
 * 文件名: main.cpp
 * 描述: 程序总指挥 (CEO)，负责状态调度与中文交互
 * 负责人: 组长
 */

#include "common.h"

int main() {
    // ==========================================
    // 1. 系统初始化
    // ==========================================
    initgraph(WIN_WIDTH, WIN_HEIGHT); // 创建窗口
    setbkmode(TRANSPARENT);           // 文字透明背景
    srand((unsigned int)time(NULL));  // 随机数种子

    // --- 数据准备 ---
    // 调用后台组函数，建立链表
    GameConfig* configList = InitList();

    // 【预设数据】手动添加三个难度 (Head, Name, Speed, Gravity, Theme)
    // 这里的字符串全是中文，配合 -fexec-charset=GBK 使用
    AddConfig(configList, "简单模式", 800,  1, 0); // 速度慢，重力低(飘)，绿色主题
    AddConfig(configList, "普通模式", 1200, 3, 1); // 速度中，重力中，蓝色主题
    AddConfig(configList, "困难模式", 1800, 5, 2); // 速度快，重力大(沉)，红色主题

    // --- 全局变量 ---
    GameState currentState = STATE_MENU; // 当前状态
    int myRole = 1;                      // 当前选的角色 (1,2,3)
    GameConfig* selectedConfig = NULL;   // 当前选的关卡/难度

    // ==========================================
    // 2. 核心主循环
    // ==========================================
    while (currentState != STATE_EXIT) {
        
        cleardevice(); // 每一帧先清屏

        switch (currentState) {
            
            // ----------------------------------
            // A. 主菜单
            // ----------------------------------
            case STATE_MENU:
                DrawMenu(); // UI组画菜单
                
                // 按 '1' 去选角色
                if (GetAsyncKeyState('1') & 0x8000) {
                    currentState = STATE_CHAR_SELECT;
                    Sleep(200); // 防抖
                }
                // 按 '2' 去后台管理
                else if (GetAsyncKeyState('2') & 0x8000) {
                    currentState = STATE_MANAGER;
                    Sleep(200);
                }
                // 按 'ESC' 退出
                else if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                    currentState = STATE_EXIT;
                }
                break;

            // ----------------------------------
            // B. 角色选择 (三选一)
            // ----------------------------------
            case STATE_CHAR_SELECT:
                DrawCharSelect(myRole); // UI组画高亮

                // 简单的按键选择
                if (GetAsyncKeyState('1') & 0x8000) myRole = 1;
                if (GetAsyncKeyState('2') & 0x8000) myRole = 2;
                if (GetAsyncKeyState('3') & 0x8000) myRole = 3;

                // 按回车确认，进入下一步(选难度)
                if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
                    currentState = STATE_LEVEL_SELECT;
                    Sleep(200);
                }
                // 按 ESC 返回
                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                    currentState = STATE_MENU;
                    Sleep(200);
                }
                break;

            // ----------------------------------
            // C. 难度选择 (从链表中选)
            // ----------------------------------
            case STATE_LEVEL_SELECT:
                DrawLevelSelect(); // UI组画三个难度框

                // 逻辑映射：1->第1个配置, 2->第2个...
                int choice = 0;
                if (GetAsyncKeyState('1') & 0x8000) choice = 1;
                if (GetAsyncKeyState('2') & 0x8000) choice = 2;
                if (GetAsyncKeyState('3') & 0x8000) choice = 3;

                if (choice > 0) {
                    // 【核心逻辑】遍历链表找到对应的配置节点
                    GameConfig* p = configList->next; 
                    int count = 1;
                    while (p != NULL && count < choice) {
                        p = p->next;
                        count++;
                    }
                    
                    if (p != NULL) {
                        selectedConfig = p; // 锁定配置
                        currentState = STATE_GAME; // 进游戏！
                        Sleep(200);
                    }
                }
                break;

            // ----------------------------------
            // D. 游戏进行中 (调用游戏引擎)
            // ----------------------------------
            case STATE_GAME:
                if (selectedConfig != NULL) {
                    // 把 关卡配置 和 角色类型 传给游戏引擎
                    // RunGame 内部有死循环，游戏结束才会返回
                    RunGame(selectedConfig, myRole);
                }
                // 游戏结束后，回到菜单
                currentState = STATE_MENU;
                break;

            // ----------------------------------
            // E. 后台管理系统 (增删改查)
            // ----------------------------------
            case STATE_MANAGER:
                DrawManager(configList); // UI组画表格

                // [A] Add: 弹窗新增 (使用中文输入框)
                if (GetAsyncKeyState('A') & 0x8000) {
                    char nameBuf[50], speedBuf[10], gravBuf[10];
                    
                    // EasyX InputBox: (缓冲区, 长度, 提示语, 标题, 默认值...)
                    InputBox(nameBuf, 50, "请输入关卡名称:", "新增关卡", "自定义模式", 0, 0, false);
                    InputBox(speedBuf, 10, "请输入速度 (800-2000):", "新增关卡", "1500", 0, 0, false);
                    InputBox(gravBuf, 10, "请输入重力 (1-5):", "新增关卡", "3", 0, 0, false);
                    
                    // 调用后台组 AddConfig
                    AddConfig(configList, nameBuf, atoi(speedBuf), atoi(gravBuf), 0);
                    Sleep(200);
                }

                // [D] Delete: 弹窗删除
                else if (GetAsyncKeyState('D') & 0x8000) {
                    char idBuf[10];
                    InputBox(idBuf, 10, "请输入要删除的ID:", "删除关卡", "", 0, 0, false);
                    DeleteConfig(configList, atoi(idBuf)); // 调用后台组函数
                    Sleep(200);
                }

                // [S] Save: 保存
                else if (GetAsyncKeyState('S') & 0x8000) {
                    SaveConfigs(configList); // 调用后台组函数
                    outtextxy(300, 300, "保存成功！");
                    Sleep(1000);
                }

                // [ESC] 返回
                else if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                    currentState = STATE_MENU;
                    Sleep(200);
                }
                break;
        }

        Sleep(16); // 帧率控制 (约60fps)
    }

    // ==========================================
    // 3. 退出清理
    // ==========================================
    closegraph();
    // FreeList(configList); // 这一步通常交给操作系统回收
    return 0;
}
