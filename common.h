/* common.h - 公共头文件  */
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

// 宏定义 
#define WIN_WIDTH 800
#define WIN_HEIGHT 600

// --- 游戏状态枚举 ---
typedef enum {
    STATE_MENU,     // 0:主菜单
    STATE_GAME,     // 1:游戏画面
    STATE_MANAGER,  // 2:后台管理
    STATE_EXIT      // 3:退出程序
} GameState;

//  数据结构定义 (后台组负责) 
typedef struct GameConfig {
    int id;       //链表节点编号
    char name[50];//关卡名称
    char bgmPath[100];//音乐路径
    int speed;//播放速度
    int themeColor;//关卡主题.0:白（冰雪），1：红（火山），3：绿（森林）
    int bestScore;//该关卡历史最高分
    struct GameConfig* next;
} GameConfig;

// 函数声明

// ui_sys.c (UI组)
void DrawMenu();//画主菜单
void DrawManager(GameConfig* head);//画关卡配置，让玩家看到有哪些关卡配置
int GetUserSelection(); // 比如返回 1=开始, 2=关卡配置, 3=退出

// data_manager.c (后台组)
GameConfig* InitList();//初始化链表
void AddConfig(GameConfig* head, char* name, int speed);//

// audio_sys.c (音频组)
void PlayBGM(char* path);//放音乐

// game_engine.c (游戏组)
void RunGame(GameConfig* config);//进入游戏逻辑


#endif
