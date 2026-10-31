/*****************************************************************************
 *
 *  PROJECT:     GTA LVIV BETA (custom build of Multi Theft Auto: San Andreas)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        core/CLvivMenu.h
 *  PURPOSE:     HTML main menu (CEF) drawn over the native main menu.
 *               UI files: MTA\cgui\lviv\*, JS -> native via mta.triggerEvent("lviv", action)
 *
 *****************************************************************************/

#pragma once

#include <core/CWebBrowserEventsInterface.h>

class CGUIWebBrowser;
class CWebViewInterface;
class CWebBrowserItem;
class CServerListItem;

class CLvivMenu : public CWebBrowserEventsInterface
{
public:
    CLvivMenu();
    ~CLvivMenu();

    // Returns false when CEF is unavailable; the native menu then stays in use
    bool Create();
    bool IsActive() const { return m_pBrowser != nullptr; }

    // Called every frame by CMainMenu::Update
    void DoPulse(bool bMenuVisible, float fAlpha, bool bIngame);

    // CWebBrowserEventsInterface
    void Events_OnCreated() override;
    void Events_OnLoadingStart(const SString& strURL, bool bMainFrame) override {}
    void Events_OnDocumentReady(const SString& strURL) override;
    void Events_OnLoadingFailed(const SString& strURL, int errorCode, const SString& errorDescription) override;
    void Events_OnNavigate(const SString& strURL, bool bIsBlocked, bool bIsMainFrame) override {}
    void Events_OnPopup(const SString& strTargetURL, const SString& strOpenerURL) override {}
    void Events_OnChangeCursor(unsigned char ucCursor) override {}
    void Events_OnTriggerEvent(const SString& strEventName, const std::vector<std::string>& arguments) override;
    void Events_OnTooltip(const SString& strTooltip) override {}
    void Events_OnInputFocusChanged(bool bGainedFocus) override {}
    bool Events_OnResourcePathCheck(SString& strURL) override;
    bool Events_OnResourceFileCheck(const SString& strURL, CBuffer& outFileData) override;
    void Events_OnResourceBlocked(const SString& strURL, const SString& strDomain, unsigned char reason) override {}
    void Events_OnAjaxRequest(CAjaxResourceHandlerInterface* pHandler, const SString& strURL) override {}
    void Events_OnConsoleMessage(const std::string& message, const std::string& source, int line, std::int16_t level) override;

private:
    void    HandleAction(const std::string& strAction);
    void    ConnectToOfficialServer();
    void    PulseServerStatus();
    void    PushState(bool bForce);
    SString BuildStateJson(bool bIngame) const;

    CWebBrowserItem*   m_pRenderItem = nullptr;
    CWebViewInterface* m_pWebView = nullptr;
    CGUIWebBrowser*    m_pBrowser = nullptr;
    bool               m_bDocumentReady = false;
    bool               m_bIngame = false;
    bool               m_bWasVisible = false;

    // Connection tracking for the "Повторити підключення" button
    bool    m_bWasConnecting = false;
    bool    m_bConnectFailed = false;
    SString m_strLastState;

    // Server status (MTA ASE query; query port is game port + 123)
    CServerListItem* m_pStatusItem = nullptr;
    uint             m_uiStatusQueryTime = 0;
    bool             m_bStatusWaiting = false;
    bool             m_bStatusKnown = false;
    bool             m_bStatusOnline = false;
    int              m_iPlayers = 0;
    int              m_iMaxPlayers = 0;
    int              m_iPing = 0;
    SString          m_strServerName;
};
