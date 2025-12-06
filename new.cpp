// game_engine.c - 游戏引擎完整版
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graphics.h>
#include <conio.h>
#include <time.h>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <math.h>

// --- 全局常量 ---
#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define GROUND_Y 500
#define DINO_WIDTH 40
#define DINO_HEIGHT 110
#define CACTUS_WIDTH 20
#define CACTUS_HEIGHT 40
#define BIRD_WIDTH 30
#define BIRD_HEIGHT 20
#define GRAVITY 1.003
#define JUMP_STRENGTH 18
#define GAME_SPEED 10
#define TARGET_FPS 60
#define MIN_OBSTACLE_DISTANCE 300
#define MAX_OBSTACLE_DISTANCE 600
#define MIN_SPAWN_INTERVAL 25
#define MAX_SPAWN_INTERVAL 60
#define MAX_OBSTACLES_ON_SCREEN 500

// --- 游戏状态 ---
typedef enum {
    STATE_MENU,
    STATE_CHAR_SELECT,
    STATE_LEVEL_SELECT,
    STATE_GAME,
    STATE_GAME_OVER,
    STATE_EXIT
} GameState;

// --- 角色类型 ---
typedef enum {
    CHAR_DEFAULT,    // 默认恐龙
    CHAR_SPEEDY,     // 速度快，跳跃高
    CHAR_TANK,       // 生命值高，速度慢
    CHAR_COUNT       // 角色总数
} CharacterType;

// --- 难度级别 ---
typedef enum {
    LEVEL_EASY,      // 简单
    LEVEL_NORMAL,    // 普通
    LEVEL_HARD,      // 困难
    LEVEL_COUNT      // 难度总数
} DifficultyLevel;

// --- 关卡配置 ---
typedef struct {
    int id;
    char name[50];
    int speed;              // 游戏速度 (1000为基准)
    int gravity;            // 重力参数 (1=飘, 3=正常, 5=重)
    int themeColor;         // 主题颜色 (0:绿, 1:蓝, 2:红)
    int obstacleDensity;    // 障碍物密度
    int birdHeight;         // 鸟的飞行高度
    int bestScore;
} GameConfig;

// --- 角色配置 ---
typedef struct {
    CharacterType type;
    char name[50];
    int colorR, colorG, colorB;
    float speedMultiplier;  // 速度倍率
    float jumpMultiplier;   // 跳跃倍率
    float gravityMultiplier; // 重力倍率
    int specialAbility;     // 特殊能力
} CharacterConfig;

// --- 游戏内部数据结构 ---
typedef struct {
    int x, y;
    int width, height;
    int velocityY;
    int isJumping;
    int isDucking;
    int frame;
    int lives;              // 生命值
    CharacterType type;     // 角色类型
} Dino;

typedef struct {
    int x, y;
    int width, height;
    int type;               // 0:仙人掌小, 1:仙人掌大, 2:鸟
    int passed;
} Obstacle;

typedef struct {
    int x, y;
    int speed;
} Cloud;

// --- 全局变量 ---
GameState gameState = STATE_MENU;
CharacterType selectedChar = CHAR_DEFAULT;
DifficultyLevel selectedLevel = LEVEL_NORMAL;

Dino dino;
Obstacle obstacles[5];
Cloud clouds[5];
int obstacleCount = 0;
int score = 0;
int highScore = 0;
int gameSpeed = GAME_SPEED;
int nightMode = 0;
int cloudCount = 5;
int frameCount = 0;
int framesSinceLastObstacle = 0;
int nextSpawnInterval = 35;
int nextMinDistance = 350;

// --- 配置数据 ---
GameConfig levelConfigs[LEVEL_COUNT] = {
    // 简单模式
    {0, "简单模式", 800, 2, 0, 30, 100, 0},
    // 普通模式
    {1, "普通模式", 1000, 3, 1, 45, 80, 0},
    // 困难模式
    {2, "困难模式", 1200, 5, 2, 60, 60, 0}
};

CharacterConfig charConfigs[CHAR_COUNT] = {
    // 默认恐龙
    {CHAR_DEFAULT, "普通龙", 80, 180, 80, 1.0, 1.0, 1.0, 0},
    // 速度型
    {CHAR_SPEEDY, "速度龙", 255, 100, 100, 1.3, 1.2, 0.8, 0},
    // 坦克型
    {CHAR_TANK, "坦克龙", 100, 100, 255, 0.8, 0.9, 1.2, 1}
};

// --- 函数声明 ---
void initGame();
void handleInput();
void updateGame();
void renderGame();
void drawMenu();
void drawCharSelect();
void drawLevelSelect();
void drawPlayingState();
void drawGameOver();
void drawDino();
void updateDino();
void drawGround();
void generateObstacle();
void updateObstacles();
void drawObstacles();
void drawClouds();
void updateClouds();
void checkCollision();
void drawScore();
void drawNightSky();
void drawButton(int x, int y, int width, int height, const char* text, int selected);
void drawCharPreview(int x, int y, CharacterType type, int selected);
void drawLevelPreview(int x, int y, DifficultyLevel level, int selected);

