//
// bidtow - bind input device to one window
//
// bidtow  Copyright (C) 2009  Yuki Mitsui <silphire@gmail.com>
// This program comes with ABSOLUTELY NO WARRANTY.
// This is free software, and you are welcome to redistribute it
// under certain conditions; read LICENSE for details.
//

//
// REFERENCES:
// http://www.codeproject.com/KB/system/rawinput.aspx
//

#include <windows.h>
#include <tchar.h>
#include <vector>
#include "resource.h"

#include "InputDeviceManager.h"
#include "InputDevice.h"

#define NELEMS(x) (sizeof(x) / sizeof(x[0]))

#define WM_NOTIFYREGION (WM_APP + 1)
#define MSG_WM_NOTIFYREGION(func) \
	if(uMsg == WM_NOTIFYREGION) { \
		SetMsgHandled(TRUE); \
		lResult = 0; \
		func(wParam, lParam); \
		if(IsMsgHandled()) \
			return TRUE; \
	}

#define SYSCOMMAND_ID_HANDLER_EX(id, func) \
	if (uMsg == WM_SYSCOMMAND && id == LOWORD(wParam)) \
	{ \
		SetMsgHandled(TRUE); \
		func((UINT)HIWORD(wParam), (int)LOWORD(wParam), (HWND)lParam); \
		lResult = 0; \
		if(IsMsgHandled()) \
			return TRUE; \
	}

CAppModule _Module;

class CMainDialog : public CDialogImpl<CMainDialog> {
public:
	enum { IDD = IDD_MAINDIALOG };

	BEGIN_MSG_MAP_EX(CMainDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_CLOSE(OnClose)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_INPUT(OnInput)
		MSG_WM_NOTIFYREGION(OnNotifyRegion)
		MESSAGE_HANDLER_EX(TaskbarRestartMessage, OnTaskbarRestart)
		COMMAND_ID_HANDLER_EX(IDOK, OnOK)
		COMMAND_ID_HANDLER_EX(ID_BIND, OnBind)
		COMMAND_ID_HANDLER_EX(ID_CONFIG, OnConfig)
		COMMAND_ID_HANDLER_EX(ID_STATUS, OnStatus)
		COMMAND_ID_HANDLER_EX(ID_ABOUT, OnAbout)
		COMMAND_ID_HANDLER_EX(ID_EXIT, OnExit)
		SYSCOMMAND_ID_HANDLER_EX(SC_CLOSE, OnSysClose)
	END_MSG_MAP()

	CMainDialog(void);
	virtual ~CMainDialog();
	BOOL ShowBidtowWindow(void);
	BOOL HideBidtowWindow(void);

private:
	static const UINT IconID;		// task tray icon's ID

	UINT TaskbarRestartMessage;
	CMenu TrayMenu;

	BOOL ManipulateIconOnTaskbar(DWORD dwMessage);
	BOOL RegisterIconOnTaskbar(DWORD dwMessage, NOTIFYICONDATA *notifyIcon);
	BOOL AddIconToTaskbar(void);
	BOOL RemoveIconFromTaskbar(void);
	BOOL ShowBalloon(const TCHAR *msg, const TCHAR *title);

protected:
	LRESULT OnInitDialog(HWND hWnd, LPARAM lParam);
	void OnClose(void);
	void OnDestroy(void);
	void OnInput(WPARAM code, HRAWINPUT hRawInput);
	void OnNotifyRegion(WPARAM wParam, LPARAM lParam);
	LRESULT OnTaskbarRestart(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnOK(UINT uNotifyCode, int nID, HWND hWndCtrl);
	LRESULT OnBind(UINT uNotifyCode, int nID, HWND hWndCtrl);
	LRESULT OnConfig(UINT uNotifyCode, int nID, HWND hWndCtrl);
	LRESULT OnStatus(UINT uNotifyCode, int nID, HWND hWndCtrl);
	LRESULT OnAbout(UINT uNotifyCode, int nID, HWND hWndCtrl);
	LRESULT OnExit(UINT uNotifyCode, int nID, HWND hWndCtrl);
	LRESULT OnSysClose(UINT uNotifyCode, int nID, HWND hWndCtrl);
};

const UINT CMainDialog::IconID = 1;

const TCHAR *appClassName = _T("bidtow");
static InputDeviceManager theManager;

CMainDialog::CMainDialog(void)
{
}

CMainDialog::~CMainDialog()
{
}


BOOL CMainDialog::ManipulateIconOnTaskbar(DWORD dwMessage)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	NOTIFYICONDATA notifyIcon;

	// append icon to notification region
	ZeroMemory(&notifyIcon, sizeof(notifyIcon));
	notifyIcon.cbSize = NOTIFYICONDATA_V2_SIZE;
	notifyIcon.hWnd = this->m_hWnd;
	notifyIcon.uID = CMainDialog::IconID;
	notifyIcon.uVersion = NOTIFYICON_VERSION;
	notifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIcon.uCallbackMessage = WM_NOTIFYREGION;
	notifyIcon.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_BIDTOW));
	_tcscpy_s(notifyIcon.szTip, NELEMS(notifyIcon.szTip), appClassName);

	return RegisterIconOnTaskbar(dwMessage, &notifyIcon);
}

