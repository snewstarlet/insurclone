#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <Windows.h>
#include <string>
#include <TlHelp32.h>
#include <Psapi.h>
#include <string>
#include <thread>
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <psapi.h>
#include <time.h>
#include <math.h>
#include <string>
#include <iostream>
#include <d3d9.h>
#include <Dwmapi.h> 
#include <TlHelp32.h>  
#include <stack>
#include <vector>
#include <future>
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <future>
#include <string>
#include <mutex>


uintptr_t GetModBase(DWORD ProcID, const wchar_t* ModuleName)
{
    uintptr_t modbaseAddr = 0;
    HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, ProcID);
    if (hsnap != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hsnap, &modEntry))
        {
            do
            {
                if (!_wcsicmp(modEntry.szModule, ModuleName))
                {
                    modbaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hsnap, &modEntry));
        }
    }
    CloseHandle(hsnap);
    return modbaseAddr;
}
bool newdata;

DWORD GetProc(const wchar_t* ProcName)
{
    DWORD ProcID = 0;
    HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hsnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 ProcEntry;
        ProcEntry.dwSize = sizeof(ProcEntry);
        if (Process32First(hsnap, &ProcEntry))
        {
            do
            {
                if (!_wcsicmp(ProcEntry.szExeFile, ProcName))
                {
                    ProcID = ProcEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hsnap, &ProcEntry));
        }
    }
    CloseHandle(hsnap);
    return ProcID;
}

struct GameVars
{
    // entity & localplayer
    DWORD entitylist = 0x6D73B4;
    DWORD LocalPlayer = 0x6BD21C;

    // FOR ESP
    DWORD dwViewMatrix = 0x6C1934;
    DWORD m_dwBoneMatrix = 0xA58;
    DWORD m_vecOrigin = 0x12C;
    DWORD m_bDormant = 0xE1;

    // FOR BHOP
    DWORD dwForceJump = 0x51A81BC;
    DWORD m_fFlags = 0xf8;

    // FOR CHECKS
    DWORD m_iTeamNum = 0xE8;
    DWORD m_iHealth = 0xF4;
    DWORD m_bIsScoped = 0x3910;

    // FOR NO FLASH
    DWORD m_flFlashDuration = 0x2FFC;

    // For FOV
    DWORD m_iFOV = 0x18e0;

    DWORD nameptr = 0x148;

}game;

struct GameHandling
{
    // Necessary variables
    DWORD process;
    uintptr_t gamemodule;
    HANDLE readproc;
}handles;

struct ESP
{
    // Variables For ESP
    HBRUSH Brush;
    HFONT Font;
    HDC game_window;
    HWND game;
    COLORREF TextColor;
    COLORREF healthcolor;
    HPEN SnaplineColor;

}esp;

// Structs to store Coords
struct Vec4
{
    float x, y, z, w;
};

// Vec4 stores our clipcoords

struct Vec3
{
    float x, y, z;
};

// Vec3 stores our position

struct Vec2
{
    float x, y;
};

// Vec2 stores our screen coords

float ScreenX = GetSystemMetrics(SM_CXSCREEN); // Get Width of screen
float ScreenY = GetSystemMetrics(SM_CYSCREEN); // Get Height of screen

float DistanceBetweenCross(float X, float Y)
{
    float ydist = (Y - (ScreenY) / 2);
    float xdist = (X - (ScreenX) / 2);
    float Hypotenuse = sqrt(pow(ydist, 2) + pow(xdist, 2));
    return Hypotenuse;
}


// ESP FUNCTIONS
void DrawFilledRect(int x, int y, int w, int h)
{
    RECT rect = { x, y, x + w, y + h };
    FillRect(esp.game_window, &rect, esp.Brush);
}

void DrawBorderBox(int x, int y, int w, int h, int thickness)
{

    DrawFilledRect(x, y, w, thickness); // bottom horizontal Line

    DrawFilledRect(x, y, thickness, h); // right vertical Line

    DrawFilledRect((x + w), y, thickness, h); // left vertical Line

    DrawFilledRect(x, y + h, w + thickness, thickness); // Top horizontal line
}

void DrawLine(int x, int y, HPEN LineColor) // Draws the Snaplines
{
    MoveToEx(esp.game_window, (ScreenX / 2), (ScreenY / 2), NULL);
    LineTo(esp.game_window, x, y);
    esp.SnaplineColor = CreatePen(PS_SOLID, 1, RGB(0, 255, 255));
    SelectObject(esp.game_window, esp.SnaplineColor);
}

void lock_target(float head_x, float head_y) {
    newdata = false;

    for (int i = 0; i < 3; i++) {
        float dx = head_x;
        float dy = head_y;

        INPUT input = { 0 };

        input.mi.dx = dx;
        input.mi.dy = dy;
        input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_MOVE;
        input.type = INPUT_MOUSE;

        SendInput(1, &input, sizeof INPUT);

        Sleep(10);
        if (newdata) return;
    }
}