// --- 初始化函数 ---
void initGame() {
    // 获取当前选择的配置
    CharacterConfig* charConfig = &charConfigs[selectedChar];
    GameConfig* levelConfig = &levelConfigs[selectedLevel];
    
    // 初始化恐龙
    dino.x = 100;
    dino.y = GROUND_Y - DINO_HEIGHT;
    dino.width = DINO_WIDTH;
    dino.height = DINO_HEIGHT;
    dino.velocityY = 0;
    dino.isJumping = 0;
    dino.isDucking = 0;
    dino.frame = 0;
    dino.lives = (selectedChar == CHAR_TANK) ? 3 : 1;  // 坦克型有3条命
    dino.type = selectedChar;
    
    // 初始化障碍物
    obstacleCount = 0;
    for (int i = 0; i < 5; i++) {
        obstacles[i].x = 0;
        obstacles[i].passed = 1;
    }
    
    // 初始化云朵
    for (int i = 0; i < cloudCount; i++) {
        clouds[i].x = rand() % WIN_WIDTH;
        clouds[i].y = 50 + rand() % 200;
        clouds[i].speed = 1 + rand() % 3;
    }
    
    // 重置游戏参数
    score = 0;
    gameSpeed = GAME_SPEED * levelConfig->speed / 1000;
    frameCount = 0;
    framesSinceLastObstacle = 0;
    
    // 根据难度设置生成参数
    nextSpawnInterval = MIN_SPAWN_INTERVAL + (MAX_SPAWN_INTERVAL - MIN_SPAWN_INTERVAL) * (100 - levelConfig->obstacleDensity) / 100;
    nextMinDistance = MIN_OBSTACLE_DISTANCE + rand() % (MAX_OBSTACLE_DISTANCE - MIN_OBSTACLE_DISTANCE);
    
    // 根据主题颜色决定是否为夜晚模式
    nightMode = (levelConfig->themeColor == 2);  // 红色主题=夜晚
    
    // 游戏开始时立即尝试生成第一个障碍物
    framesSinceLastObstacle = nextSpawnInterval;
}

// --- 输入处理函数 ---
void handleInput() {
    ExMessage msg;
    
    while (peekmessage(&msg, EX_KEY)) {
        if (msg.message == WM_KEYDOWN) {
            int key = msg.vkcode;
            
            switch (gameState) {
                case STATE_MENU:
                    if (key == VK_SPACE) {
                        gameState = STATE_CHAR_SELECT;
                    } else if (key == VK_ESCAPE) {
                        gameState = STATE_EXIT;
                    }
                    break;
                    
case STATE_CHAR_SELECT:
    if (key == VK_LEFT) {
        selectedChar = (CharacterType)((selectedChar - 1 + CHAR_COUNT) % CHAR_COUNT);  // 第237行
    } else if (key == VK_RIGHT) {
        selectedChar = (CharacterType)((selectedChar + 1) % CHAR_COUNT);  // 第239行
    } else if (key == VK_SPACE) {
        gameState = STATE_LEVEL_SELECT;
    } else if (key == VK_ESCAPE) {
        gameState = STATE_MENU;
    }
    break;
    
case STATE_LEVEL_SELECT:
    if (key == VK_LEFT) {
        selectedLevel = (DifficultyLevel)((selectedLevel - 1 + LEVEL_COUNT) % LEVEL_COUNT);  // 第249行
    } else if (key == VK_RIGHT) {
        selectedLevel = (DifficultyLevel)((selectedLevel + 1) % LEVEL_COUNT);  // 第251行
    } else if (key == VK_SPACE) {
        gameState = STATE_GAME;
        initGame();
    } else if (key == VK_ESCAPE) {
        gameState = STATE_CHAR_SELECT;
    }
    break;
                    
                case STATE_GAME:
                    if (key == VK_SPACE && !dino.isJumping) {
                        // 根据角色配置调整跳跃力度
                        float jumpPower = JUMP_STRENGTH * charConfigs[selectedChar].jumpMultiplier;
                        dino.velocityY = -(int)jumpPower;
                        dino.isJumping = 1;
                    } else if (key == VK_ESCAPE) {
                        gameState = STATE_MENU;
                    } else if (key == VK_DOWN) {
                        dino.isDucking = 1;
                    }
                    break;
                    
                case STATE_GAME_OVER:
                    if (key == VK_SPACE) {
                        gameState = STATE_MENU;
                    } else if (key == VK_ESCAPE) {
                        gameState = STATE_EXIT;
                    }
                    break;
            }
        }
        else if (msg.message == WM_KEYUP) {
            if (msg.vkcode == VK_DOWN && gameState == STATE_GAME) {
                dino.isDucking = 0;
            }
        }
    }
    
    // 持续按键检测
    if (gameState == STATE_GAME && (GetAsyncKeyState(VK_DOWN) & 0x8000)) {
        dino.isDucking = 1;
    }
}

// --- 游戏更新函数 ---
void updateGame() {
    if (gameState == STATE_GAME) {
        updateDino();
        updateObstacles();
        updateClouds();
        checkCollision();
        frameCount++;
        framesSinceLastObstacle++;
    }
}

// --- 渲染函数 ---
void renderGame() {
    cleardevice();
    
    switch (gameState) {
        case STATE_MENU:
            drawMenu();
            break;
        case STATE_CHAR_SELECT:
            drawCharSelect();
            break;
        case STATE_LEVEL_SELECT:
            drawLevelSelect();
            break;
        case STATE_GAME:
            drawPlayingState();
            break;
        case STATE_GAME_OVER:
            drawGameOver();
            break;
        case STATE_EXIT:
            // 退出游戏
            break;
    }
    
    FlushBatchDraw();
}