BOOL CMainDialog::RegisterIconOnTaskbar(DWORD dwMessage, NOTIFYICONDATA *notifyIcon)
{
	while(!Shell_NotifyIcon(dwMessage, notifyIcon)) {
		// Shell_NotifyIcon() can be timeout, so we should check icon has registered certainly.
		// see also: http://support.microsoft.com/kb/418138/
		DWORD err = GetLastError();
		if(err == ERROR_TIMEOUT) {
			Sleep(100);
			if(Shell_NotifyIcon(NIM_MODIFY, notifyIcon))
				break;
		} else {
			// something error occured, but not processed specially.
			return err == ERROR_SUCCESS;
		}
	}

	return TRUE;
}

BOOL CMainDialog::AddIconToTaskbar(void)
{
	BOOL ret = TRUE;
	ret = ret && ManipulateIconOnTaskbar(NIM_SETVERSION);
	ret = ret && ManipulateIconOnTaskbar(NIM_ADD);
	return ret;
}

BOOL CMainDialog::RemoveIconFromTaskbar(void)
{
	return ManipulateIconOnTaskbar(NIM_DELETE);
}

BOOL CMainDialog::ShowBalloon(const TCHAR *msg, const TCHAR *title)
{
	NOTIFYICONDATA notifyIcon;

	::ZeroMemory(&notifyIcon, sizeof(notifyIcon));
	notifyIcon.cbSize = NOTIFYICONDATA_V2_SIZE;
	notifyIcon.hWnd = this->m_hWnd;
	notifyIcon.uID = CMainDialog::IconID;
	notifyIcon.uFlags = NIF_INFO;
	notifyIcon.dwInfoFlags = NIIF_INFO;
	_tcscpy_s(notifyIcon.szInfo, NELEMS(notifyIcon.szInfo), msg);
	_tcscpy_s(notifyIcon.szInfoTitle, NELEMS(notifyIcon.szInfoTitle), title);
	return RegisterIconOnTaskbar(NIM_MODIFY, &notifyIcon);
}

//
//
//
BOOL CMainDialog::ShowBidtowWindow(void)
{
	ShowWindow(SW_SHOW);
	return TRUE;
}

//
//
//
BOOL CMainDialog::HideBidtowWindow(void)
{
	ShowWindow(SW_HIDE);
	return TRUE;
}

//
//
//
void GetCurrentAvailableInputDevicesName()
{
	RAWINPUTDEVICELIST *devices;
	UINT i, nDevice;

	GetRawInputDeviceList(NULL, &nDevice, sizeof(RAWINPUTDEVICELIST));
	devices = new RAWINPUTDEVICELIST[nDevice];
	GetRawInputDeviceList(devices, &nDevice, sizeof(RAWINPUTDEVICELIST));

	for(i = 0; i < nDevice; ++i) {
		if(devices[i].dwType == RIM_TYPEKEYBOARD || devices[i].dwType == RIM_TYPEMOUSE) {
			InputDevice *devobj = new InputDevice();
		}
	}
}