void DrawString(int x, int y, COLORREF color, const char* text) // Draws the text on screen
{
    SetTextAlign(esp.game_window, TA_CENTER | TA_NOUPDATECP);

    //TA_NOUPDATECP: The current position is not updated after each text output call.

    SetBkColor(esp.game_window, RGB(0, 0, 0));

    SetBkMode(esp.game_window, TRANSPARENT);

    // TRANSPARENT: Background remains untouched.

    SetTextColor(esp.game_window, color);

    SelectObject(esp.game_window, esp.Font);

    TextOutA(esp.game_window, x, y, text, strlen(text));

    DeleteObject(esp.Font);
}

bool WorldToScreen(Vec3 pos, Vec2& screen, float matrix[16], int windowWidth, int windowHeight) // converts 3D coords to 2D coords
{
    //Matrix-vector Product, multiplying world(eye) coordinates by projection matrix = clipCoords
    Vec4 clipCoords;
    clipCoords.x = pos.x * matrix[0] + pos.y * matrix[1] + pos.z * matrix[2] + matrix[3];
    clipCoords.y = pos.x * matrix[4] + pos.y * matrix[5] + pos.z * matrix[6] + matrix[7];
    clipCoords.z = pos.x * matrix[8] + pos.y * matrix[9] + pos.z * matrix[10] + matrix[11];
    clipCoords.w = pos.x * matrix[12] + pos.y * matrix[13] + pos.z * matrix[14] + matrix[15];

    if (clipCoords.w < 0.1f) // if the camera is behind our player don't draw, i think?
        return false;


    Vec3 NDC;
    // Normalize our coords
    NDC.x = clipCoords.x / clipCoords.w;
    NDC.y = clipCoords.y / clipCoords.w;
    NDC.z = clipCoords.z / clipCoords.w;

    // Convert to window coords (x,y)
    screen.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
    screen.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
    return true;
}

struct entity
{
    // Read & store All Necessary entity info

    DWORD Current_entity;
    DWORD EntityList;
    DWORD Bone;
    Vec3 entPos;
    Vec3 entBonePos;
    int Team;
    int Health;
    int Dormant;

    void ReadentityInfo(int player)
    {
        ReadProcessMemory(handles.readproc, (LPVOID)(handles.gamemodule + game.entitylist + player * 0x10), &Current_entity, sizeof(Current_entity), 0);
        ReadProcessMemory(handles.readproc, (LPVOID)(Current_entity + game.m_vecOrigin), &entPos, sizeof(entPos), 0);
        ReadProcessMemory(handles.readproc, (LPVOID)(Current_entity + game.m_dwBoneMatrix), &Bone, sizeof(Bone), 0);
        ReadProcessMemory(handles.readproc, (Vec3*)(Bone + 0x30 * 21 + 0x0C), &entBonePos.x, sizeof(entBonePos.x), 0);
        ReadProcessMemory(handles.readproc, (Vec3*)(Bone + 0x30 * 21 + 0x1C), &entBonePos.y, sizeof(entBonePos.y), 0);
        ReadProcessMemory(handles.readproc, (Vec3*)(Bone + 0x30 * 21 + 0x2C), &entBonePos.z, sizeof(entBonePos.z), 0);
        ReadProcessMemory(handles.readproc, (LPINT)(Current_entity + game.m_iTeamNum), &Team, sizeof(Team), 0);
        ReadProcessMemory(handles.readproc, (LPINT)(Current_entity + game.m_iHealth), &Health, sizeof(Health), 0);
        ReadProcessMemory(handles.readproc, (LPINT)(EntityList + game.m_bDormant), &Dormant, sizeof(Dormant), 0);
    }
}PlayerList[64];

struct me
{
    // Read & Store needed player info

    DWORD Player;
    int Team;
    int myhealth;

    void ReadMyINFO()
    {
        ReadProcessMemory(handles.readproc, (LPVOID)(handles.gamemodule + game.LocalPlayer), &Player, sizeof(Player), 0);
        ReadProcessMemory(handles.readproc, (LPINT)(Player + game.m_iTeamNum), &Team, sizeof(Team), 0);
  
    }
}Me;