// --- 绘制主菜单 ---
void drawMenu() {
    // 绘制渐变背景
    for (int i = 0; i < WIN_HEIGHT; i++) {
        int r = 30 + i * 20 / WIN_HEIGHT;
        int g = 30 + i * 40 / WIN_HEIGHT;
        int b = 50 + i * 60 / WIN_HEIGHT;
        setlinecolor(RGB(r, g, b));
        line(0, i, WIN_WIDTH, i);
    }
    
    // 绘制星空
    setfillcolor(RGB(255, 255, 200));
    for (int i = 0; i < 100; i++) {
        int x = (rand() + frameCount) % WIN_WIDTH;
        int y = (rand() + frameCount / 2) % WIN_HEIGHT;
        if (i % 7 == 0) {
            solidcircle(x, y, 2);
        } else {
            solidcircle(x, y, 1);
        }
    }
    
    // 绘制标题
    settextcolor(RGB(100, 255, 100));
    settextstyle(80, 0, _T("Consolas"));
    outtextxy(WIN_WIDTH / 2 - 200, WIN_HEIGHT / 4 - 50, _T("DINO RUN"));
    
    // 绘制装饰性恐龙
    setfillcolor(RGB(80, 180, 80));
    for (int i = 0; i < 3; i++) {
        int x = 150 + i * 200;
        int y = WIN_HEIGHT / 2 - 50 + 20 * sin(frameCount * 0.05 + i);
        fillrectangle(x - 20, y, x + 20, y + 80);
        fillrectangle(x - 10, y - 15, x + 10, y + 10);
    }
    
    // 绘制菜单选项
    drawButton(WIN_WIDTH / 2 - 100, WIN_HEIGHT / 2 + 100, 200, 50, "开始游戏", 1);
    
    settextcolor(RGB(200, 200, 255));
    settextstyle(20, 0, _T("Consolas"));
    outtextxy(WIN_WIDTH / 2 - 150, WIN_HEIGHT - 80, _T("按空格键开始游戏"));
    outtextxy(WIN_WIDTH / 2 - 120, WIN_HEIGHT - 50, _T("按ESC键退出游戏"));
    
    // 绘制最高分
    char scoreText[100];
    sprintf(scoreText, "历史最高分: %d", highScore);
    settextcolor(RGB(255, 255, 100));
    settextstyle(25, 0, _T("Consolas"));
    outtextxy(WIN_WIDTH / 2 - 100, WIN_HEIGHT / 2 + 170, scoreText);
}

// --- 绘制角色选择界面 ---
void drawCharSelect() {
    // 绘制背景
    setfillcolor(RGB(40, 40, 80));
    solidrectangle(0, 0, WIN_WIDTH, WIN_HEIGHT);
    
    // 绘制标题
    settextcolor(RGB(255, 255, 200));
    settextstyle(50, 0, _T("Consolas"));
    outtextxy(WIN_WIDTH / 2 - 150, 50, _T("选择角色"));
    
    // 绘制角色预览
    int startX = WIN_WIDTH / 2 - 250;
    for (int i = 0; i < CHAR_COUNT; i++) {
        int x = startX + i * 250;
        int y = WIN_HEIGHT / 2 - 50;
        drawCharPreview(x, y, (CharacterType)i, selectedChar == i);
    }
    
    // 绘制控制提示
    settextcolor(RGB(200, 200, 255));
    settextstyle(20, 0, _T("Consolas"));
    outtextxy(WIN_WIDTH / 2 - 200, WIN_HEIGHT - 150, _T("← → 键选择角色"));
    outtextxy(WIN_WIDTH / 2 - 150, WIN_HEIGHT - 120, _T("空格键确认选择"));
    outtextxy(WIN_WIDTH / 2 - 150, WIN_HEIGHT - 90, _T("ESC键返回主菜单"));
    
    // 绘制当前角色信息
    CharacterConfig* config = &charConfigs[selectedChar];
    settextcolor(RGB(255, 255, 100));
    settextstyle(25, 0, _T("Consolas"));
    outtextxy(WIN_WIDTH / 2 - 100, WIN_HEIGHT - 200, config->name);
    
    // 绘制角色属性
    settextcolor(RGB(200, 255, 200));
    settextstyle(18, 0, _T("Consolas"));
    outtextxy(WIN_WIDTH / 2 - 200, WIN_HEIGHT - 250, _T("速度: "));
    outtextxy(WIN_WIDTH / 2 - 200, WIN_HEIGHT - 220, _T("跳跃: "));
    outtextxy(WIN_WIDTH / 2 - 200, WIN_HEIGHT - 190, _T("生命: "));
    
    char buffer[50];
    sprintf(buffer, "%.1fx", config->speedMultiplier);
    outtextxy(WIN_WIDTH / 2 - 120, WIN_HEIGHT - 250, buffer);
    
    sprintf(buffer, "%.1fx", config->jumpMultiplier);
    outtextxy(WIN_WIDTH / 2 - 120, WIN_HEIGHT - 220, buffer);
    
    sprintf(buffer, "%d", (selectedChar == CHAR_TANK) ? 3 : 1);
    outtextxy(WIN_WIDTH / 2 - 120, WIN_HEIGHT - 190, buffer);
}

