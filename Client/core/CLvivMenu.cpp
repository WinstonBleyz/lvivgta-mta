/*****************************************************************************
 *
 *  PROJECT:     GTA LVIV BETA (custom build of Multi Theft Auto: San Andreas)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        core/CLvivMenu.cpp
 *  PURPOSE:     HTML main menu (CEF) drawn over the native main menu
 *
 *****************************************************************************/

#include "StdInc.h"
#include "CLvivMenu.h"
#include <core/CWebCoreInterface.h>
#include <core/CWebViewInterface.h>

#define LVIV_UI_DIR             "MTA\\cgui\\lviv\\"
#define LVIV_UI_START_URL       "http://mta/local/index.html"
#define LVIV_STATUS_INTERVAL_MS 15000

namespace
{
    SString JsonEscape(const SString& strIn)
    {
        SString strOut;
        for (unsigned char c : strIn)
        {
            switch (c)
            {
                case '"':
                    strOut += "\\\"";
                    break;
                case '\\':
                    strOut += "\\\\";
                    break;
                case '\n':
                    strOut += "\\n";
                    break;
                case '\r':
                case '\t':
                    strOut += ' ';
                    break;
                default:
                    if (c >= 0x20)
                        strOut += static_cast<char>(c);
                    break;
            }
        }
        return strOut;
    }

    // Only plain relative paths inside the UI folder are served
    bool IsSafeUiPath(const SString& strPath)
    {
        if (strPath.empty() || strPath.Contains("..") || strPath.Contains(":") || strPath[0] == '/' || strPath[0] == '\\')
            return false;
        for (unsigned char c : strPath)
            if (!(isalnum(c) || c == '/' || c == '_' || c == '-' || c == '.'))
                return false;
        return true;
    }
}

CLvivMenu::CLvivMenu()
{
}

CLvivMenu::~CLvivMenu()
{
    if (m_pWebView)
        m_pWebView->ClearWebBrowserEvents(this);

    if (m_pBrowser)
        CCore::GetSingleton().GetGUI()->DestroyElementRecursive(m_pBrowser);
    m_pBrowser = nullptr;

    if (m_pWebView)
    {
        m_pWebView->SetBeingDestroyed(true);
        if (auto pWebCore = CCore::GetSingleton().GetWebCoreUnchecked())
            pWebCore->DestroyWebView(m_pWebView);
        m_pWebView = nullptr;
    }

    SAFE_RELEASE(m_pRenderItem);
    SAFE_DELETE(m_pStatusItem);
}

bool CLvivMenu::Create()
{
    if (!FileExists(CalcMTASAPath(LVIV_UI_DIR "index.html")))
    {
        WriteDebugEvent("CLvivMenu: UI files missing, using native menu");
        return false;
    }

    CWebCoreInterface* pWebCore = CCore::GetSingleton().GetWebCore();
    if (!pWebCore)
    {
        WriteDebugEvent("CLvivMenu: CEF unavailable, using native menu");
        return false;
    }

    CGUI*     pGUI = CCore::GetSingleton().GetGUI();
    CVector2D vecResolution = pGUI->GetResolution();
    uint      uiSizeX = static_cast<uint>(vecResolution.fX);
    uint      uiSizeY = static_cast<uint>(vecResolution.fY);

    m_pRenderItem = CGraphics::GetSingleton().GetRenderItemManager()->CreateWebBrowser(uiSizeX, uiSizeY);
    if (!m_pRenderItem)
        return false;

    m_pWebView = pWebCore->CreateWebView(uiSizeX, uiSizeY, true, m_pRenderItem, false);
    if (!m_pWebView)
    {
        SAFE_RELEASE(m_pRenderItem);
        return false;
    }
    m_pWebView->SetWebBrowserEvents(this);
    m_pWebView->Initialise();

    // Full-screen element behind dialogs (settings, message boxes are always-on-top windows)
    m_pBrowser = pGUI->CreateWebBrowser(static_cast<CGUIElement*>(nullptr));
    m_pBrowser->SetPosition(CVector2D(0, 0), false);
    m_pBrowser->SetSize(vecResolution, false);
    m_pBrowser->LoadFromWebView(m_pWebView);
    m_pBrowser->SetProperty("InheritsAlpha", "False");
    m_pBrowser->SetZOrderingEnabled(false);
    m_pBrowser->SetAlwaysOnTop(true);
    m_pBrowser->MoveToBack();
    m_pBrowser->SetVisible(false);

    // The page is loaded in Events_OnCreated: CEF creates the browser asynchronously and
    // LoadURL fails until then

    in_addr address;
    address.s_addr = inet_addr(LVIV_SERVER_HOST);
    m_pStatusItem = new CServerListItem(address, LVIV_SERVER_PORT);

    WriteDebugEvent("CLvivMenu: created");
    return true;
}

