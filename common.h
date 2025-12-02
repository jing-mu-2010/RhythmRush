/* 
 * 文件名: common.h
 * 描述: 全局定义、结构体与函数声明
 * 状态: 已更新 (包含选人功能、暂停功能、物理参数)
 */
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graphics.h> // EasyX 库
#include <conio.h>
#include <time.h>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// --- 宏定义 ---
#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define GROUND_Y 500  // 地面高度 (方便大家统一)

// --- 游戏状态枚举 (增加了选人状态) ---
typedef enum {
    STATE_MENU,         // 0: 主菜单
    STATE_CHAR_SELECT,  // 1: 角色选择界面 【新增】
    STATE_GAME,         // 2: 游戏画面
    STATE_MANAGER,      // 3: 后台管理
    STATE_EXIT          // 4: 退出程序
} GameState;

// --- 数据结构 (关卡配置) ---
typedef struct GameConfig {
    // 基础信息
    int id;                 // 编号
    char name[50];          // 关卡名
    
    // 音频与难度
    char bgmPath[100];      // 音乐路径
    int speed;              // 速度 (1000基准)
    
    // 【新增】物理与视觉参数
    int gravity;            // 重力 (1-5)
    int themeColor;         // 主题色 (0:红 1:蓝 2:绿)
    int bestScore;          // 最高分记录

    struct GameConfig* next;// 链表指针
} GameConfig;

// --- 函数声明 (各部门工作清单) ---

// ui_sys.c (UI组)
void DrawMenu();
void DrawManager(GameConfig* head);
void DrawCharSelect(int currentType); // 【新增】画选人界面
int GetUserSelection(); // 获取菜单选择

// data_manager.c (后台组)
GameConfig* InitList();
void AddConfig(GameConfig* head, char* name, int speed, int gravity, int theme); // 参数变多了
void SaveConfigs(GameConfig* head); // 存档
void LoadConfigs(GameConfig* head); // 读档

// audio_sys.c (音频组)
void PlayBGM(char* path);
void Audio_Pause();  // 【新增】暂停音乐
void Audio_Resume(); // 【新增】恢复音乐

// game_engine.c (游戏组)
// 【关键修改】增加 playerType 参数，告诉游戏用哪个角色
void RunGame(GameConfig* config, int playerType); 

#endif
