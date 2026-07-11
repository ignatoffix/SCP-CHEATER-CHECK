
#include <windows.h>
#include <urlmon.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")


enum class ConsoleColor : WORD {
    Black       = 0,
    DarkBlue    = 1,
    DarkGreen   = 2,
    DarkCyan    = 3,
    DarkRed     = 4,
    DarkMagenta = 5,
    DarkYellow  = 6,
    Gray        = 7,
    DarkGray    = 8,
    Blue        = 9,
    Green       = 10,
    Cyan        = 11,
    Red         = 12,
    Magenta     = 13,
    Yellow      = 14,
    White       = 15
};


void SetColor(ConsoleColor color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, static_cast<WORD>(color));
}


void ResetColor() {
    SetColor(ConsoleColor::Gray);
}


void PrintColored(const std::string& text, ConsoleColor color) {
    SetColor(color);
    std::cout << text << std::endl;
    ResetColor();
}


void PrintColoredNoNewline(const std::string& text, ConsoleColor color) {
    SetColor(color);
    std::cout << text;
    ResetColor();
}


std::string GetDesktopPath() {
    wchar_t wpath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, wpath))) {

        int len = WideCharToMultiByte(CP_ACP, 0, wpath, -1, NULL, 0, NULL, NULL);
        std::string path(len, '\0');
        WideCharToMultiByte(CP_ACP, 0, wpath, -1, &path[0], len, NULL, NULL);
        if (!path.empty() && path.back() == '\0') path.pop_back();
        return path;
    }

    char* userProfile = getenv("USERPROFILE");
    if (userProfile) {
        return std::string(userProfile) + "\\Desktop";
    }
    return "C:\\Users\\Default\\Desktop";
}


bool DownloadFile(const std::string& url, const std::string& dest) {
    std::cout << "Скачивание " << dest << "..." << std::endl;


    int wlen = MultiByteToWideChar(CP_ACP, 0, url.c_str(), -1, NULL, 0);
    std::wstring wurl(wlen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, url.c_str(), -1, &wurl[0], wlen);

    wlen = MultiByteToWideChar(CP_ACP, 0, dest.c_str(), -1, NULL, 0);
    std::wstring wdest(wlen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, dest.c_str(), -1, &wdest[0], wlen);

    HRESULT hr = URLDownloadToFileW(NULL, wurl.c_str(), wdest.c_str(), 0, NULL);
    if (SUCCEEDED(hr)) {
        std::cout << "Успешно: " << dest << std::endl;
        return true;
    } else {
        std::cout << "Ошибка загрузки (HRESULT: 0x" << std::hex << hr << ")" << std::endl;
        return false;
    }
}


bool ExtractZip(const std::string& zipPath, const std::string& destFolder) {
    
    int wlen = MultiByteToWideChar(CP_ACP, 0, zipPath.c_str(), -1, NULL, 0);
    std::wstring wzip(wlen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, zipPath.c_str(), -1, &wzip[0], wlen);

    wlen = MultiByteToWideChar(CP_ACP, 0, destFolder.c_str(), -1, NULL, 0);
    std::wstring wdest(wlen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, destFolder.c_str(), -1, &wdest[0], wlen);

    CreateDirectoryA(destFolder.c_str(), NULL);

    std::wstring command = L"powershell.exe -Command \"Expand-Archive -Path '" + wzip + L"' -DestinationPath '" + wdest + L"'\"";

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessW(NULL, &command[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return (exitCode == 0);
    } else {
        std::cout << "Не удалось запустить PowerShell для распаковки." << std::endl;
        return false;
    }
}


void ShowSystemInfo() {
    std::cout << "Получение информации о системе..." << std::endl;

    FILE* pipe = _popen("systeminfo", "r");
    if (!pipe) {
        PrintColored("Не удалось выполнить systeminfo", ConsoleColor::Red);
        return;
    }

    std::string installDate, bootTime;
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), pipe)) {
        
        int wideLen = MultiByteToWideChar(CP_OEMCP, 0, buffer, -1, NULL, 0);
        std::wstring wline(wideLen, L'\0');
        MultiByteToWideChar(CP_OEMCP, 0, buffer, -1, &wline[0], wideLen);
        int ansiLen = WideCharToMultiByte(CP_ACP, 0, wline.c_str(), -1, NULL, 0, NULL, NULL);
        std::string line(ansiLen, '\0');
        WideCharToMultiByte(CP_ACP, 0, wline.c_str(), -1, &line[0], ansiLen, NULL, NULL);
        if (!line.empty() && line.back() == '\0') line.pop_back();

        if (line.find("Дата установки") != std::string::npos ||
            line.find("Original Install Date") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                installDate = line.substr(pos + 1);
                while (!installDate.empty() && (installDate[0] == ' ' || installDate[0] == '\t'))
                    installDate.erase(0, 1);
                while (!installDate.empty() && (installDate.back() == '\n' || installDate.back() == '\r'))
                    installDate.pop_back();
            }
        }
        if (line.find("Время загрузки системы") != std::string::npos ||
            line.find("System Boot Time") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                bootTime = line.substr(pos + 1);
                while (!bootTime.empty() && (bootTime[0] == ' ' || bootTime[0] == '\t'))
                    bootTime.erase(0, 1);
                while (!bootTime.empty() && (bootTime.back() == '\n' || bootTime.back() == '\r'))
                    bootTime.pop_back();
            }
        }
    }
    _pclose(pipe);

    if (installDate.empty() && bootTime.empty()) {
        PrintColored("Не удалось извлечь информацию о датах.", ConsoleColor::Red);
        return;
    }

    if (!installDate.empty())
        PrintColored("Дата установки Windows : " + installDate, ConsoleColor::Yellow);
    if (!bootTime.empty())
        PrintColored("Последняя загрузка системы: " + bootTime, ConsoleColor::Yellow);
}