LRESULT CMainDialog::OnInitDialog(HWND hWnd, LPARAM lParam)
{
	CIcon appIcon;

	appIcon.LoadIcon(IDI_ICON_BIDTOW);
	this->SetIcon(appIcon);
	if(!AddIconToTaskbar())
		return FALSE;

	TaskbarRestartMessage = RegisterWindowMessage(TEXT("TaskbarCreated"));

	return TRUE;
}


void CMainDialog::OnClose(void)
{
	RemoveIconFromTaskbar();
	DestroyWindow();
	PostQuitMessage(0);
}

void CMainDialog::OnDestroy(void)
{
	RemoveIconFromTaskbar();
	PostQuitMessage(0);
}

void CMainDialog::OnInput(WPARAM code, HRAWINPUT hRawInput)
{
	theManager.PassInputMessage(code, hRawInput);
}

void CMainDialog::OnNotifyRegion(WPARAM wParam, LPARAM lParam)
{
	if(lParam == WM_LBUTTONDBLCLK) {
		ShowBidtowWindow();
	} else if(lParam == WM_RBUTTONUP) {
		// show context menu
		UINT flag = 0;
		CMenu TrayMenu;
		POINT pt;

		GetCursorPos(&pt);
		flag |= (pt.x < GetSystemMetrics(SM_CXSCREEN) / 2) ? TPM_LEFTALIGN : TPM_RIGHTALIGN;
		flag |= (pt.y < GetSystemMetrics(SM_CYSCREEN) / 2) ? TPM_TOPALIGN : TPM_BOTTOMALIGN;

		TrayMenu.LoadMenu(IDR_MENU_TASKTRAY);
		TrayMenu.GetSubMenu(0).TrackPopupMenu(flag, pt.x, pt.y, this->m_hWnd);
	}
}

LRESULT CMainDialog::OnTaskbarRestart(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// taskbar recreated
	// see also: http://www31.ocn.ne.jp/~yoshio2/vcmemo17-1.html
	AddIconToTaskbar();
	return TRUE;
}

LRESULT CMainDialog::OnOK(UINT uNotifyCode, int nID, HWND hWndCtrl)
{
	return TRUE;
}

//
// start binding window with input device
//
LRESULT CMainDialog::OnBind(UINT uNotifyCode, int nID, HWND hWndCtrl)
{
	theManager.StartBind();
	return TRUE;
}

LRESULT CMainDialog::OnConfig(UINT uNotifyCode, int nID, HWND hWndCtrl)
{
	return TRUE;
}

LRESULT CMainDialog::OnStatus(UINT uNotifyCode, int nID, HWND hWndCtrl)
{
	return TRUE;
}

LRESULT CMainDialog::OnAbout(UINT uNotifyCode, int nID, HWND hWndCtrl)
{
	return TRUE;
}

//
// "Exit" on the tasktray menu
//
LRESULT CMainDialog::OnExit(UINT uNotifyCode, int nID, HWND hWndCtrl)
{
	RemoveIconFromTaskbar();
	DestroyWindow();
	PostQuitMessage(0);
	return TRUE;
}

LRESULT CMainDialog::OnSysClose(UINT uNotifyCode, int nID, HWND hWndCtrl)
{
	HideBidtowWindow();
	return TRUE;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nShowCmd)
{
	RAWINPUTDEVICE devs[2];
	CMainDialog dlg;
	CMessageLoop theLoop;
	BOOL bResult;

	_Module.Init(NULL, hInstance);

	dlg.Create(NULL);

	// hook keyboard and mouse control
	ZeroMemory(devs, sizeof(devs));
	devs[0].hwndTarget	= devs[1].hwndTarget	= dlg.m_hWnd;
	devs[0].usUsagePage	= devs[1].usUsagePage	= 0x01;
	devs[0].usUsage		= 0x02;	// Usage Page=1, Usage ID=2 -> mouse
	devs[1].usUsage		= 0x06;	// Usage Page=1, Usage ID=6 -> keyboard
	devs[0].dwFlags		= devs[1].dwFlags		= RIDEV_INPUTSINK;
	bResult = RegisterRawInputDevices(devs, 2, sizeof(RAWINPUTDEVICE));
	if(!bResult) {
		return 0;
	}

	dlg.ShowBidtowWindow();

	int ret = theLoop.Run();

	_Module.RemoveMessageLoop();
	_Module.Term();
	return ret;
}