void ESP()
{
    Vec2 vHead;
    Vec2 vScreen;



    float Matrix[16]; // [4][4]: 4*4 = 16
    ReadProcessMemory(handles.readproc, (PFLOAT)(handles.gamemodule + game.dwViewMatrix), &Matrix, sizeof(Matrix), 0);

    for (int i = 0; i < 64; i++) // For loop to loop through all the entities
    {
        PlayerList[i].ReadentityInfo(i);
        Me.ReadMyINFO();
        if (PlayerList[i].Current_entity != NULL)
        {
            if (PlayerList[i].Current_entity != Me.Player)
            {
                if (PlayerList->Dormant == 0) // if the players are getting updated by the server
                {
                    if (PlayerList[i].Health > 0) // if the entities are not dead
                    {
                        if (WorldToScreen(PlayerList[i].entPos, vScreen, Matrix, ScreenX, ScreenY))
                        {
                            if (WorldToScreen(PlayerList[i].entBonePos, vHead, Matrix, ScreenX, ScreenY))
                            {
                                //    ESP Box Calculations
                                float head = vHead.y - vScreen.y;
                                float width = head / 2;
                                float center = width / -2;
                                float headx = vScreen.x + center;
                                if (PlayerList[i].Team == Me.Team)
                                {
                                    esp.Brush = CreateSolidBrush(RGB(0, 0, 255));
                                    DrawBorderBox(vScreen.x + center, vScreen.y, width, head - 15, 2);

                                    esp.TextColor = RGB(0, 255, 0);
                                    char Healthchar[15];
                                    sprintf_s(Healthchar, sizeof(Healthchar), "Health: %i", (int)(PlayerList[i].Health));
                                    DrawString(vScreen.x, vScreen.y, esp.TextColor, Healthchar);
                                    DeleteObject(esp.Brush);    

                                    // Draws Team ESP
                                }
                                else
                                {


                                    esp.Brush = CreateSolidBrush(RGB(255, 0, 0));
                                    DrawBorderBox(vScreen.x + center, vScreen.y, width, head - 15 , 2);
                                    DeleteObject(esp.Brush);


                                    esp.TextColor = RGB(0, 255, 0);
                                    char Healthchar[15];
                                    sprintf_s(Healthchar, sizeof(Healthchar), "Health: %i", (int)(PlayerList[i].Health));
                                    DrawString(vScreen.x, vScreen.y, esp.TextColor, Healthchar);

                                 

                                 



                                   
                                }




                                



                            }
                        }
                    }
                }
              }
        }
    }
}

void Aim()
{
    Vec2 vHead;
    Vec2 vScreen;

    double fov = 2;
    double foveksi = -2;
    bool x1 = false;
    bool x2 = false;
    bool y1 = false;
    bool y2 = false;
    bool ok = false;

    float Matrix[16]; // [4][4]: 4*4 = 16
    ReadProcessMemory(handles.readproc, (PFLOAT)(handles.gamemodule + game.dwViewMatrix), &Matrix, sizeof(Matrix), 0);










        for (int i = 0; i < 64; i++) // For loop to loop through all the entities
        {

            PlayerList[i].ReadentityInfo(i);
            Me.ReadMyINFO();
            if (PlayerList[i].Current_entity != NULL)
            {
                if (PlayerList[i].Current_entity != Me.Player)
                {
                    if (PlayerList->Dormant == 0)
                    {
                        if (PlayerList[i].Health > 0)
                        {
                            if (WorldToScreen(PlayerList[i].entPos, vScreen, Matrix, ScreenX, ScreenY))
                            {
                                if (WorldToScreen(PlayerList[i].entBonePos, vHead, Matrix, ScreenX, ScreenY))
                                {
                                    if (PlayerList[i].Team == Me.Team)
                                    {
                                    }
                                    else
                                    {
                                        double headoffy = vHead.y - (ScreenY / 2);
                                        double headoffx = vHead.x - (ScreenX / 2);

                                        if (headoffx<= 75 && headoffx>= -75 && headoffy <= 75 && headoffy >= -75)
                                        {
                                            x1 = true;
                                        }
                                        else
                                        {
                                            x1 = false;
                                        }



                                        if (x1 == true)
                                        {
                                            std::cout << "if1 ok true" << "\n";
                                            std::cout << "entity :" << i << "\n";
                                            std::cout << "head y :" << vHead.y << "\n";
                                            std::cout << "head x :" << vHead.x << "\n";
                                            std::cout << "screen y :" << ScreenY / 2 << "\n";
                                            std::cout << "screen x :" << ScreenX / 2 << "\n";
                                            std::cout << "headoff y :" << headoffy << "\n";
                                            std::cout << "headoff x :" << headoffx << "\n";
                                            ok = true;

                                        }
                                        else
                                        {
                                            ok = false;
                                            break;

                                        }










                                    }
                                }

                            }
                        }
                    }
                }
            }
        }
    
  }







int main()
{
    bool espchk = false;
    SetConsoleTitleA("31");
    handles.process = GetProc(L"insurgency.exe");
    if (handles.process == NULL)
    {
        std::cout << "Failed to Get Process";
        getchar();
        exit(0);
    }
    handles.gamemodule = GetModBase(handles.process, L"client.dll");
    if (handles.gamemodule == NULL)
    {
        std::cout << "Failed to Get Module 'client.dll'" << std::endl;
        getchar();
        exit(0);
    }
    handles.readproc = OpenProcess(PROCESS_ALL_ACCESS, false, handles.process);
    esp.game = FindWindowA(0, "INSURGENCY");
    esp.game_window = GetDC(esp.game);

    while (handles.process != NULL)
    {
        ESP();
    }
    CloseHandle(handles.readproc);
}