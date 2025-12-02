/* 
 * 文件名: common.h
 * 描述: 全局定义、结构体与函数声明 (项目的“宪法”)
 * 状态: 最终完整版
 */
#ifndef COMMON_H
#define COMMON_H

// --- 包含库文件 ---
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graphics.h> // EasyX 图形库
#include <conio.h>
#include <time.h>
#include <windows.h>
#include <math.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") // 链接音频库

// --- 宏定义 (全局常量) ---
#define WIN_WIDTH 800     // 窗口宽度
#define WIN_HEIGHT 600    // 窗口高度
#define GROUND_Y 500      // 地面高度

// --- 游戏状态枚举 (控制程序流程) ---
typedef enum {
    STATE_MENU,         // 0: 主菜单
    STATE_CHAR_SELECT,  // 1: 角色选择
    STATE_LEVEL_SELECT, // 2: 难度选择
    STATE_GAME,         // 3: 游戏进行中
    STATE_MANAGER,      // 4: 后台管理系统
    STATE_EXIT          // 5: 退出程序
} GameState;

// --- 数据结构: 关卡配置 (后台组负责) ---
typedef struct GameConfig {
    int id;                 // 编号 (1, 2, 3...)
    char name[50];          // 关卡名称 (如 "简单模式")
    
    // 核心参数
    char bgmPath[100];      // 音乐路径
    int speed;              // 游戏速度 (1000为基准)
    int gravity;            // 重力参数 (1=飘, 5=重)
    int themeColor;         // 主题颜色 (0:绿, 1:蓝, 2:红)
    
    int bestScore;          // 历史最高分记录

    struct GameConfig* next;// 链表指针
} GameConfig;

// --- 函数声明 (各部门工作清单) ---

// [UI组] ui_sys.c
void DrawMenu();                           // 画主菜单
void DrawCharSelect(int currentType);      // 画选人界面
void DrawLevelSelect();                    // 画选难度界面
void DrawManager(GameConfig* head);        // 画后台表格

// [后台组] data_manager.c
GameConfig* InitList();                    // 初始化链表
void AddConfig(GameConfig* head, char* name, int speed, int gravity, int theme); // 增加
void DeleteConfig(GameConfig* head, int id); // 删除
void SaveConfigs(GameConfig* head);        // 保存文件
void LoadConfigs(GameConfig* head);        // 读取文件

// [音频组] audio_sys.c
void PlayBGM(char* path);                  // 播放音乐
void Audio_Pause();                        // 暂停
void Audio_Resume();                       // 恢复

// [游戏组] game_engine.c
// 参数: config=当前关卡设定, playerType=当前选的角色
void RunGame(GameConfig* config, int playerType); 

#endif