void OpenFolders() {
    std::string appData = getenv("APPDATA");
    std::string recent = appData + "\\Microsoft\\Windows\\Recent";
    std::string prefetch = "C:\\Windows\\Prefetch";

    auto openFolder = [](const std::string& path) {
        if (!PathFileExistsA(path.c_str())) {
            PrintColored("Папка не найдена: " + path, ConsoleColor::Red);
            return;
        }
        std::cout << "Открываю " << path << "..." << std::endl;
        ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
    };

    openFolder(appData);
    openFolder(recent);
    openFolder(prefetch);
}


void DownloadAllTools() {
    std::string desktop = GetDesktopPath();
    std::string toolsDir = desktop + "\\SCP_Tools";
    CreateDirectoryA(toolsDir.c_str(), NULL);

    {
        std::string everythingExe = toolsDir + "\\Everything.exe";
        std::string primaryUrl = "https://www.voidtools.com/Everything-1.4.1.1032.x86.exe";
        bool ok = DownloadFile(primaryUrl, everythingExe);
        if (!ok) {
            std::string zipUrl = "https://www.voidtools.com/Everything-1.4.1.1032.x86.zip";
            std::string zipPath = toolsDir + "\\Everything.zip";
            PrintColored("Основная ссылка недоступна. Пробуем загрузить ZIP...", ConsoleColor::Yellow);
            if (DownloadFile(zipUrl, zipPath)) {
                if (ExtractZip(zipPath, toolsDir)) {
                    DeleteFileA(zipPath.c_str());
                    if (PathFileExistsA(everythingExe.c_str()))
                        std::cout << "Everything.exe успешно распакован." << std::endl;
                    else
                        PrintColored("Everything.exe не найден после распаковки.", ConsoleColor::Yellow);
                } else {
                    PrintColored("Ошибка распаковки Everything.zip", ConsoleColor::Red);
                }
            }
        }
    }


    DownloadFile("https://privazer.com/ru/shellbag_analyzer_cleaner.exe",
                 toolsDir + "\\ShellBag_Analyzer.exe");

    DownloadFile("https://github.com/ponei/JournalTrace/releases/download/1.0/JournalTrace.exe",
                 toolsDir + "\\JournalTrace.exe");

    {
        std::string zipPath = toolsDir + "\\LastActivityView.zip";
        std::string destFolder = toolsDir + "\\LastActivityView";
        if (DownloadFile("https://www.nirsoft.net/utils/lastactivityview.zip", zipPath)) {
            if (ExtractZip(zipPath, destFolder)) {
                DeleteFileA(zipPath.c_str());
                std::cout << "LastActivityView распакован в " << destFolder << std::endl;
            } else {
                PrintColored("Ошибка распаковки LastActivityView.zip", ConsoleColor::Red);
            }
        }
    }

    PrintColored("Все программы сохранены в " + toolsDir, ConsoleColor::Green);
}

void ShowMenu() {
    std::cout << std::endl;
    PrintColoredNoNewline("[1] ", ConsoleColor::Green);
    std::cout << "Скачать все программы для проверки" << std::endl;

    PrintColoredNoNewline("[2] ", ConsoleColor::Yellow);
    std::cout << "Информация о системе (systeminfo)" << std::endl;

    PrintColoredNoNewline("[3] ", ConsoleColor::Cyan);
    std::cout << "Открыть Recent, AppData, Prefetch" << std::endl;

    PrintColoredNoNewline("[0] ", ConsoleColor::Gray);
    std::cout << "Выход" << std::endl;
}

void DrawHeader() {
    SetColor(ConsoleColor::Cyan);
    std::cout << "+======================================+" << std::endl;
    std::cout << "|     SCP | Проверка на читы         |" << std::endl;
    std::cout << "+======================================+" << std::endl;
    ResetColor();
}

int main() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    DrawHeader();

    int choice = -1;
    while (choice != 0) {
        ShowMenu();
        std::cout << "> ";
        std::cin >> choice;

        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            PrintColored("Неверный ввод. Пожалуйста, введите число.", ConsoleColor::Red);
            continue;
        }

        switch (choice) {
        case 1:
            DownloadAllTools();
            break;
        case 2:
            ShowSystemInfo();
            break;
        case 3:
            OpenFolders();
            break;
        case 0:
            std::cout << "Выход..." << std::endl;
            break;
        default:
            PrintColored("Неверный пункт меню. Попробуйте снова.", ConsoleColor::Red);
            break;
        }
    }

    return 0;
}