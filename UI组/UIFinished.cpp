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
#define DINO_WIDTH 80
#define DINO_HEIGHT 100
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
IMAGE pixelManImg[3];
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
void loadAllCharImages();

// --- 初始化函数 ---
void initGame() {
    // 获取当前选择的配置
    CharacterConfig* charConfig = &charConfigs[selectedChar];
    GameConfig* levelConfig = &levelConfigs[selectedLevel];
    // 设置恐龙尺寸
    dino.width = DINO_WIDTH;
    dino.height = DINO_HEIGHT;
    
    // 加载三个像素小人图片（从assets目录）
    const TCHAR* filenames[3] = {_T("assets/pixel_man1.png"), _T("assets/pixel_man2.png"), _T("assets/pixel_man3.png")};
    
    // 直接加载图片
    for (int i = 0; i < 3; i++) {
        loadimage(&pixelManImg[i], filenames[i], dino.width, dino.height,true);
    }
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
    //定义图片变量 int a;
    IMAGE img_mm;
    //加载图片 scanf("%d",&a);
    loadimage(&img_mm,"assets//mm.jpg",800,700);
    //输出图片 printf("%d",a);
    putimage(0,0,&img_mm);
    // 绘制标题
    settextcolor(RGB(255, 214, 0));
    settextstyle(60, 0, _T("Abaddon Bold"));
    setbkmode(TRANSPARENT);
    outtextxy(WIN_WIDTH / 2 - 200, WIN_HEIGHT / 4 + 10, _T("Speed of Sound"));
    // 绘制菜单选项
    drawButton(WIN_WIDTH / 2 - 100, WIN_HEIGHT / 4 + 80, 200, 50, "开始游戏", 1);
    
    settextcolor(RGB(255, 140, 0));
    settextstyle(25, 0, _T("黑体"));
    outtextxy(WIN_WIDTH / 2 - 100, WIN_HEIGHT / 4+180, _T("按空格键开始游戏"));
    outtextxy(WIN_WIDTH / 2 - 100, WIN_HEIGHT / 4+210, _T("按ESC键退出游戏"));
    
    // 绘制最高分
    char scoreText[100];
    sprintf(scoreText, "历史最高分: %d", highScore);
    settextcolor(RGB(255, 140, 0));
    settextstyle(25, 0, _T("宋体"));
    outtextxy(WIN_WIDTH / 2 - 100, WIN_HEIGHT / 4 + 150, scoreText);
}

// --- 绘制角色选择界面 ---
void drawCharSelect() {
    // 绘制背景
    IMAGE img_xx;
    loadimage(&img_xx,"assets//xx.jpg",800,700);
    putimage(0,0,&img_xx);
    // 绘制标题 - 居中橙红色宋体
    settextcolor(RGB(255, 100, 50)); // 橙红色
    settextstyle(50, 0, _T("宋体"));
    RECT titleRect = {0, 50, WIN_WIDTH, 120};
    drawtext(_T("选择角色"), &titleRect, DT_CENTER);
    // 绘制角色预览
    int startX = WIN_WIDTH / 2 - (CHAR_COUNT * 250) / 2 + 125;
    for (int i = 0; i < CHAR_COUNT; i++) {
        int x = startX + i * 250;
        int y = WIN_HEIGHT / 2 - 50;
        drawCharPreview(x, y, (CharacterType)i, selectedChar == i);
    // 在每个角色下方显示其特性
        CharacterConfig* config = &charConfigs[i];
        int attrY = WIN_HEIGHT / 2 + 30; // 调整到预览图下方
    // 角色属性
        settextcolor(RGB(255, 50, 150));
        settextstyle(16, 0, _T("宋体"));
        
        char buffer[50];
        sprintf(buffer, "速度: %.1fx", config->speedMultiplier);
        RECT speedRect = {x - 100, attrY + 35, x + 100, attrY + 60};
        drawtext(buffer, &speedRect, DT_CENTER);
        
        sprintf(buffer, "跳跃: %.1fx", config->jumpMultiplier);
        RECT jumpRect = {x - 100, attrY + 60, x + 100, attrY + 85};
        drawtext(buffer, &jumpRect, DT_CENTER);
        
    // 特殊处理普通龙的生命值显示
        int health = (i == CHAR_TANK) ? 3 : 1;
        sprintf(buffer, "生命: %d", health);
        RECT healthRect = {x - 100, attrY + 85, x + 100, attrY + 110};
        drawtext(buffer, &healthRect, DT_CENTER);
    }
    
    // 绘制控制提示 - 居中玫红色黑体
    settextcolor(RGB(255, 50, 150)); // 玫红色
    settextstyle(22, 0, _T("黑体"));
    // 计算提示信息总高度以便居中
    int totalHintHeight = 120;
    int hintStartY = WIN_HEIGHT - 150;
    
    //提示
    RECT hintRect1 = {0, hintStartY, WIN_WIDTH, hintStartY + 30};
    drawtext(_T("← → 键选择角色"), &hintRect1, DT_CENTER);
    RECT hintRect2 = {0, hintStartY + 35, WIN_WIDTH, hintStartY + 65};
    drawtext(_T("空格键确认选择"), &hintRect2, DT_CENTER);
    RECT hintRect3 = {0, hintStartY + 70, WIN_WIDTH, hintStartY + 100};
    drawtext(_T("ESC键返回主菜单"), &hintRect3, DT_CENTER);
    // 添加当前选中角色高亮指示器
    if (selectedChar >= 0 && selectedChar < CHAR_COUNT) {
        int indicatorX = startX + selectedChar * 250;
        int indicatorY = WIN_HEIGHT / 2 - 70;
        POINT triangle[3] = {
            {indicatorX - 15, indicatorY},
            {indicatorX + 15, indicatorY},
            {indicatorX, indicatorY + 15}
        };
        solidpolygon(triangle, 3);
        
        // 恢复默认线型
        setlinestyle(PS_SOLID, 1);
    }
}
    // --- 绘制难度选择界面 ---
