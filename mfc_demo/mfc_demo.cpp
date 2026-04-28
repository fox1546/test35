// mfc_demo.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "mfc_demo.h"
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

#define MAX_LOADSTRING 100
#define MAX_OPPONENTS 5
#define TRACK_WIDTH 400
#define TRACK_HEIGHT 300
#define CAR_WIDTH 20
#define CAR_HEIGHT 12
#define TRACK_BORDER 20

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

struct GameSettings {
    int opponentCount;
    int keyUp;
    int keyDown;
    int keyLeft;
    int keyRight;
    
    GameSettings() : opponentCount(2), keyUp(VK_UP), keyDown(VK_DOWN), keyLeft(VK_LEFT), keyRight(VK_RIGHT) {}
};

struct Car {
    float x, y;
    float angle;
    float speed;
    float maxSpeed;
    int health;
    int lap;
    float progress;
    bool finished;
    int finishRank;
    COLORREF color;
    bool isPlayer;
    
    Car() : x(0), y(0), angle(0), speed(0), maxSpeed(3.0f), health(100), lap(0), progress(0), finished(false), finishRank(0), color(RGB(255, 0, 0)), isPlayer(false) {}
};

enum GameState {
    GAME_MENU,
    GAME_PLAYING,
    GAME_FINISHED
};

GameSettings g_settings;
GameState g_gameState = GAME_MENU;
std::vector<Car> g_cars;
int g_currentRank = 1;
bool g_keyStates[256] = {false};
UINT_PTR g_timerId = 0;

struct TrackPoint {
    float x, y;
    float width;
};

std::vector<TrackPoint> g_trackPath;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    SettingsDlg(HWND, UINT, WPARAM, LPARAM);
void                InitTrackPath();
void                StartGame(HWND hWnd);
void                UpdateGame();
void                DrawGame(HDC hdc, HWND hWnd);
void                DrawCar(HDC hdc, const Car& car);
bool                CheckCollision(const Car& car);
float               CalculateProgress(const Car& car);
std::wstring        GetKeyName(int vk);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MFCDEMO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    InitTrackPath();

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MFCDEMO));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

void InitTrackPath()
{
    g_trackPath.clear();
    int centerX = 400;
    int centerY = 300;
    float radiusX = 280.0f;
    float radiusY = 180.0f;
    
    for (int i = 0; i < 100; i++) {
        float angle = (i / 100.0f) * 6.28318f;
        TrackPoint pt;
        pt.x = centerX + radiusX * cos(angle);
        pt.y = centerY + radiusY * sin(angle);
        pt.width = 60.0f;
        g_trackPath.push_back(pt);
    }
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MFCDEMO));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MFCDEMO);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 850, 650, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

std::wstring GetKeyName(int vk)
{
    WCHAR buffer[64] = {0};
    UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    
    switch (vk) {
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_INSERT:
        case VK_DELETE:
        case VK_DIVIDE:
        case VK_NUMLOCK:
            scanCode |= 0x100;
            break;
    }
    
    if (GetKeyNameTextW(scanCode << 16, buffer, 64)) {
        return buffer;
    }
    
    std::wostringstream ss;
    ss << L"Key " << vk;
    return ss.str();
}