// --- 绘制难度选择界面 ---
void drawLevelSelect() {
    // 绘制背景
    setfillcolor(RGB(40, 80, 40));
    solidrectangle(0, 0, WIN_WIDTH, WIN_HEIGHT);
    
    // 绘制标题
    settextcolor(RGB(255, 255, 200));
    settextstyle(50, 0, _T("Consolas"));
    outtextxy(WIN_WIDTH / 2 - 150, 50, _T("选择难度"));
    
    // 绘制当前选择的角色
    CharacterConfig* charConfig = &charConfigs[selectedChar];
    settextcolor(RGB(charConfig->colorR, charConfig->colorG, charConfig->colorB));
    settextstyle(25, 0, _T("Consolas"));
    outtextxy(100, 120, charConfig->name);
    drawCharPreview(50, 150, selectedChar, 1);
    
    // 绘制难度预览
    int startX = WIN_WIDTH / 2 - 200;
    for (int i = 0; i < LEVEL_COUNT; i++) {
        int x = startX + i * 200;
        int y = WIN_HEIGHT / 2 - 50;
        drawLevelPreview(x, y, (DifficultyLevel)i, selectedLevel == i);
    }
    
    // 绘制控制提示
    settextcolor(RGB(200, 200, 255));
    settextstyle(20, 0, _T("Consolas"));
    outtextxy(WIN_WIDTH / 2 - 200, WIN_HEIGHT - 150, _T("← → 键选择难度"));
    outtextxy(WIN_WIDTH / 2 - 150, WIN_HEIGHT - 120, _T("空格键开始游戏"));
    outtextxy(WIN_WIDTH / 2 - 150, WIN_HEIGHT - 90, _T("ESC键返回角色选择"));
    
    // 绘制当前难度信息
    GameConfig* config = &levelConfigs[selectedLevel];
    settextcolor(RGB(255, 255, 100));
    settextstyle(25, 0, _T("Consolas"));
    outtextxy(WIN_WIDTH / 2 - 100, WIN_HEIGHT - 200, config->name);
    
    // 绘制难度参数
    settextcolor(RGB(200, 255, 200));
    settextstyle(18, 0, _T("Consolas"));
    outtextxy(WIN_WIDTH / 2 - 200, WIN_HEIGHT - 250, _T("速度: "));
    outtextxy(WIN_WIDTH / 2 - 200, WIN_HEIGHT - 220, _T("障碍物: "));
    outtextxy(WIN_WIDTH / 2 - 200, WIN_HEIGHT - 190, _T("重力: "));
    
    char buffer[50];
    sprintf(buffer, "%.1fx", config->speed / 1000.0);
    outtextxy(WIN_WIDTH / 2 - 120, WIN_HEIGHT - 250, buffer);
    
    sprintf(buffer, "%d%%", config->obstacleDensity);
    outtextxy(WIN_WIDTH / 2 - 120, WIN_HEIGHT - 220, buffer);
    
    sprintf(buffer, "%d级", config->gravity);
    outtextxy(WIN_WIDTH / 2 - 120, WIN_HEIGHT - 190, buffer);
}

// --- 绘制游戏进行界面 ---
void drawPlayingState() {
    // 根据主题绘制背景
    GameConfig* config = &levelConfigs[selectedLevel];
    int themeColor = config->themeColor;
    
    if (themeColor == 2) {  // 夜晚
        setfillcolor(RGB(20, 20, 40));
        solidrectangle(0, 0, WIN_WIDTH, WIN_HEIGHT);
        drawNightSky();
    } else if (themeColor == 1) {  // 蓝色主题
        setfillcolor(RGB(150, 200, 255));
        solidrectangle(0, 0, WIN_WIDTH, WIN_HEIGHT);
    } else {  // 绿色主题
        setfillcolor(RGB(180, 230, 200));
        solidrectangle(0, 0, WIN_WIDTH, WIN_HEIGHT);
    }
    
    // 绘制游戏元素
    drawClouds();
    drawGround();
    drawObstacles();
    drawDino();
    drawScore();
}

// --- 绘制游戏结束界面 ---
void drawGameOver() {
    // 绘制半透明黑色背景
    setfillcolor(RGB(0, 0, 0));
    solidrectangle(0, 0, WIN_WIDTH, WIN_HEIGHT);
    
    // 游戏结束面板
    setfillcolor(RGB(40, 40, 70));
    int panelWidth = 500;
    int panelHeight = 300;
    int panelX = (WIN_WIDTH - panelWidth) / 2;
    int panelY = (WIN_HEIGHT - panelHeight) / 2;
    
    fillrectangle(panelX, panelY, panelX + panelWidth, panelY + panelHeight);
    
    // 面板边框
    setlinecolor(RGB(100, 100, 150));
    rectangle(panelX, panelY, panelX + panelWidth, panelY + panelHeight);
    rectangle(panelX + 5, panelY + 5, panelX + panelWidth - 5, panelY + panelHeight - 5);
    
    // 游戏结束文本
    settextcolor(RGB(255, 100, 100));
    settextstyle(50, 0, _T("Consolas"));
    outtextxy(panelX + 120, panelY + 30, _T("游戏结束"));
    
    // 分数显示
    char scoreText[100];
    sprintf(scoreText, "分数: %d", score);
    settextcolor(RGB(255, 255, 200));
    settextstyle(30, 0, _T("Consolas"));
    outtextxy(panelX + 150, panelY + 100, scoreText);
    
    // 最高分显示
    if (score > highScore) {
        highScore = score;
        settextcolor(RGB(255, 255, 100));
        outtextxy(panelX + 150, panelY + 140, _T("新纪录!"));
    }
    
    sprintf(scoreText, "最高分: %d", highScore);
    settextcolor(RGB(200, 200, 255));
    outtextxy(panelX + 150, panelY + 180, scoreText);
    
    // 重新开始提示
    settextcolor(RGB(200, 255, 200));
    settextstyle(20, 0, _T("Consolas"));
    outtextxy(panelX + 120, panelY + 230, _T("按空格键返回主菜单"));
    outtextxy(panelX + 140, panelY + 260, _T("按ESC键退出游戏"));
}