void CLvivMenu::DoPulse(bool bMenuVisible, float fAlpha, bool bIngame)
{
    if (!m_pBrowser)
        return;

    m_bIngame = bIngame;
    m_pBrowser->SetVisible(bMenuVisible);
    m_pBrowser->SetAlpha(Clamp(0.f, fAlpha, 1.f));

    // The menu needs mouse only. A focused web view would receive all game keyboard input
    // (CKeyBinds forwards keys to the focused view), which also re-toggled the menu on Esc.
    // Only drop MTA's keyboard routing; CefBrowserHost::SetFocus(false) would take OS focus away from
    // the game window, and a fullscreen game minimizes when it loses focus.
    if (auto pWebCore = CCore::GetSingleton().GetWebCoreUnchecked(); pWebCore && pWebCore->GetFocusedWebView() == m_pWebView)
        pWebCore->SetFocusedWebView(nullptr);

    // Rendering is never paused: the texture is recreated on device resets (connect, alt-tab)
    // and CEF only repaints on changes, so force a repaint whenever the menu is shown again
    if (bMenuVisible != m_bWasVisible)
        WriteDebugEvent(SString("CLvivMenu: visible=%d alpha=%.2f ingame=%d ready=%d texture=%p", bMenuVisible, fAlpha, bIngame, m_bDocumentReady,
                                m_pWebView->GetTexture()));
    if (bMenuVisible && !m_bWasVisible && m_bDocumentReady)
    {
        m_pWebView->ExecuteJavascript("window.lvivNative && window.lvivNative.repaint && window.lvivNative.repaint();");
        PushState(true);
    }
    m_bWasVisible = bMenuVisible;

    // Detect a finished connect attempt that did not get us into the game
    const bool bConnecting = CCore::GetSingleton().GetConnectManager()->IsConnecting();
    const bool bConnected = CCore::GetSingleton().IsConnected() || CModManager::GetSingleton().IsLoaded();
    if (m_bWasConnecting && !bConnecting && !bConnected)
        m_bConnectFailed = true;
    if (bConnecting || bConnected)
        m_bConnectFailed = false;
    m_bWasConnecting = bConnecting;

    PulseServerStatus();
    PushState(false);
}

void CLvivMenu::PulseServerStatus()
{
    if (!m_pStatusItem)
        return;

    const uint uiNow = GetTickCount32();
    if (m_pStatusItem->bScanned || m_pStatusItem->bSkipped)
    {
        if (!m_bStatusWaiting)
        {
            m_bStatusKnown = true;
            m_bStatusOnline = m_pStatusItem->bScanned;
            m_iPlayers = m_pStatusItem->nPlayers;
            m_iMaxPlayers = m_pStatusItem->nMaxPlayers;
            m_iPing = m_pStatusItem->nPing;
            m_strServerName = m_pStatusItem->strName;
            m_uiStatusQueryTime = uiNow;
            m_bStatusWaiting = true;
        }
        else if (uiNow - m_uiStatusQueryTime >= LVIV_STATUS_INTERVAL_MS)
        {
            m_bStatusWaiting = false;
            m_pStatusItem->ResetForRefresh();
        }
        return;
    }

    m_pStatusItem->Pulse(true);
}

void CLvivMenu::ConnectToOfficialServer()
{
    if (CCore::GetSingleton().IsConnected())
        return;
    if (CCore::GetSingleton().GetConnectManager()->IsConnecting())
        return;

    m_bConnectFailed = false;
    CCommandFuncs::Connect(SString("%s %u", LVIV_SERVER_HOST, LVIV_SERVER_PORT));
}