void drawLevelSelect() {
   IMAGE img_ss;
   loadimage(&img_ss,"assets//ss.jpg",800,700);
   putimage(0,0,&img_ss);
    // 绘制标题
    settextcolor(RGB(255, 100, 50));  // 橙红色
    settextstyle(56, 0, _T("宋体"));
    RECT titleRect = {0, 40, WIN_WIDTH, 130};
    drawtext(_T("选择难度"), &titleRect, DT_CENTER);
    
// 绘制当前选择的角色
CharacterConfig* charConfig = &charConfigs[selectedChar];
settextcolor(RGB(charConfig->colorR, charConfig->colorG, charConfig->colorB));
settextstyle(25, 0, _T("Consolas"));
// 角色名称改为居中：计算文字宽度，然后调整x坐标使其居中
int textWidth = textwidth(charConfig->name);
int centerX = WIN_WIDTH / 2;
outtextxy(centerX - textWidth / 2, 120, charConfig->name);
drawCharPreview(50, 150, selectedChar, 1);

// 绘制难度预览
int startX = WIN_WIDTH / 2 - (LEVEL_COUNT * 200) / 2; // 调整起始位置使整体居中
for (int i = 0; i < LEVEL_COUNT; i++) {
    int x = startX + i * 200 + 100; // 每个预览中心点对齐
    int y = WIN_HEIGHT / 2 - 50;
    drawLevelPreview(x, y, (DifficultyLevel)i, selectedLevel == i);
}

// 绘制控制提示 
settextcolor(RGB(255, 69, 0)); 
settextstyle(20, 0, _T("Consolas"));

// 计算每行提示的居中位置
const TCHAR* hint1 = _T("← → 键选择难度");
const TCHAR* hint2 = _T("空格键开始游戏");
const TCHAR* hint3 = _T("ESC键返回角色选择");

int hint1Width = textwidth(hint1);
int hint2Width = textwidth(hint2);
int hint3Width = textwidth(hint3);

outtextxy(WIN_WIDTH / 2 - hint1Width / 2, WIN_HEIGHT - 140, hint1);
outtextxy(WIN_WIDTH / 2 - hint2Width / 2, WIN_HEIGHT - 110, hint2);
outtextxy(WIN_WIDTH / 2 - hint3Width / 2, WIN_HEIGHT - 80, hint3);

// 绘制当前难度信息
GameConfig* config = &levelConfigs[selectedLevel];
settextcolor(RGB(140, 80, 200)); // 
settextstyle(25, 0, _T("黑体"));

int levelNameWidth = textwidth(config->name);
outtextxy(WIN_WIDTH / 2 - levelNameWidth / 2, WIN_HEIGHT - 170, config->name);

// 绘制难度参数 
settextcolor(RGB(204, 85, 0)); 
settextstyle(18, 0, _T("Consolas"));

// 参数标签居中
const TCHAR* speedLabel = _T("速度: ");
const TCHAR* obstacleLabel = _T("障碍物: ");
const TCHAR* gravityLabel = _T("重力: ");

int labelWidth = textwidth(speedLabel); // 假设三个标签宽度相近
int valueStartX = WIN_WIDTH / 2 - labelWidth / 2 + labelWidth;

outtextxy(WIN_WIDTH / 2 - labelWidth / 2-30, WIN_HEIGHT - 250, speedLabel);
outtextxy(WIN_WIDTH / 2 - labelWidth / 2-30, WIN_HEIGHT - 220, obstacleLabel);
outtextxy(WIN_WIDTH / 2 - labelWidth / 2-30, WIN_HEIGHT - 190, gravityLabel);
settextcolor(RGB(204, 85, 0)); 

char buffer[50];
sprintf(buffer, "%.1fx", config->speed / 1000.0);
outtextxy(valueStartX, WIN_HEIGHT - 250, buffer);

sprintf(buffer, "%d%%", config->obstacleDensity);
outtextxy(valueStartX, WIN_HEIGHT - 220, buffer);

sprintf(buffer, "%d级", config->gravity);
outtextxy(valueStartX, WIN_HEIGHT - 190, buffer);
}
// --- 绘制游戏进行界面 ---
void drawPlayingState() {
    // 根据主题绘制背景
    GameConfig* config = &levelConfigs[selectedLevel];
    int themeColor = config->themeColor;
    IMAGE img_2,img_1,img_0;
    if (themeColor == 2) {  // 夜晚
        loadimage (&img_2,"assets//night.jpg",800,600);
        putimage(0,0,&img_2);
    } else if (themeColor == 1) {  // 蓝色主题
        loadimage (&img_1,"assets//sun.jpg",800,600);
        putimage(0,0,&img_1);
    } else {  // 绿色主题
        loadimage (&img_0,"assets//green.jpg",800,600);
        putimage(0,0,&img_0);
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
    IMAGE img_zz;
    loadimage(&img_zz,"assets//zz.jpg",800,700);
    putimage(0,0,&img_zz);
// 游戏结束面板
    setfillcolor(RGB(20, 50, 70));
    int panelWidth = 500;
    int panelHeight = 300;
    int panelX = (WIN_WIDTH - panelWidth) / 2;
    int panelY = (WIN_HEIGHT - panelHeight) / 2;
    
    fillrectangle(panelX, panelY, panelX + panelWidth, panelY + panelHeight);
    
    // 面板边框
    setlinecolor(RGB(255, 140, 0));
    rectangle(panelX, panelY, panelX + panelWidth, panelY + panelHeight);
    rectangle(panelX + 5, panelY + 5, panelX + panelWidth - 5, panelY + panelHeight - 5);
    
    // 游戏结束文本
    settextcolor(RGB(255, 69, 0));
    settextstyle(50, 0, _T("Consolas"));
    outtextxy(panelX + 120, panelY + 30, _T("游戏结束"));
    
    // 分数显示
    char scoreText[100];
    sprintf(scoreText, "分数: %d", score);
    settextcolor(RGB(255, 228, 100));
    settextstyle(30, 0, _T("Consolas"));
    outtextxy(panelX + 150, panelY + 100, scoreText);
    
    // 最高分显示
    if (score > highScore) {
        highScore = score;
        settextcolor(RGB(255, 228, 100));
        outtextxy(panelX + 150, panelY + 140, _T("新纪录!"));
    }
    
    sprintf(scoreText, "最高分: %d", highScore);
    settextcolor(RGB(255, 228, 100));
    outtextxy(panelX + 150, panelY + 180, scoreText);
    
    // 重新开始提示
    settextcolor(RGB(221, 0, 112));
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
// 全局IMAGE变量
IMAGE img_normal, img_speed, img_tank;

// 加载图片
void loadAllCharImages() {
    loadimage(&img_normal, "assets//hero.jpg", 120, 140);   // 普通龙=hero.jpg
    loadimage(&img_speed, "assets//hero2.jpg", 120, 140);  // 速度龙=hero2.jpg
    loadimage(&img_tank, "assets//hero3.jpg", 120, 140);   // 坦克龙=hero3.jpg
}
// --- 绘制角色预览 ---
void drawCharPreview(int x, int y, CharacterType type, int selected) {
    CharacterConfig* config = &charConfigs[type];
void loadAllCharImages();
    // 背景框
    if (selected) {
        
        setlinecolor(RGB(255, 255, 196));
    } else {
        
        setlinecolor(RGB(200, 200, 200));
    }
    fillrectangle(x - 60, y - 60, x + 60, y + 100);
    rectangle(x - 60, y - 60, x + 60, y + 100);

    if (selected) {
        rectangle(x - 55, y - 55, x + 55, y + 95);
    }

    // 核心：绘制jpg格式的小人图（对应type匹配hero/hero2/hero3）
    switch (type) {
    case CHAR_DEFAULT:
        putimage(x - 60, y - 60, 120, 140, &img_normal, 0, 0);
        break;
    case CHAR_SPEEDY:
        putimage(x - 60, y - 60, 120, 140, &img_speed, 0, 0);
        break;
    case CHAR_TANK:
        putimage(x - 60, y - 60, 120, 140, &img_tank, 0, 0);
        break;
    }

    // 角色名称
    settextcolor(RGB(199, 21, 133));
    settextstyle(20, 0, _T("Consolas"));
    int nameWidth = textwidth(config->name);
    outtextxy(x - nameWidth / 2, y + 80, config->name);
}
// --- 绘制难度预览 ---
void drawLevelPreview(int x, int y, DifficultyLevel level, int selected) {
    GameConfig* config = &levelConfigs[level];
    
    // 根据难度设置颜色
COLORREF bgColor, borderColor;
switch (level) {
    case LEVEL_EASY:
        bgColor = RGB(80, 220, 80); // 亮绿背景
        borderColor = selected ? RGB(100, 255, 100) : RGB(50, 180, 50); // 选中高亮绿
        break;
    case LEVEL_NORMAL:
        bgColor = RGB(80, 150, 255); // 亮蓝背景
        borderColor = selected ? RGB(100, 180, 255) : RGB(50, 120, 220); // 选中高亮蓝
        break;
    case LEVEL_HARD:
        bgColor = RGB(255, 80, 80); // 亮红背景
        borderColor = selected ? RGB(255, 100, 100) : RGB(220, 50, 50); // 选中高亮红
        break;
    default:
        bgColor = RGB(150, 150, 150);
        borderColor = RGB(200, 200, 200);
        break;
}

// 背景框
setfillcolor(bgColor);
setlinecolor(borderColor);
fillrectangle(x - 70, y - 70, x + 70, y + 70);
rectangle(x - 70, y - 70, x + 70, y + 70);

// 选中效果：白色加粗内层边框
if (selected) {
    setlinecolor(RGB(255, 255, 255));
    setlinestyle(PS_SOLID, 3);
    rectangle(x - 65, y - 65, x + 65, y + 65);
    setlinestyle(PS_SOLID, 1); // 恢复默认粗细
} 
    
    // 难度图标
    setfillcolor(RGB(255, 255, 200));
    IMAGE img_d1,img_d2,img_d3;
    if (level == LEVEL_EASY) {
        loadimage(&img_d1,"assets//difficulty1.jpg",100,120);
        putimage(x-50,y-60,&img_d1);
    } else if (level == LEVEL_NORMAL) {
        // 中等
        loadimage(&img_d2,"assets//difficulty2.jpg",100,120);
        putimage(x-50,y-60,&img_d2);
    } else {
        // 困难
        loadimage(&img_d3,"assets//difficulty3.jpg",100,120);
        putimage(x-50,y-60,&img_d3);
    }
    
    // 难度名称
    settextcolor(RGB(123, 31, 62));
    settextstyle(25, 0, _T("黑体"));
    outtextxy(x - textwidth(config->name) / 2, y + 80, config->name);
}

// --- 绘制恐龙（现在用像素小人替代） ---
void drawDino() {
    int drawY = dino.y;
    
    // 根据状态调整位置
    if (dino.isDucking) {
        drawY += 20;
    }
    
     // 绘制透明像素小人（核心修改部分）
     putimage(dino.x, drawY, &pixelManImg[dino.type],SRCPAINT);
    // 绘制生命值
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
        case 0: groundColor = RGB(240, 230, 210); break;  // 绿色主题
        case 1: groundColor = RGB(220, 215, 190); break;  // 蓝色主题
        case 2: groundColor = RGB(200, 220, 200); break;     // 夜晚主题
        default: groundColor = RGB(240, 230, 210);
    }
    
    setfillcolor(groundColor);
    solidrectangle(0, GROUND_Y, WIN_WIDTH, WIN_HEIGHT);
    
    // 提取颜色分量
    int r = GetRValue(groundColor);
    int g = GetGValue(groundColor);
    int b = GetBValue(groundColor);
    
    // 地面纹理
    setlinecolor(RGB((int)(r * 0.95), (int)(g * 0.95), (int)(b * 0.95)));
    for (int i = -(frameCount * gameSpeed) % 15; i < WIN_WIDTH; i += 15) {
        line(i, GROUND_Y, i + 8, GROUND_Y + 3);
    }
    
    // 地平线
    setlinecolor(RGB((int)(r * 0.9), (int)(g * 0.9), (int)(b * 0.9)));
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
    //加载角色图片
    loadAllCharImages();
    
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