// --- 绘制按钮 ---
void drawButton(int x, int y, int width, int height, const char* text, int selected) {
    // 按钮背景
    if (selected) {
        setfillcolor(RGB(100, 200, 100));
    } else {
        setfillcolor(RGB(80, 160, 80));
    }
    solidrectangle(x, y, x + width, y + height);
    
    // 按钮边框
    if (selected) {
        setlinecolor(RGB(150, 255, 150));
    } else {
        setlinecolor(RGB(120, 200, 120));
    }
    rectangle(x, y, x + width, y + height);
    
    // 按钮文字
    settextcolor(RGB(255, 255, 255));
    settextstyle(height * 0.6, 0, _T("Consolas"));
    int textWidth = textwidth(text);
    int textHeight = textheight(text);
    outtextxy(x + (width - textWidth) / 2, y + (height - textHeight) / 2, text);
}

// --- 绘制角色预览 ---
void drawCharPreview(int x, int y, CharacterType type, int selected) {
    CharacterConfig* config = &charConfigs[type];
    
    // 背景框
    if (selected) {
        setfillcolor(RGB(config->colorR + 50, config->colorG + 50, config->colorB + 50));
        setlinecolor(RGB(255, 255, 100));
    } else {
        setfillcolor(RGB(config->colorR, config->colorG, config->colorB));
        setlinecolor(RGB(200, 200, 200));
    }
    
    fillrectangle(x - 60, y - 60, x + 60, y + 100);
    rectangle(x - 60, y - 60, x + 60, y + 100);
    
    if (selected) {
        rectangle(x - 55, y - 55, x + 55, y + 95);
    }
    
    // 绘制恐龙
    setfillcolor(RGB(config->colorR, config->colorG, config->colorB));
    fillrectangle(x - 15, y, x + 15, y + 70);
    fillrectangle(x - 10, y - 15, x + 10, y + 10);
    
    // 眼睛
    setfillcolor(RGB(255, 255, 255));
    solidcircle(x - 5, y - 5, 4);
    setfillcolor(RGB(0, 0, 0));
    solidcircle(x - 4, y - 5, 2);
    
    // 角色名称
    settextcolor(RGB(255, 255, 255));
    settextstyle(20, 0, _T("Consolas"));
    outtextxy(x - textwidth(config->name) / 2, y + 80, config->name);
}

// --- 绘制难度预览 ---
void drawLevelPreview(int x, int y, DifficultyLevel level, int selected) {
    GameConfig* config = &levelConfigs[level];
    
    // 根据难度设置颜色
    COLORREF bgColor, borderColor;
    switch (level) {
        case LEVEL_EASY:
            bgColor = RGB(100, 200, 100);
            borderColor = selected ? RGB(150, 255, 150) : RGB(80, 180, 80);
            break;
        case LEVEL_NORMAL:
            bgColor = RGB(100, 150, 255);
            borderColor = selected ? RGB(150, 200, 255) : RGB(80, 130, 230);
            break;
        case LEVEL_HARD:
            bgColor = RGB(255, 100, 100);
            borderColor = selected ? RGB(255, 150, 150) : RGB(230, 80, 80);
            break;
        default:
            bgColor = RGB(150, 150, 150);
            borderColor = RGB(200, 200, 200);
    }
    
    // 背景框
    setfillcolor(bgColor);
    setlinecolor(borderColor);
    fillrectangle(x - 70, y - 70, x + 70, y + 70);
    rectangle(x - 70, y - 70, x + 70, y + 70);
    
    if (selected) {
        rectangle(x - 65, y - 65, x + 65, y + 65);
    }
    
    // 难度图标
    setfillcolor(RGB(255, 255, 200));
    if (level == LEVEL_EASY) {
        // 简单的仙人掌
        fillrectangle(x - 10, y - 20, x + 10, y + 20);
        setlinecolor(RGB(80, 140, 80));
        line(x, y - 15, x, y + 15);
    } else if (level == LEVEL_NORMAL) {
        // 中等的仙人掌和鸟
        fillrectangle(x - 15, y - 25, x - 5, y + 10);
        setfillcolor(RGB(200, 100, 100));
        fillellipse(x + 5, y - 10, x + 25, y + 10);
    } else {
        // 困难的仙人掌、鸟和闪电
        fillrectangle(x - 20, y - 30, x - 10, y + 15);
        setfillcolor(RGB(200, 100, 100));
        fillellipse(x, y - 15, x + 15, y + 5);
        setfillcolor(RGB(255, 255, 0));
        int points[8] = {x + 25, y - 25, x + 35, y - 15, x + 25, y - 5, x + 35, y + 5};
        solidpolygon((POINT*)points, 4);
    }
    
    // 难度名称
    settextcolor(RGB(255, 255, 255));
    settextstyle(25, 0, _T("Consolas"));
    outtextxy(x - textwidth(config->name) / 2, y + 80, config->name);
}