void CLvivMenu::HandleAction(const std::string& strAction)
{
    WriteDebugEvent(SString("CLvivMenu: action %s", strAction.c_str()));
    CMainMenu* pMainMenu = CCore::GetSingleton().GetLocalGUI()->GetMainMenu();
    WatchDogUserDidInteractWithMenu();

    if (strAction == "connect" || strAction == "retry")
    {
        if (!CCore::GetSingleton().IsNetworkReady())
            pMainMenu->ShowNetworkNotReadyWindow();
        else
            ConnectToOfficialServer();
    }
    else if (strAction == "cancel")
    {
        CCore::GetSingleton().GetConnectManager()->Abort();
        CCore::GetSingleton().RemoveMessageBox(true);
        m_bConnectFailed = false;
    }
    else if (strAction == "settings")
    {
        pMainMenu->GetSettingsWindow()->SetVisible(true);
    }
    else if (strAction == "resume")
    {
        if (m_bIngame)
            pMainMenu->SetVisible(false);
    }
    else if (strAction == "disconnect")
    {
        if (m_bIngame)
            CCommands::GetSingleton().Execute("disconnect", "");
    }
    else if (strAction == "quit")
    {
        CCommands::GetSingleton().Execute("exit", "");
    }
    else if (strAction == "ready")
    {
        m_bDocumentReady = true;
    }

    PushState(true);
}

SString CLvivMenu::BuildStateJson(bool bIngame) const
{
    const bool bConnecting = CCore::GetSingleton().GetConnectManager()->IsConnecting();
    SString    strStatus = !m_bStatusKnown ? "unknown" : (m_bStatusOnline ? "online" : "offline");

    return SString(
        "{\"ingame\":%s,\"connecting\":%s,\"failed\":%s,\"networkReady\":%s,"
        "\"server\":{\"host\":\"%s\",\"port\":%u,\"status\":\"%s\",\"players\":%d,\"maxPlayers\":%d,\"ping\":%d,\"name\":\"%s\"},"
        "\"version\":\"%s\"}",
        bIngame ? "true" : "false", bConnecting ? "true" : "false", m_bConnectFailed ? "true" : "false",
        CCore::GetSingleton().IsNetworkReady() ? "true" : "false", LVIV_SERVER_HOST, LVIV_SERVER_PORT, *strStatus, m_iPlayers, m_iMaxPlayers, m_iPing,
        *JsonEscape(m_strServerName), MTA_DM_BUILDTAG_SHORT);
}

void CLvivMenu::PushState(bool bForce)
{
    if (!m_pWebView || !m_bDocumentReady)
        return;

    SString strState = BuildStateJson(m_bIngame);
    if (!bForce && strState == m_strLastState)
        return;

    m_strLastState = strState;
    m_pWebView->ExecuteJavascript("window.lvivNative && window.lvivNative.setState(" + strState + ");");
}

void CLvivMenu::Events_OnCreated()
{
    if (!m_pWebView->LoadURL(LVIV_UI_START_URL, false))
        WriteDebugEvent("CLvivMenu: LoadURL failed");
}

void CLvivMenu::Events_OnDocumentReady(const SString& strURL)
{
    m_bDocumentReady = true;
    PushState(true);
}

void CLvivMenu::Events_OnLoadingFailed(const SString& strURL, int errorCode, const SString& errorDescription)
{
    WriteDebugEvent(SString("CLvivMenu: loading failed %s (%d %s)", *strURL, errorCode, *errorDescription));
}

void CLvivMenu::Events_OnTriggerEvent(const SString& strEventName, const std::vector<std::string>& arguments)
{
    if (strEventName != "lviv" || arguments.empty())
        return;
    HandleAction(arguments[0]);
}

bool CLvivMenu::Events_OnResourcePathCheck(SString& strURL)
{
    // strURL is the part after http://mta/local/
    SString strPath = strURL.SplitLeft("?");
    if (!IsSafeUiPath(strPath))
        return false;

    strURL = CalcMTASAPath(LVIV_UI_DIR + strPath.Replace("/", "\\"));
    return FileExists(strURL);
}

bool CLvivMenu::Events_OnResourceFileCheck(const SString& strPath, CBuffer& outFileData)
{
    // strPath was produced by Events_OnResourcePathCheck, so it is inside the UI folder
    return outFileData.LoadFromFile(strPath);
}

void CLvivMenu::Events_OnConsoleMessage(const std::string& message, const std::string& source, int line, std::int16_t level)
{
    if (level >= 2)
        WriteDebugEvent(SString("CLvivMenu JS: %s (%s:%d)", message.c_str(), source.c_str(), line));
}
