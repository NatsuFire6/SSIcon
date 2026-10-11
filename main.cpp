#include <windows.h>
#include <string>

// ============================================================
// CONFIGURATION
// ============================================================

constexpr int HOTKEY_ID = 1;

constexpr UINT HOTKEY_MODIFIERS = MOD_CONTROL | MOD_ALT;
constexpr UINT HOTKEY_KEY = 'D';

constexpr wchar_t MUTEX_NAME[] =
    L"ToggleDesktopIcons_SingleInstance_Mutex";

constexpr wchar_t WINDOW_CLASS_NAME[] =
    L"ToggleDesktopIconsWindow";

constexpr wchar_t TOGGLE_MESSAGE_NAME[] =
    L"ToggleDesktopIcons_ToggleMessage";


// ============================================================
// RECHERCHE DES ICÔNES DU BUREAU
// ============================================================

HWND FindDesktopIcons()
{
    // Première méthode : Progman
    HWND progman = FindWindowW(
        L"Progman",
        nullptr
    );

    if (progman)
    {
        HWND shellView = FindWindowExW(
            progman,
            nullptr,
            L"SHELLDLL_DefView",
            nullptr
        );

        if (shellView)
            return shellView;
    }


    // Deuxième méthode : WorkerW
    HWND workerW = nullptr;

    while ((workerW = FindWindowExW(
        nullptr,
        workerW,
        L"WorkerW",
        nullptr)))
    {
        HWND shellView = FindWindowExW(
            workerW,
            nullptr,
            L"SHELLDLL_DefView",
            nullptr
        );

        if (shellView)
            return shellView;
    }

    return nullptr;
}


// ============================================================
// BASCULER L'AFFICHAGE DES ICÔNES
// ============================================================

void ToggleDesktopIcons()
{
    HWND desktopIcons = FindDesktopIcons();

    if (!desktopIcons)
        return;

    SendMessageW(
        desktopIcons,
        WM_COMMAND,
        0x7402,
        0
    );
}


// ============================================================
// MESSAGE DE BASCULEMENT
// ============================================================

UINT GetToggleMessage()
{
    static UINT message = RegisterWindowMessageW(
        TOGGLE_MESSAGE_NAME
    );

    return message;
}


// ============================================================
// PROCÉDURE DE FENÊTRE
// ============================================================

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    // --------------------------------------------------------
    // Raccourci clavier
    // --------------------------------------------------------

    if (message == WM_HOTKEY)
    {
        if (wParam == HOTKEY_ID)
        {
            ToggleDesktopIcons();
        }

        return 0;
    }


    // --------------------------------------------------------
    // Demande de basculement provenant d'une autre instance
    // --------------------------------------------------------

    if (message == GetToggleMessage())
    {
        ToggleDesktopIcons();

        return 0;
    }


    // --------------------------------------------------------
    // Fermeture
    // --------------------------------------------------------

    if (message == WM_DESTROY)
    {
        UnregisterHotKey(
            hwnd,
            HOTKEY_ID
        );

        PostQuitMessage(0);

        return 0;
    }


    return DefWindowProcW(
        hwnd,
        message,
        wParam,
        lParam
    );
}


// ============================================================
// VÉRIFIE SI --startup EST PRÉSENT
// ============================================================

bool IsStartupMode()
{
    int argc = 0;

    LPWSTR* argv = CommandLineToArgvW(
        GetCommandLineW(),
        &argc
    );

    if (!argv)
        return false;

    bool startup = false;

    for (int i = 1; i < argc; ++i)
    {
        if (std::wstring(argv[i]) == L"--startup")
        {
            startup = true;
            break;
        }
    }

    LocalFree(argv);

    return startup;
}


// ============================================================
// POINT D'ENTRÉE
// ============================================================

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int)
{
    const bool startupMode = IsStartupMode();


    // ========================================================
    // 1. CRÉATION DU MUTEX
    // ========================================================

    HANDLE mutex = CreateMutexW(
        nullptr,
        TRUE,
        MUTEX_NAME
    );

    if (!mutex)
        return 1;


    // ========================================================
    // 2. VÉRIFIER SI UNE INSTANCE EXISTE DÉJÀ
    // ========================================================

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Si c'est un lancement automatique avec Windows,
        // on ne fait absolument rien.
        if (startupMode)
        {
            CloseHandle(mutex);
            return 0;
        }


        // Sinon, il s'agit d'un double-clic normal.
        // On demande à l'instance existante de basculer
        // les icônes.

        HWND existingWindow = FindWindowW(
            WINDOW_CLASS_NAME,
            nullptr
        );

        if (existingWindow)
        {
            PostMessageW(
                existingWindow,
                GetToggleMessage(),
                0,
                0
            );
        }

        CloseHandle(mutex);

        return 0;
    }


    // ========================================================
    // 3. CRÉATION DE LA CLASSE DE FENÊTRE
    // ========================================================

    WNDCLASSW windowClass{};

    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassW(&windowClass))
    {
        CloseHandle(mutex);
        return 1;
    }


    // ========================================================
    // 4. CRÉATION D'UNE FENÊTRE INVISIBLE
    // ========================================================

    HWND hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        L"Toggle Desktop Icons",
        0,
        0,
        0,
        0,
        0,
        HWND_MESSAGE,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd)
    {
        CloseHandle(mutex);
        return 1;
    }


    // ========================================================
    // 5. ENREGISTREMENT DU RACCOURCI
    // ========================================================

    if (!RegisterHotKey(
        hwnd,
        HOTKEY_ID,
        HOTKEY_MODIFIERS,
        HOTKEY_KEY))
    {
        DestroyWindow(hwnd);
        CloseHandle(mutex);

        return 1;
    }


    // ========================================================
    // 6. DOUBLE-CLIC NORMAL
    // ========================================================

    // Si le programme n'a PAS été lancé avec --startup,
    // alors il s'agit d'un lancement manuel.
    //
    // On bascule donc les icônes.

    if (!startupMode)
    {
        ToggleDesktopIcons();
    }


    // ========================================================
    // 7. BOUCLE PRINCIPALE
    // ========================================================

    MSG msg{};

    while (GetMessageW(
        &msg,
        nullptr,
        0,
        0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }


    // ========================================================
    // 8. NETTOYAGE
    // ========================================================

    CloseHandle(mutex);

    return 0;
}