// --- 游戏核心函数（与之前相同，略作调整） ---
void drawNightSky() {
    setfillcolor(RGB(255, 255, 200));
    for (int i = 0; i < 150; i++) {
        int x = (rand() + frameCount) % WIN_WIDTH;
        int y = (rand() + frameCount / 2) % (GROUND_Y - 100);
        int size = rand() % 3 + 1;
        if (x % 4 == 0) {
            solidcircle(x, y, size);
        }
    }
}

void drawDino() {
    CharacterConfig* config = &charConfigs[dino.type];
    COLORREF dinoColor = RGB(config->colorR, config->colorG, config->colorB);
    COLORREF eyeColor = RGB(255, 255, 255);
    COLORREF pupilColor = RGB(0, 0, 0);
    
    setfillcolor(dinoColor);
    if (dino.isDucking) {
        // 下蹲状态
        fillrectangle(dino.x, dino.y + 20, dino.x + dino.width, dino.y + dino.height);
        fillrectangle(dino.x + 10, dino.y + 10, dino.x + dino.width - 10, dino.y + 40);
        
        setfillcolor(eyeColor);
        solidcircle(dino.x + 25, dino.y + 20, 5);
        setfillcolor(pupilColor);
        solidcircle(dino.x + 26, dino.y + 20, 2);
        
        setfillcolor(dinoColor);
        if (frameCount % 20 < 10) {
            fillrectangle(dino.x + 5, dino.y + dino.height - 10, dino.x + 15, dino.y + dino.height);
            fillrectangle(dino.x + 25, dino.y + dino.height - 5, dino.x + 35, dino.y + dino.height);
        } else {
            fillrectangle(dino.x + 5, dino.y + dino.height - 5, dino.x + 15, dino.y + dino.height);
            fillrectangle(dino.x + 25, dino.y + dino.height - 10, dino.x + 35, dino.y + dino.height);
        }
    } else {
        // 站立/跳跃状态
        fillrectangle(dino.x, dino.y, dino.x + dino.width, dino.y + dino.height);
        fillrectangle(dino.x + 10, dino.y - 15, dino.x + dino.width - 10, dino.y + 10);
        
        setfillcolor(eyeColor);
        solidcircle(dino.x + 25, dino.y - 5, 5);
        setfillcolor(pupilColor);
        solidcircle(dino.x + 26, dino.y - 5, 2);
        
        setlinecolor(RGB(0, 0, 0));
        line(dino.x + 30, dino.y, dino.x + 35, dino.y - 5);
        
        if (!dino.isJumping) {
            setfillcolor(dinoColor);
            if (frameCount % 20 < 10) {
                fillrectangle(dino.x + 5, dino.y + dino.height - 10, dino.x + 15, dino.y + dino.height);
                fillrectangle(dino.x + 25, dino.y + dino.height - 5, dino.x + 35, dino.y + dino.height);
            } else {
                fillrectangle(dino.x + 5, dino.y + dino.height - 5, dino.x + 15, dino.y + dino.height);
                fillrectangle(dino.x + 25, dino.y + dino.height - 10, dino.x + 35, dino.y + dino.height);
            }
        }
    }
    
    // 绘制生命值（如果有）
    if (dino.lives > 1) {
        settextcolor(RGB(255, 100, 100));
        settextstyle(15, 0, _T("Consolas"));
        char livesText[20];
        sprintf(livesText, "x%d", dino.lives);
        outtextxy(dino.x + dino.width + 5, dino.y, livesText);
    }
}

void updateDino() {
    // 应用重力（根据角色配置调整）
    CharacterConfig* config = &charConfigs[dino.type];
    float effectiveGravity = GRAVITY * config->gravityMultiplier;
    dino.velocityY += effectiveGravity;
    dino.y += dino.velocityY;
    
    if (dino.y >= GROUND_Y - dino.height) {
        dino.y = GROUND_Y - dino.height;
        dino.velocityY = 0;
        dino.isJumping = 0;
    }
    
    if (dino.isDucking) {
        dino.height = DINO_HEIGHT - 20;
        dino.y = GROUND_Y - dino.height;
    } else {
        dino.height = DINO_HEIGHT;
    }
}

void drawGround() {
    GameConfig* config = &levelConfigs[selectedLevel];
    COLORREF groundColor;
    
    switch (config->themeColor) {
        case 0: groundColor = RGB(220, 200, 170); break;  // 绿色主题
        case 1: groundColor = RGB(200, 220, 240); break;  // 蓝色主题
        case 2: groundColor = RGB(60, 60, 70); break;     // 夜晚主题
        default: groundColor = RGB(220, 200, 170);
    }
    
    setfillcolor(groundColor);
    solidrectangle(0, GROUND_Y, WIN_WIDTH, WIN_HEIGHT);
    
    // 提取颜色分量
    int r = GetRValue(groundColor);
    int g = GetGValue(groundColor);
    int b = GetBValue(groundColor);
    
    // 地面纹理
    setlinecolor(RGB((int)(r * 0.9), (int)(g * 0.9), (int)(b * 0.9)));
    for (int i = -(frameCount * gameSpeed) % 20; i < WIN_WIDTH; i += 20) {
        line(i, GROUND_Y, i + 10, GROUND_Y + 5);
    }
    
    // 地平线
    setlinecolor(RGB((int)(r * 0.8), (int)(g * 0.8), (int)(b * 0.8)));
    line(0, GROUND_Y, WIN_WIDTH, GROUND_Y);
}