INT_PTR CALLBACK SettingsDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static int* currentKey = nullptr;
    
    switch (message)
    {
    case WM_INITDIALOG:
        {
            HWND hCombo = GetDlgItem(hDlg, IDC_OPPONENT_COUNT);
            for (int i = 1; i <= MAX_OPPONENTS; i++) {
                std::wostringstream ss;
                ss << i;
                ComboBox_AddString(hCombo, ss.str().c_str());
            }
            ComboBox_SetCurSel(hCombo, g_settings.opponentCount - 1);
            
            SetDlgItemText(hDlg, IDC_KEY_UP, GetKeyName(g_settings.keyUp).c_str());
            SetDlgItemText(hDlg, IDC_KEY_DOWN, GetKeyName(g_settings.keyDown).c_str());
            SetDlgItemText(hDlg, IDC_KEY_LEFT, GetKeyName(g_settings.keyLeft).c_str());
            SetDlgItemText(hDlg, IDC_KEY_RIGHT, GetKeyName(g_settings.keyRight).c_str());
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            int notifyCode = HIWORD(wParam);
            
            if (wmId == IDC_KEY_UP || wmId == IDC_KEY_DOWN || 
                wmId == IDC_KEY_LEFT || wmId == IDC_KEY_RIGHT) {
                if (notifyCode == EN_SETFOCUS) {
                    switch (wmId) {
                        case IDC_KEY_UP: currentKey = &g_settings.keyUp; break;
                        case IDC_KEY_DOWN: currentKey = &g_settings.keyDown; break;
                        case IDC_KEY_LEFT: currentKey = &g_settings.keyLeft; break;
                        case IDC_KEY_RIGHT: currentKey = &g_settings.keyRight; break;
                    }
                    SetDlgItemText(hDlg, wmId, L"按新键...");
                }
            }
            
            if (wmId == IDOK) {
                HWND hCombo = GetDlgItem(hDlg, IDC_OPPONENT_COUNT);
                int sel = ComboBox_GetCurSel(hCombo);
                if (sel >= 0) {
                    g_settings.opponentCount = sel + 1;
                }
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            else if (wmId == IDCANCEL) {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
        }
        break;
        
    case WM_KEYDOWN:
        if (currentKey && wParam != VK_SHIFT && wParam != VK_CONTROL && wParam != VK_MENU) {
            *currentKey = (int)wParam;
            int ctrlId = 0;
            if (currentKey == &g_settings.keyUp) ctrlId = IDC_KEY_UP;
            else if (currentKey == &g_settings.keyDown) ctrlId = IDC_KEY_DOWN;
            else if (currentKey == &g_settings.keyLeft) ctrlId = IDC_KEY_LEFT;
            else if (currentKey == &g_settings.keyRight) ctrlId = IDC_KEY_RIGHT;
            
            if (ctrlId > 0) {
                SetDlgItemText(hDlg, ctrlId, GetKeyName((int)wParam).c_str());
            }
            currentKey = nullptr;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void StartGame(HWND hWnd)
{
    g_cars.clear();
    g_currentRank = 1;
    g_gameState = GAME_PLAYING;
    
    Car player;
    player.isPlayer = true;
    player.color = RGB(255, 50, 50);
    player.x = g_trackPath[0].x;
    player.y = g_trackPath[0].y - 30;
    player.angle = 1.57f;
    player.maxSpeed = 4.0f;
    g_cars.push_back(player);
    
    COLORREF opponentColors[] = {
        RGB(50, 50, 255),
        RGB(50, 255, 50),
        RGB(255, 255, 50),
        RGB(255, 50, 255),
        RGB(50, 255, 255)
    };
    
    for (int i = 0; i < g_settings.opponentCount; i++) {
        Car opponent;
        opponent.isPlayer = false;
        opponent.color = opponentColors[i % 5];
        opponent.x = g_trackPath[0].x;
        opponent.y = g_trackPath[0].y + (i + 1) * 30 - 30;
        opponent.angle = 1.57f;
        opponent.maxSpeed = 2.5f + (rand() % 10) * 0.1f;
        g_cars.push_back(opponent);
    }
    
    if (g_timerId) {
        KillTimer(hWnd, g_timerId);
    }
    g_timerId = SetTimer(hWnd, 1, 20, nullptr);
    
    InvalidateRect(hWnd, nullptr, TRUE);
}

float CalculateProgress(const Car& car)
{
    float minDist = 10000.0f;
    int nearestIdx = 0;
    
    for (size_t i = 0; i < g_trackPath.size(); i++) {
        float dx = car.x - g_trackPath[i].x;
        float dy = car.y - g_trackPath[i].y;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist < minDist) {
            minDist = dist;
            nearestIdx = (int)i;
        }
    }
    
    return (float)nearestIdx / g_trackPath.size() + car.lap;
}

bool CheckCollision(const Car& car)
{
    float minDist = 10000.0f;
    
    for (size_t i = 0; i < g_trackPath.size(); i++) {
        float dx = car.x - g_trackPath[i].x;
        float dy = car.y - g_trackPath[i].y;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist < minDist) {
            minDist = dist;
        }
    }
    
    return minDist > g_trackPath[0].width / 2 + CAR_WIDTH / 2;
}

void UpdateGame()
{
    if (g_gameState != GAME_PLAYING) return;
    
    int finishedCount = 0;
    
    for (size_t i = 0; i < g_cars.size(); i++) {
        Car& car = g_cars[i];
        
        if (car.finished) {
            finishedCount++;
            continue;
        }
        
        if (car.isPlayer) {
            if (g_keyStates[g_settings.keyUp]) {
                car.speed += 0.15f;
            }
            if (g_keyStates[g_settings.keyDown]) {
                car.speed -= 0.1f;
            }
            if (g_keyStates[g_settings.keyLeft]) {
                car.angle -= 0.05f * (car.speed > 0.5f ? 1.0f : 0.3f);
            }
            if (g_keyStates[g_settings.keyRight]) {
                car.angle += 0.05f * (car.speed > 0.5f ? 1.0f : 0.3f);
            }
        } else {
            float targetProgress = car.progress + 0.02f;
            int targetIdx = (int)(fmod(targetProgress, 1.0f) * g_trackPath.size());
            if (targetIdx >= (int)g_trackPath.size()) targetIdx = 0;
            
            TrackPoint& target = g_trackPath[targetIdx];
            float dx = target.x - car.x;
            float dy = target.y - car.y;
            float targetAngle = atan2(dy, dx);
            
            float angleDiff = targetAngle - car.angle;
            while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
            while (angleDiff < -3.14159f) angleDiff += 6.28318f;
            
            car.angle += angleDiff * 0.08f;
            car.speed += 0.12f;
        }
        
        if (car.speed > car.maxSpeed) car.speed = car.maxSpeed;
        if (car.speed < -1.0f) car.speed = -1.0f;
        if (car.speed > 0) car.speed *= 0.98f;
        else car.speed *= 0.95f;
        
        float oldProgress = car.progress;
        car.x += cos(car.angle) * car.speed;
        car.y += sin(car.angle) * car.speed;
        car.progress = CalculateProgress(car);
        
        if (car.progress - oldProgress < -0.5f) {
            car.lap++;
            car.progress += 1.0f;
        }
        
        if (CheckCollision(car)) {
            car.health -= 2;
            car.speed *= 0.7f;
            if (car.health <= 0) {
                car.health = 0;
                car.speed = 0;
            }
        }
        
        if (car.lap >= 3) {
            car.finished = true;
            car.finishRank = g_currentRank++;
        }
    }
    
    if (finishedCount == (int)g_cars.size()) {
        g_gameState = GAME_FINISHED;
    }
}

void DrawCar(HDC hdc, const Car& car)
{
    float points[4][2] = {
        {CAR_WIDTH/2, 0},
        {-CAR_WIDTH/2, -CAR_HEIGHT/2},
        {-CAR_WIDTH/3, 0},
        {-CAR_WIDTH/2, CAR_HEIGHT/2}
    };
    
    POINT polyPoints[4];
    for (int i = 0; i < 4; i++) {
        float rx = points[i][0] * cos(car.angle) - points[i][1] * sin(car.angle);
        float ry = points[i][0] * sin(car.angle) + points[i][1] * cos(car.angle);
        polyPoints[i].x = (int)(car.x + rx);
        polyPoints[i].y = (int)(car.y + ry);
    }
    
    COLORREF drawColor = car.color;
    if (car.health < 30 && car.health > 0) {
        drawColor = RGB(128, 128, 128);
    }
    if (car.health <= 0) {
        drawColor = RGB(64, 64, 64);
    }
    
    HBRUSH hBrush = CreateSolidBrush(drawColor);
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    Polygon(hdc, polyPoints, 4);
    
    if (car.isPlayer) {
        HPEN hIndicatorPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 0));
        HPEN hOldIndPen = (HPEN)SelectObject(hdc, hIndicatorPen);
        Arc(hdc, (int)car.x - 15, (int)car.y - 15, (int)car.x + 15, (int)car.y + 15, 0, 0, 0, 0);
        SelectObject(hdc, hOldIndPen);
        DeleteObject(hIndicatorPen);
    }
    
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

void DrawGame(HDC hdc, HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    
    HBRUSH hBgBrush = CreateSolidBrush(RGB(34, 139, 34));
    FillRect(hdc, &rect, hBgBrush);
    DeleteObject(hBgBrush);
    
    HBRUSH hTrackBrush = CreateSolidBrush(RGB(100, 100, 100));
    HBRUSH hBorderBrush = CreateSolidBrush(RGB(200, 200, 200));
    
    for (size_t i = 0; i < g_trackPath.size(); i++) {
        float r = g_trackPath[i].width / 2 + 8;
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBorderBrush);
        Ellipse(hdc, (int)(g_trackPath[i].x - r), (int)(g_trackPath[i].y - r),
                (int)(g_trackPath[i].x + r), (int)(g_trackPath[i].y + r));
        
        r = g_trackPath[i].width / 2;
        SelectObject(hdc, hTrackBrush);
        Ellipse(hdc, (int)(g_trackPath[i].x - r), (int)(g_trackPath[i].y - r),
                (int)(g_trackPath[i].x + r), (int)(g_trackPath[i].y + r));
        
        SelectObject(hdc, hOldBrush);
    }
    
    HPEN hLinePen = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hLinePen);
    
    TrackPoint& start = g_trackPath[0];
    MoveToEx(hdc, (int)(start.x - 30), (int)(start.y - g_trackPath[0].width/2), nullptr);
    LineTo(hdc, (int)(start.x - 30), (int)(start.y + g_trackPath[0].width/2));
    
    for (int i = 0; i < 6; i++) {
        int yOffset = (int)(-g_trackPath[0].width/2) + i * 20;
        MoveToEx(hdc, (int)(start.x - 30), (int)(start.y + yOffset), nullptr);
        LineTo(hdc, (int)(start.x - 30), (int)(start.y + yOffset + 10));
    }
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hLinePen);
    
    for (const Car& car : g_cars) {
        DrawCar(hdc, car);
    }
    
    int yPos = 10;
    std::wostringstream header;
    header << L"排名   车手     圈数   状态   生命值";
    TextOutW(hdc, 10, yPos, header.str().c_str(), (int)header.str().length());
    yPos += 25;
    
    std::vector<size_t> sortedIndices;
    for (size_t i = 0; i < g_cars.size(); i++) {
        sortedIndices.push_back(i);
    }
    
    for (size_t i = 0; i < sortedIndices.size(); i++) {
        for (size_t j = i + 1; j < sortedIndices.size(); j++) {
            const Car& a = g_cars[sortedIndices[i]];
            const Car& b = g_cars[sortedIndices[j]];
            bool aFinished = a.finished;
            bool bFinished = b.finished;
            
            bool swap = false;
            if (aFinished && bFinished) {
                swap = a.finishRank > b.finishRank;
            } else if (aFinished) {
                swap = false;
            } else if (bFinished) {
                swap = true;
            } else {
                swap = a.progress < b.progress;
            }
            
            if (swap) {
                size_t temp = sortedIndices[i];
                sortedIndices[i] = sortedIndices[j];
                sortedIndices[j] = temp;
            }
        }
    }
    
    for (size_t rank = 0; rank < sortedIndices.size(); rank++) {
        size_t idx = sortedIndices[rank];
        const Car& car = g_cars[idx];
        
        std::wostringstream ss;
        ss << (rank + 1) << L"      ";
        if (car.isPlayer) {
            ss << L"玩家    ";
        } else {
            ss << L"电脑" << (idx) << L"   ";
        }
        ss << car.lap << L"/3    ";
        
        if (car.finished) {
            ss << L"完成    ";
        } else if (car.health <= 0) {
            ss << L"损坏    ";
        } else {
            ss << L"比赛中  ";
        }
        
        ss << car.health << L"%";
        
        TextOutW(hdc, 10, yPos, ss.str().c_str(), (int)ss.str().length());
        yPos += 20;
    }
    
    if (g_gameState == GAME_FINISHED) {
        HFONT hFont = CreateFontW(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                   DEFAULT_PITCH | FF_SWISS, L"Arial");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 215, 0));
        
        std::wostringstream finishMsg;
        finishMsg << L"游戏结束！";
        
        int playerRank = 0;
        for (size_t rank = 0; rank < sortedIndices.size(); rank++) {
            if (g_cars[sortedIndices[rank]].isPlayer) {
                playerRank = (int)(rank + 1);
                break;
            }
        }
        
        TextOutW(hdc, 300, 200, finishMsg.str().c_str(), (int)finishMsg.str().length());
        
        std::wostringstream rankMsg;
        rankMsg << L"你的名次: 第" << playerRank << L"名";
        TextOutW(hdc, 280, 260, rankMsg.str().c_str(), (int)rankMsg.str().length());
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }
    
    DeleteObject(hTrackBrush);
    DeleteObject(hBorderBrush);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_SETTINGS:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS_DIALOG), hWnd, SettingsDlg);
                break;
            case IDM_START:
                StartGame(hWnd);
                break;
            case IDM_EXIT:
                if (g_timerId) {
                    KillTimer(hWnd, g_timerId);
                }
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
        
    case WM_KEYDOWN:
        g_keyStates[wParam] = true;
        break;
        
    case WM_KEYUP:
        g_keyStates[wParam] = false;
        break;
        
    case WM_TIMER:
        UpdateGame();
        InvalidateRect(hWnd, nullptr, FALSE);
        break;
        
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            if (g_gameState == GAME_PLAYING || g_gameState == GAME_FINISHED) {
                DrawGame(hdc, hWnd);
            } else {
                RECT rect;
                GetClientRect(hWnd, &rect);
                HBRUSH hBrush = CreateSolidBrush(RGB(34, 139, 34));
                FillRect(hdc, &rect, hBrush);
                DeleteObject(hBrush);
                
                HFONT hFont = CreateFontW(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                           CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                           DEFAULT_PITCH | FF_SWISS, L"Arial");
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(255, 255, 255));
                
                std::wstring title = L"赛车游戏";
                TextOutW(hdc, 320, 150, title.c_str(), (int)title.length());
                
                HFONT hSmallFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                               CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                               DEFAULT_PITCH | FF_SWISS, L"Arial");
                SelectObject(hdc, hSmallFont);
                
                std::wstring instructions = L"请点击 目录->设置 配置游戏，然后点击 目录->开始 开始游戏";
                TextOutW(hdc, 150, 250, instructions.c_str(), (int)instructions.length());
                
                std::wstring instructions2 = L"玩家: 红色车辆（黄色圆圈标记），用方向键控制";
                TextOutW(hdc, 200, 300, instructions2.c_str(), (int)instructions2.length());
                
                std::wstring instructions3 = L"完成3圈即可获胜，注意不要撞到赛道边缘！";
                TextOutW(hdc, 220, 350, instructions3.c_str(), (int)instructions3.length());
                
                SelectObject(hdc, hOldFont);
                DeleteObject(hFont);
                DeleteObject(hSmallFont);
            }
            
            EndPaint(hWnd, &ps);
        }
        break;
        
    case WM_DESTROY:
        if (g_timerId) {
            KillTimer(hWnd, g_timerId);
        }
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