void generateObstacle() {
    if (obstacleCount >= MAX_OBSTACLES_ON_SCREEN) return;
    if (framesSinceLastObstacle < nextSpawnInterval) return;
    
    int availableSlot = -1;
    for (int i = 0; i < 5; i++) {
        if (obstacles[i].passed) {
            availableSlot = i;
            break;
        }
    }
    if (availableSlot == -1) return;
    
    int minDistance = WIN_WIDTH;
    for (int i = 0; i < 5; i++) {
        if (!obstacles[i].passed) {
            int distance = WIN_WIDTH - obstacles[i].x;
            if (distance < minDistance) minDistance = distance;
        }
    }
    
    if (obstacleCount > 0 && minDistance < nextMinDistance) {
        if (framesSinceLastObstacle > MAX_SPAWN_INTERVAL * 2) {
            // 强制生成
        } else {
            return;
        }
    }
    
    // 根据难度调整障碍物类型概率
    GameConfig* config = &levelConfigs[selectedLevel];
    int type;
    int typeRand = rand() % 100;
    
    if (typeRand < 40) type = 0;
    else if (typeRand < 40 + config->obstacleDensity * 0.35) type = 1;
    else type = 2;
    
    for (int i = 0; i < 5; i++) {
        if (!obstacles[i].passed && obstacles[i].type == type) {
            if (rand() % 2 == 0) type = (type + 1) % 3;
            break;
        }
    }
    
    obstacles[availableSlot].type = type;
    obstacles[availableSlot].x = WIN_WIDTH;
    obstacles[availableSlot].passed = 0;
    
    if (type == 0) {
        obstacles[availableSlot].width = CACTUS_WIDTH;
        obstacles[availableSlot].height = CACTUS_HEIGHT;
        obstacles[availableSlot].y = GROUND_Y - CACTUS_HEIGHT;
    } else if (type == 1) {
        obstacles[availableSlot].width = CACTUS_WIDTH + 10;
        obstacles[availableSlot].height = CACTUS_HEIGHT + 20;
        obstacles[availableSlot].y = GROUND_Y - (CACTUS_HEIGHT + 20);
    } else {
        obstacles[availableSlot].width = BIRD_WIDTH;
        obstacles[availableSlot].height = BIRD_HEIGHT;
        obstacles[availableSlot].y = GROUND_Y - config->birdHeight - (rand() % 30);
    }
    
    obstacleCount++;
    framesSinceLastObstacle = 0;
    
    // 更新生成参数
    int baseSpawnInterval = 35 - (gameSpeed - GAME_SPEED) * 2;
    if (baseSpawnInterval < MIN_SPAWN_INTERVAL) baseSpawnInterval = MIN_SPAWN_INTERVAL;
    nextSpawnInterval = baseSpawnInterval + rand() % 20;
    if (nextSpawnInterval < MIN_SPAWN_INTERVAL) nextSpawnInterval = MIN_SPAWN_INTERVAL;
    if (nextSpawnInterval > MAX_SPAWN_INTERVAL) nextSpawnInterval = MAX_SPAWN_INTERVAL;
}

void updateObstacles() {
    for (int i = 0; i < 5; i++) {
        if (!obstacles[i].passed) {
            obstacles[i].x -= gameSpeed;
            
            if (obstacles[i].x + obstacles[i].width < 0) {
                obstacles[i].passed = 1;
                obstacleCount--;
                framesSinceLastObstacle = nextSpawnInterval;
            }
            
            if (obstacles[i].x + obstacles[i].width < dino.x && !obstacles[i].passed) {
                obstacles[i].passed = 1;
                score += 10;
                
                if (score % 500 == 0 && gameSpeed < 25) {
                    gameSpeed += 1;
                }
            }
        }
    }
    
    framesSinceLastObstacle++;
    
    if (framesSinceLastObstacle > MAX_SPAWN_INTERVAL) {
        generateObstacle();
        return;
    }
    
    if (obstacleCount < MAX_OBSTACLES_ON_SCREEN && framesSinceLastObstacle > nextSpawnInterval) {
        int baseChance = 10 + (gameSpeed - GAME_SPEED) * 3;
        int randomChance = baseChance - 5 + rand() % 11;
        if (randomChance < 5) randomChance = 5;
        
        if (rand() % 100 < randomChance) {
            generateObstacle();
            return;
        }
    }
    
    if (obstacleCount == 0 && framesSinceLastObstacle > 20) {
        generateObstacle();
        return;
    }
}

void drawObstacles() {
    for (int i = 0; i < 5; i++) {
        if (!obstacles[i].passed) {
            if (obstacles[i].type == 0 || obstacles[i].type == 1) {
                setfillcolor(nightMode ? RGB(80, 120, 80) : RGB(100, 160, 100));
                fillrectangle(obstacles[i].x, obstacles[i].y, 
                             obstacles[i].x + obstacles[i].width, 
                             obstacles[i].y + obstacles[i].height);
                
                setlinecolor(nightMode ? RGB(60, 100, 60) : RGB(80, 140, 80));
                line(obstacles[i].x + obstacles[i].width / 2, obstacles[i].y + 5,
                     obstacles[i].x + obstacles[i].width / 2, obstacles[i].y + obstacles[i].height - 5);
                
                if (obstacles[i].type == 1) {
                    fillrectangle(obstacles[i].x - 5, obstacles[i].y + 20,
                                 obstacles[i].x, obstacles[i].y + 30);
                    fillrectangle(obstacles[i].x + obstacles[i].width, obstacles[i].y + 10,
                                 obstacles[i].x + obstacles[i].width + 5, obstacles[i].y + 20);
                }
            } else {
                setfillcolor(nightMode ? RGB(120, 100, 120) : RGB(200, 100, 100));
                fillellipse(obstacles[i].x, obstacles[i].y,
                           obstacles[i].x + obstacles[i].width, obstacles[i].y + obstacles[i].height);
                
                setfillcolor(nightMode ? RGB(100, 80, 100) : RGB(180, 80, 80));
                if (frameCount % 20 < 10) {
                    fillrectangle(obstacles[i].x - 5, obstacles[i].y + 5,
                                 obstacles[i].x + 5, obstacles[i].y + obstacles[i].height - 5);
                } else {
                    fillrectangle(obstacles[i].x + obstacles[i].width - 5, obstacles[i].y + 5,
                                 obstacles[i].x + obstacles[i].width + 5, obstacles[i].y + obstacles[i].height - 5);
                }
                
                setfillcolor(RGB(255, 255, 255));
                solidcircle(obstacles[i].x + obstacles[i].width - 8, obstacles[i].y + 5, 3);
                setfillcolor(RGB(0, 0, 0));
                solidcircle(obstacles[i].x + obstacles[i].width - 7, obstacles[i].y + 5, 1);
            }
        }
    }
}

void drawClouds() {
    for (int i = 0; i < cloudCount; i++) {
        setfillcolor(nightMode ? RGB(100, 100, 120) : RGB(250, 250, 255));
        fillellipse(clouds[i].x, clouds[i].y, clouds[i].x + 40, clouds[i].y + 20);
        fillellipse(clouds[i].x + 15, clouds[i].y - 10, clouds[i].x + 55, clouds[i].y + 15);
        fillellipse(clouds[i].x + 30, clouds[i].y, clouds[i].x + 70, clouds[i].y + 20);
    }
}

void updateClouds() {
    for (int i = 0; i < cloudCount; i++) {
        clouds[i].x -= clouds[i].speed;
        
        if (clouds[i].x + 70 < 0) {
            clouds[i].x = WIN_WIDTH;
            clouds[i].y = 50 + rand() % 200;
            clouds[i].speed = 1 + rand() % 3;
        }
    }
}

void checkCollision() {
    for (int i = 0; i < 5; i++) {
        if (!obstacles[i].passed) {
            if (dino.x < obstacles[i].x + obstacles[i].width &&
                dino.x + dino.width > obstacles[i].x &&
                dino.y < obstacles[i].y + obstacles[i].height &&
                dino.y + dino.height > obstacles[i].y) {
                
                if (dino.lives > 1) {
                    dino.lives--;
                    obstacles[i].passed = 1;
                    obstacleCount--;
                } else {
                    gameState = STATE_GAME_OVER;
                }
            }
        }
    }
}

void drawScore() {
    char scoreText[100];
    
    // 分数
    sprintf(scoreText, "分数: %d", score);
    settextcolor(nightMode ? RGB(200, 200, 255) : RGB(80, 80, 120));
    settextstyle(20, 0, _T("Consolas"));
    outtextxy(20, 20, scoreText);
    
    // 最高分
    sprintf(scoreText, "最高分: %d", highScore);
    outtextxy(20, 50, scoreText);
    
    // 速度
    sprintf(scoreText, "速度: %d", gameSpeed);
    outtextxy(20, 80, scoreText);
    
    // 生命值（如果有）
    if (dino.lives > 1) {
        settextcolor(RGB(255, 100, 100));
        sprintf(scoreText, "生命: %d", dino.lives);
        outtextxy(20, 110, scoreText);
    }
    
    // 角色和难度信息
    CharacterConfig* charConfig = &charConfigs[selectedChar];
    GameConfig* levelConfig = &levelConfigs[selectedLevel];
    
    settextcolor(RGB(charConfig->colorR, charConfig->colorG, charConfig->colorB));
    outtextxy(WIN_WIDTH - 200, 20, charConfig->name);
    
    settextcolor(nightMode ? RGB(200, 200, 255) : RGB(100, 100, 150));
    outtextxy(WIN_WIDTH - 200, 50, levelConfig->name);
}

// --- 游戏引擎主函数 ---
void RunGame() {
    // 初始化图形窗口
    initgraph(WIN_WIDTH, WIN_HEIGHT);
    
    // 开启双缓冲
    BeginBatchDraw();
    
    // 设置随机种子
    srand((unsigned int)time(NULL));
    
    // 游戏主循环
    while (gameState != STATE_EXIT) {
        handleInput();
        updateGame();
        renderGame();
        
        // 控制帧率
        static DWORD lastTime = GetTickCount();
        DWORD currentTime = GetTickCount();
        DWORD deltaTime = currentTime - lastTime;
        
        if (deltaTime < 1000 / TARGET_FPS) {
            Sleep(1000 / TARGET_FPS - deltaTime);
        }
        
        lastTime = GetTickCount();
    }
    
    EndBatchDraw();
    closegraph();
}

// --- 主函数 ---
int main() {
    RunGame();
    return 0;
}