#include <vcl.h>
#include <windows.h>
#pragma hdrstop
#pragma argsused
#include <PluginAPI.h>
#include "SettingsFrm.h"
#include <process.h>
#include <inifiles.hpp>
#include <IdHashMessageDigest.hpp>
#define ALPHAWINDOWS_OLDPROC L"AlphaWindows/OldProc"
#define ALPHAWINDOWS_SETTRANSPARENCY L"AlphaWindows/SetTransparency"
#define TABKIT_OLDPROC L"TabKit/OldProc"

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
  return 1;
}
//---------------------------------------------------------------------------

//Uchwyt-do-formy-ustawien---------------------------------------------------
TSettingsForm *hSettingsForm;
//Struktury-glowne-----------------------------------------------------------
TPluginLink PluginLink;
TPluginInfo PluginInfo;
PPluginWindowEvent WindowEvent;
//---------------------------------------------------------------------------
//Tablica z uchwytem i stara procka okna
struct TWinProcTable
{
  HWND hwnd;
  WNDPROC OldProc;
  WNDPROC CurrentProc;
};
TWinProcTable WinProcTable[30];
//PID procesu
DWORD ProcessPID;
//Uchwyt do aplikacji
HINSTANCE hAppHandle;
//Uchwyt do okna kontaktow
HWND hFrmMain;
//Uchwyt do okna rozmowy
HWND hFrmSend;
//Uchwyt do okna z chmurka informacyjna
HWND hFrmMiniStatus;
//Uchwyr do okna odtwarzacza YouTube
HWND hFrmYTMovie;
//Uchwyt do okna timera
HWND hTimerFrm;
//Uchwyt do ostatnio aktywnego okna
HWND hLastActiveFrm;
//Zdefiniowana przezroczystosc
int AlphaValue;
//Informacja o widocznym oknie instalowania dodatku
bool FrmInstallAddonExist = false;
//Gdy zostalo uruchomione wyladowanie wtyczki wraz z zamknieciem komunikatora
bool ForceUnloadExecuted = false;
//TIMERY---------------------------------------------------------------------
#define TIMER_CHKACTIVEWINDOW 10
//FORWARD-TIMER--------------------------------------------------------------
LRESULT CALLBACK TimerFrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//FORWARD-WINDOW-PROC--------------------------------------------------------
LRESULT CALLBACK FrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//FORWARD-AQQ-HOOKS----------------------------------------------------------
int __stdcall OnBeforeUnload(WPARAM wParam, LPARAM lParam);
int __stdcall OnPrimaryTab(WPARAM wParam, LPARAM lParam);
int __stdcall OnRecvOldProc(WPARAM wParam, LPARAM lParam);
int __stdcall OnThemeChanged(WPARAM wParam, LPARAM lPaam);
int __stdcall OnWindowEvent(WPARAM wParam, LPARAM lParam);
//FORWARD-OTHER-FUNCTION-----------------------------------------------------
void LoadSettings();
//---------------------------------------------------------------------------

//Pobieranie sciezki katalogu prywatnego wtyczek
UnicodeString GetPluginUserDir()
{
  return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETPLUGINUSERDIR,0,0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------
UnicodeString GetPluginUserDirW()
{
  return (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETPLUGINUSERDIR,0,0);
}
//---------------------------------------------------------------------------

//Pobieranie sciezki do kompozycji
UnicodeString GetThemeDir()
{
  return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETTHEMEDIR,0,0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------

//Pobieranie sciezki do skorki kompozycji
UnicodeString GetThemeSkinDir()
{
  return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETTHEMEDIR,0,0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll) + "\\\\Skin";
}
//---------------------------------------------------------------------------

//Sprawdzanie czy  wlaczona jest zaawansowana stylizacja okien
bool ChkSkinEnabled()
{
  TStrings* IniList = new TStringList();
  IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP,0,0));
  TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
  Settings->SetStrings(IniList);
  delete IniList;
  UnicodeString SkinsEnabled = Settings->ReadString("Settings","UseSkin","1");
  delete Settings;
  return StrToBool(SkinsEnabled);
}
//---------------------------------------------------------------------------

//Sprawdzanie ustawien animacji AlphaControls
bool ChkThemeAnimateWindows()
{
  TStrings* IniList = new TStringList();
  IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP,0,0));
  TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
  Settings->SetStrings(IniList);
  delete IniList;
  UnicodeString AnimateWindowsEnabled = Settings->ReadString("Theme","ThemeAnimateWindows","1");
  delete Settings;
  return StrToBool(AnimateWindowsEnabled);
}
//---------------------------------------------------------------------------
bool ChkThemeGlowing()
{
  TStrings* IniList = new TStringList();
  IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP,0,0));
  TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
  Settings->SetStrings(IniList);
  delete IniList;
  UnicodeString GlowingEnabled = Settings->ReadString("Theme","ThemeGlowing","1");
  delete Settings;
  return StrToBool(GlowingEnabled);
}
//---------------------------------------------------------------------------

//Szukanie uchwytu do okna kontaktow
bool CALLBACK FindFrmMain(HWND hwnd, LPARAM)
{
  //Pobranie klasy okna
  wchar_t WClassName[128];
  GetClassNameW(hwnd, WClassName, sizeof(WClassName));
  //Sprawdenie klasy okna
  if((UnicodeString)WClassName=="TfrmMain")
  {
	//Pobranie PID okna
	DWORD PID;
	GetWindowThreadProcessId(hwnd, &PID);
	//Porownanie PID okna
	if(PID==ProcessPID)
	{
	  //Przypisanie uchwytu
	  hFrmMain = hwnd;
	  //Pobranie uchwytu do aplikacji
	  hAppHandle = (HINSTANCE)GetWindowLong(hFrmMain,GWLP_HINSTANCE);
	  return false;
	}
  }
  return true;
}
//---------------------------------------------------------------------------

//Pobieranie indeksu wolnego rekordu
int ReciveFreeIdx()
{
  for(int Count=0;Count<30;Count++)
  {
	if(!WinProcTable[Count].hwnd)
	 return Count;
  }
  return -1;
}
//--------------------------------------------------------------------------

//Pobieranie starej procki na podstawie uchwytu okna
WNDPROC ReciveOldProc(HWND hwnd)
{
  for(int Count=0;Count<30;Count++)
  {
	if(WinProcTable[Count].hwnd==hwnd)
	 return WinProcTable[Count].OldProc;
  }
  return NULL;
}
//---------------------------------------------------------------------------

//Pobieranie akualnej procki na podstawie uchwytu okna
WNDPROC ReciveCurrentProc(HWND hwnd)
{
  for(int Count=0;Count<30;Count++)
  {
	if(WinProcTable[Count].hwnd==hwnd)
	 return WinProcTable[Count].CurrentProc;
  }
  return NULL;
}
//---------------------------------------------------------------------------

//Pobieranie indeksu rekordu na podstawie uchwytu okna
int ReciveIdx(HWND hwnd)
{
  for(int Count=0;Count<30;Count++)
  {
	if(WinProcTable[Count].hwnd==hwnd)
	 return Count;
  }
  return -1;
}
//---------------------------------------------------------------------------

//Ustawianie przezroczystosci dla danego okna
void SetAlphaWnd(HWND hwnd)
{
  /*if(hwnd!=hFrmYTMovie)
  {
	long exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	if(!(exStyle & WS_EX_LAYERED)) SetWindowLong(hwnd, GWL_EXSTYLE, exStyle|WS_EX_LAYERED);
	Vcl::Forms::SetLayeredWindowAttributes((int)hwnd, 0, AlphaValue, LWA_ALPHA);
  }*/
  //Ustawienie przezroczystosci
  PluginLink.CallService(AQQ_WINDOW_TRANSPARENT,(WPARAM)hwnd,(LPARAM)AlphaValue);
  //Wyslanie informacji o nadaniu przezroczystosci
  TPluginHook PluginHook;
  PluginHook.HookName = ALPHAWINDOWS_SETTRANSPARENCY;
  PluginHook.wParam = (WPARAM)AlphaValue;
  PluginHook.lParam = (LPARAM)hwnd;
  PluginLink.CallService(AQQ_SYSTEM_SENDHOOK,(WPARAM)(&PluginHook),0);
}
//---------------------------------------------------------------------------

//Ustawienie przezroczystosci dla wszystkich okien
void SetAlpha()
{
  //Przejrzenie calej tablicy
  for(int Count=0;Count<30;Count++)
  {
	//Rekord zawiera uchwyt
	if(WinProcTable[Count].hwnd)
	 SetAlphaWnd(WinProcTable[Count].hwnd);
  }
}
//---------------------------------------------------------------------------

//Ustawienie przezroczystosci dla wszystkich okien z podaniem wartosci
void SetAlphaEx(int Value)
{
  //Przejrzenie calej tablicy
  for(int Count=0;Count<30;Count++)
  {
	//Rekord zawiera uchwyt
	if(WinProcTable[Count].hwnd)
	{
	  //Okno nie jest chmurka informacyjna
	  if(WinProcTable[Count].hwnd!=hFrmMiniStatus)
	  {
		/*if(WinProcTable[Count].hwnd!=hFrmYTMovie)
		{
		  long exStyle = GetWindowLong(WinProcTable[Count].hwnd, GWL_EXSTYLE);
		  if(!(exStyle & WS_EX_LAYERED)) SetWindowLong(WinProcTable[Count].hwnd, GWL_EXSTYLE, exStyle|WS_EX_LAYERED);
		  Vcl::Forms::SetLayeredWindowAttributes((int)WinProcTable[Count].hwnd, 0, Value, LWA_ALPHA);
		}*/
		//Ustawienie przezroczystosci
		PluginLink.CallService(AQQ_WINDOW_TRANSPARENT,(WPARAM)WinProcTable[Count].hwnd,(LPARAM)Value);
		//Wyslanie informacji o nadaniu przezroczystosci
		TPluginHook PluginHook;
		PluginHook.HookName = ALPHAWINDOWS_SETTRANSPARENCY;
		PluginHook.wParam = (WPARAM)Value;
		PluginHook.lParam = (LPARAM)WinProcTable[Count].hwnd;
		PluginLink.CallService(AQQ_SYSTEM_SENDHOOK,(WPARAM)(&PluginHook),0);
	  }
	}
  }
}
//---------------------------------------------------------------------------

//Ustawianie przezroczystosci dla danego okna
void RemoveAlphaWnd(HWND hwnd)
{
  //if(hwnd!=hFrmYTMovie) Vcl::Forms::SetLayeredWindowAttributes((int)hwnd, 0, 255, LWA_ALPHA);
  //Ustawienie przezroczystosci
  PluginLink.CallService(AQQ_WINDOW_TRANSPARENT,(WPARAM)hwnd,255);
  //Wyslanie informacji o nadaniu przezroczystosci
  TPluginHook PluginHook;
  PluginHook.HookName = ALPHAWINDOWS_SETTRANSPARENCY;
  PluginHook.wParam = (WPARAM)255;
  PluginHook.lParam = (LPARAM)hwnd;
  PluginLink.CallService(AQQ_SYSTEM_SENDHOOK,(WPARAM)(&PluginHook),0);
}
//---------------------------------------------------------------------------

//Procka okna timera
LRESULT CALLBACK TimerFrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if((uMsg==WM_TIMER)&&(!ForceUnloadExecuted))
  {
	//Sprawdzanie aktywnego okna
	if(wParam==TIMER_CHKACTIVEWINDOW)
	{
	  //Pobieranie aktywnego okna
	  HWND hActiveFrm = GetForegroundWindow();
	  //Aktywne okno zostalo zmienione
	  if((hActiveFrm!=hLastActiveFrm)&&(hActiveFrm!=hFrmMiniStatus))
	  {
		//Zabezpieczenie przed bialym tlem po zamknieciu okna
		if((IsWindowVisible(hLastActiveFrm))||(!hLastActiveFrm))
		{
		  //Pobranie PID aktywnego okna
		  DWORD PID;
		  GetWindowThreadProcessId(hActiveFrm, &PID);
		  //Okno komunikatora ale nie okno wtyczki
		  if(PID==ProcessPID)
		  {
			//Ustawianie przezroczystysci okna
			SetAlphaWnd(hActiveFrm);
			//Jezeli procka dla okna nie istnieje i nie jest to okno wtyczki
			if((!ReciveOldProc(hActiveFrm))&&(hAppHandle==(HINSTANCE)GetWindowLong(hActiveFrm,GWLP_HINSTANCE)))
			{
			  //Pobierane wolnego rekordu
			  int Idx = ReciveFreeIdx();
			  //Zapisanie uchwytu do okna
			  WinProcTable[Idx].hwnd = hActiveFrm;
			  //Zmiana procki okna + zapisanie starej procki
			  WinProcTable[Idx].OldProc = (WNDPROC)SetWindowLongPtrW(hActiveFrm, GWLP_WNDPROC,(LONG)FrmProc);
			  //Zapisanie aktualnej procki
			  WinProcTable[Idx].CurrentProc = (WNDPROC)GetWindowLongPtr(hActiveFrm, GWLP_WNDPROC);
			}
		  }
		}
		//Zapamietywanie ostatnio aktywnego okna
		hLastActiveFrm = hActiveFrm;
	  }
	}

	return 0;
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------


LRESULT CALLBACK FrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if(!ForceUnloadExecuted)
  {
	//Ustawienie przezroczystosci dla chmurki informacyjnej
	if(hwnd==hFrmMiniStatus)
	{
	  if(uMsg==WM_NCPAINT) SetAlphaWnd(hwnd);
	}
	//Ustawienie przezroczystosci dla pozostalych okien
	else
	{
	  if((uMsg==WM_ACTIVATE)&&((wParam==WA_ACTIVE)||(wParam==WA_CLICKACTIVE))) SetAlphaWnd(hwnd);
	  if((uMsg==WM_ACTIVATE)&&(wParam==WA_INACTIVE)) SetAlphaWnd(hwnd);
	  if(uMsg==WM_ACTIVATEAPP) SetAlphaWnd(hwnd);
	  if(uMsg==WM_SETFOCUS) SetAlphaWnd(hwnd);
	  if((uMsg==WM_SETICON)&&(hwnd==hFrmSend)) SetAlphaWnd(hwnd);
	  //if((uMsg==WM_ERASEBKGND)&&(hwnd==hFrmSend)) SetAlphaWnd(hwnd);
	  if((uMsg==WM_ERASEBKGND)&&(GetForegroundWindow()==hwnd)&&(hwnd!=hLastActiveFrm)) SetAlphaWnd(hwnd);
	  if(uMsg==WM_PAINT) SetAlphaWnd(hwnd);
	  if(uMsg==WM_NCPAINT) SetAlphaWnd(hwnd);
	  //if(uMsg==WM_NOTIFY) SetAlphaWnd(hwnd);
	  //if(uMsg==WM_SETTEXT) SetAlphaWnd(hwnd);
	  //if(uMsg==WM_SETICON) SetAlphaWnd(hwnd);
	}
	//Usuniecie przezroczystosci
	if(uMsg==WM_CLOSE)
	{
	  //Przywrocenie starej procki
	  if(ReciveCurrentProc(hwnd)==(WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC))
	   SetWindowLongPtrW(hwnd, GWLP_WNDPROC,(LONG)ReciveOldProc(hwnd));
	  //Samo wyrejestrowanie hooka
	  else
	  {
		//Przekazanie starej procki przez API
		TPluginHook PluginHook;
		PluginHook.HookName = ALPHAWINDOWS_OLDPROC;
		PluginHook.wParam = (WPARAM)ReciveOldProc(hwnd);
		PluginHook.lParam = (LPARAM)hwnd;
		PluginLink.CallService(AQQ_SYSTEM_SENDHOOK,(WPARAM)(&PluginHook),0);
		//Wyrejestrowanie hooka
		SetWindowLongPtrW(hwnd, GWLP_WNDPROC,(LONG)(WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC));
	  }
	  //Usuniecie przezroczystosci
	  RemoveAlphaWnd(hwnd);
	  //Pobranie indeksu rekordu
	  int Idx = ReciveIdx(hwnd);
	  //Zapisanie starej procki do zwrotu
	  WNDPROC OldProc = WinProcTable[Idx].OldProc;
	  //Usuniecie danych w rekordzie
	  WinProcTable[Idx].hwnd = NULL;
	  WinProcTable[Idx].OldProc = NULL;
	  //Zwrot w funkcji
	  return CallWindowProc(OldProc, hwnd, uMsg, wParam, lParam);
	}
  }

  return CallWindowProc(ReciveOldProc(hwnd), hwnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

//Hook na wylaczenie komunikatora poprzez usera
int __stdcall OnBeforeUnload(WPARAM wParam, LPARAM lParam)
{
  //Info o rozpoczeciu procedury zamykania komunikatora
  ForceUnloadExecuted = true;

  return 0;
}
//---------------------------------------------------------------------------

//Hook na aktywna zakladke
int __stdcall OnPrimaryTab(WPARAM wParam, LPARAM lParam)
{
  //Uchwyt do okna rozmowy nie zostal jeszcze pobrany
  if(!hFrmSend)
  {
	//Przypisanie uchwytu okna rozmowy
	hFrmSend = (HWND)(int)wParam;
	//Pobierane wolnego rekordu
	int Idx = ReciveFreeIdx();
	//Zapisanie uchwytu do okna
	WinProcTable[Idx].hwnd = hFrmSend;
	//Zmiana procki okna + zapisanie starej procki
	WinProcTable[Idx].OldProc = (WNDPROC)SetWindowLongPtrW(hFrmSend, GWLP_WNDPROC,(LONG)FrmProc);
	//Zapisanie aktualnej procki
	WinProcTable[Idx].CurrentProc = (WNDPROC)GetWindowLongPtr(hFrmSend, GWLP_WNDPROC);
	//Ustawienie przezroczystosci
	SetAlphaWnd(hFrmSend);
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na odbieranie starej procki przekazanej przez wtyczke TabKit
int __stdcall OnRecvOldProc(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie przekazanego uchwytu do okna
  HWND hwnd = (HWND)lParam;
  //Wskazany uchwyt znajduje sie w tablicy
  if(ReciveIdx(hwnd)!=-1)
  {
	//Zmiana starej procki
	if(ReciveCurrentProc(hwnd)==(WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC))
	 WinProcTable[ReciveIdx(hwnd)].OldProc = (WNDPROC)wParam;
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na zmianê kompozycji
int __stdcall OnThemeChanged(WPARAM wParam, LPARAM lParam)
{
  //Okno ustawien zostalo juz stworzone
  if(hSettingsForm)
  {
	//Wlaczona zaawansowana stylizacja okien
	if(ChkSkinEnabled())
	{
	  UnicodeString ThemeSkinDir = GetThemeSkinDir();
	  //Plik zaawansowanej stylizacji okien istnieje
	  if(FileExists(ThemeSkinDir + "\\\\Skin.asz"))
	  {
		ThemeSkinDir = StringReplace(ThemeSkinDir, "\\\\", "\\", TReplaceFlags() << rfReplaceAll);
		hSettingsForm->sSkinManager->SkinDirectory = ThemeSkinDir;
		hSettingsForm->sSkinManager->SkinName = "Skin.asz";
		if(ChkThemeAnimateWindows()) hSettingsForm->sSkinManager->AnimEffects->FormShow->Time = 200;
		else hSettingsForm->sSkinManager->AnimEffects->FormShow->Time = 0;
		hSettingsForm->sSkinManager->Effects->AllowGlowing = ChkThemeGlowing();
		hSettingsForm->sSkinManager->Active = true;
	  }
	  //Brak pliku zaawansowanej stylizacji okien
	  else hSettingsForm->sSkinManager->Active = false;
	}
	//Zaawansowana stylizacja okien wylaczona
	else hSettingsForm->sSkinManager->Active = false;
  }
  //Odczyt ustawien - sprawdzanie czy w zmienionej kompozycji jest plik konfiguracyjny
  LoadSettings();
  //Ustawienie przezroczystosci dla wszystkich okien
  SetAlpha();

  return 0;
}
//---------------------------------------------------------------------------

//Hook na zamkniecie/otwarcie okien
int __stdcall OnWindowEvent(WPARAM wParam, LPARAM lParam)
{
  if(!ForceUnloadExecuted)
  {
	//Pobranie informacji o oknie i eventcie
	WindowEvent = (PPluginWindowEvent)lParam;
	int Event = WindowEvent->WindowEvent;
	UnicodeString ClassName = (wchar_t*)WindowEvent->ClassName;
	//Otwarcie okna
	if(Event==WINDOW_EVENT_CREATE)
	{
	  //Pobranie uchwytu do okna
	  HWND hwnd = (HWND)(int)WindowEvent->Handle;
	  //Okno kontaktow
	  if(ClassName=="TfrmMain")
	  {
		//Zapisanie uchwytu do okna
		hFrmMain = hwnd;
		//Pobranie uchwytu do aplikacji
		hAppHandle = (HINSTANCE)GetWindowLong(hFrmMain,GWLP_HINSTANCE);
	  }
	  //Okno rozmowy
	  else if(ClassName=="TfrmSend") hFrmSend = hwnd;
	  //Okno chmurki informacyjnej
	  else if(ClassName=="TfrmMiniStatus") hFrmMiniStatus = hwnd;
	  //Okno odtwarzacza YouTube
	  else if(ClassName=="TfrmYTMovie") hFrmYTMovie = hwnd;
	  //Okno instalowania dodatkow
	  else if(ClassName=="TfrmInstallAddon") FrmInstallAddonExist = true;
	  //Jezeli procka dla okna nie istnieje
	  if(!ReciveOldProc(hwnd))
	  {
		//Pobierane wolnego rekordu
		int Idx = ReciveFreeIdx();
		//Zapisanie uchwytu do okna
		WinProcTable[Idx].hwnd = hwnd;
		//Zmiana procki okna + zapisanie starej procki
		WinProcTable[Idx].OldProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC,(LONG)FrmProc);
		//Zapisanie aktualnej procki
		WinProcTable[Idx].CurrentProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
	  }
	}
	//Zamkniecie okna
	if(Event==WINDOW_EVENT_CLOSE)
	{
	  //Pobranie uchwytu do okna
	  HWND hwnd = (HWND)(int)WindowEvent->Handle;
	  //Okno rozmowy
	  if(ClassName=="TfrmSend") hFrmSend = NULL;
	  //Okno chmurki informacyjnej
	  else if(ClassName=="TfrmMiniStatus") hFrmMiniStatus = NULL;
	  //Okno odtwarzacza YouTube
	  else if(ClassName=="TfrmYTMovie") hFrmYTMovie = NULL;
	  //Okno instalowania dodatkow
	  else if(ClassName=="TfrmInstallAddon") FrmInstallAddonExist = false;
	  //Jezeli procka dla okna nadal istnieje
	  if(ReciveOldProc(hwnd))
	  {
		//Przywrocenie starej procki
		if(ReciveCurrentProc(hwnd)==(WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC))
		 SetWindowLongPtrW(hwnd, GWLP_WNDPROC,(LONG)ReciveOldProc(hwnd));
		//Samo wyrejestrowanie hooka
		else
		{
		  //Przekazanie starej procki przez API
		  TPluginHook PluginHook;
		  PluginHook.HookName = ALPHAWINDOWS_OLDPROC;
		  PluginHook.wParam = (WPARAM)ReciveOldProc(hwnd);
		  PluginHook.lParam = (LPARAM)hwnd;
		  PluginLink.CallService(AQQ_SYSTEM_SENDHOOK,(WPARAM)(&PluginHook),0);
          //Wyrejestrowanie hooka
		  SetWindowLongPtrW(hwnd, GWLP_WNDPROC,(LONG)(WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC));
		}
		//Usuniecie przezroczystosci
		RemoveAlphaWnd(hwnd);
		//Pobranie indeksu rekordu
		int Idx = ReciveIdx(hwnd);
		//Usuniecie danych w rekordzie
		WinProcTable[Idx].hwnd = NULL;
		WinProcTable[Idx].OldProc = NULL;
	  }
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

//Zapisywanie zasobów
void ExtractRes(wchar_t* FileName, wchar_t* ResName, wchar_t* ResType)
{
  TPluginTwoFlagParams PluginTwoFlagParams;
  PluginTwoFlagParams.cbSize = sizeof(TPluginTwoFlagParams);
  PluginTwoFlagParams.Param1 = ResName;
  PluginTwoFlagParams.Param2 = ResType;
  PluginTwoFlagParams.Flag1 = (int)HInstance;
  PluginLink.CallService(AQQ_FUNCTION_SAVERESOURCE,(WPARAM)&PluginTwoFlagParams,(LPARAM)FileName);
}
//---------------------------------------------------------------------------

//Obliczanie sumy kontrolnej pliku
UnicodeString MD5File(UnicodeString FileName)
{
  if(FileExists(FileName))
  {
	UnicodeString Result;
    TFileStream *fs;

	fs = new TFileStream(FileName, fmOpenRead | fmShareDenyWrite);
	try
	{
	  TIdHashMessageDigest5 *idmd5= new TIdHashMessageDigest5();
	  try
	  {
	    Result = idmd5->HashStreamAsHex(fs);
	  }
	  __finally
	  {
		delete idmd5;
	  }
    }
	__finally
    {
	  delete fs;
    }

    return Result;
  }
  else
   return 0;
}
//---------------------------------------------------------------------------

//Odczyt ustawien
void LoadSettings()
{
  //Sprawdzenie pliku konfiguracyjnego kompozycji
  TIniFile *Ini = new TIniFile(GetThemeDir()+"\\\\theme.ini");
  //Pre-konfigurowana przezroczystosc w kompozycji
  if(Ini->ValueExists("AlphaWindows","AlphaValue"))
  {
	AlphaValue = 255 - Ini->ReadInteger("AlphaWindows","AlphaValue",30);
  }
  //Wartosc przezroczystosci zdefiniowana przez wtyczke
  else
  {
	Ini = new TIniFile(GetPluginUserDir()+"\\\\AlphaWindows\\\\Settings.ini");
	AlphaValue = 255 - Ini->ReadInteger("Settings","AlphaValue",30);
  }
  delete Ini;
}
//---------------------------------------------------------------------------

extern "C" int __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
  //Linkowanie wtyczki z komunikatorem
  PluginLink = *Link;
  //Pobranie PID procesu AQQ
  ProcessPID = getpid();
  //Pobranie sciezki do prywatnego folderu wtyczek
  UnicodeString PluginUserDir = GetPluginUserDir();
  //Folder z ustawieniami wtyczki
  if(!DirectoryExists(PluginUserDir + "\\\\AlphaWindows"))
   CreateDir(PluginUserDir + "\\\\AlphaWindows");
  //Wczytanie ustawien
  LoadSettings();
  //Wypakiwanie ikonki AlphaWindows.dll.png
  //38B9FF3EF3526B3E5DC4FCA991BB3D81
  if(!DirectoryExists(PluginUserDir + "\\\\Shared"))
   CreateDir(PluginUserDir + "\\\\Shared");
  if(!FileExists(PluginUserDir + "\\\\Shared\\\\AlphaWindows.dll.png"))
   ExtractRes((PluginUserDir + "\\\\Shared\\\\AlphaWindows.dll.png").w_str(),L"PLUGIN_RES",L"DATA");
  else if(MD5File(PluginUserDir + "\\\\Shared\\\\AlphaWindows.dll.png")!="38B9FF3EF3526B3E5DC4FCA991BB3D81")
   ExtractRes((PluginUserDir + "\\\\Shared\\\\AlphaWindows.dll.png").w_str(),L"PLUGIN_RES",L"DATA");
  //Hook na wylaczenie komunikatora poprzez usera
  PluginLink.HookEvent(AQQ_SYSTEM_BEFOREUNLOAD,OnBeforeUnload);
  //Hook na odbieranie starej procki przekazanej przez wtyczke TabKit
  PluginLink.HookEvent(TABKIT_OLDPROC,OnRecvOldProc);
  //Hook na zmiane kompozycji
  PluginLink.HookEvent(AQQ_SYSTEM_THEMECHANGED, OnThemeChanged);
  //Hook na zamkniecie/otwarcie okien
  PluginLink.HookEvent(AQQ_SYSTEM_WINDOWEVENT,OnWindowEvent);
  //Wszystkie moduly zostaly zaladowane
  if(PluginLink.CallService(AQQ_SYSTEM_MODULESLOADED,0,0))
  {
	//Szukanie uchwytu do okna kontaktowa
	EnumWindows((WNDENUMPROC)FindFrmMain,0);
	//Pobierane wolnego rekordu
	int Idx = ReciveFreeIdx();
	//Zapisanie uchwytu do okna
	WinProcTable[Idx].hwnd = hFrmMain;
	//Zmiana procki okna + zapisanie starej procki
	WinProcTable[Idx].OldProc = (WNDPROC)SetWindowLongPtrW(hFrmMain, GWLP_WNDPROC,(LONG)FrmProc);
	//Zapisanie aktualnej procki
	WinProcTable[Idx].CurrentProc = (WNDPROC)GetWindowLongPtr(hFrmMain, GWLP_WNDPROC);
	//Ustawienie przezroczystosci
	SetAlphaWnd(hFrmMain);
	//Hook na aktywna zakladke
	PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_PRIMARYTAB,OnPrimaryTab);
	//Pobieranie aktywnych zakladek
	PluginLink.CallService(AQQ_CONTACTS_BUDDY_FETCHALLTABS,0,0);
	//Usuniecie hooka na aktywna zakladke
	PluginLink.UnhookEvent(OnPrimaryTab);
  }
  //Rejestowanie klasy okna timera
  WNDCLASSEX wincl;
  wincl.cbSize = sizeof (WNDCLASSEX);
  wincl.style = 0;
  wincl.lpfnWndProc = TimerFrmProc;
  wincl.cbClsExtra = 0;
  wincl.cbWndExtra = 0;
  wincl.hInstance = HInstance;
  wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
  wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
  wincl.lpszMenuName = NULL;
  wincl.lpszClassName = L"TAlphaWindowsTimer";
  wincl.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
  RegisterClassEx(&wincl);
  //Tworzenie okna timera
  hTimerFrm = CreateWindowEx(0, L"TAlphaWindowsTimer", L"",	0, 0, 0, 0, 0, NULL, NULL, HInstance, NULL);
  //Timer na sprawdzanie aktywnego okna
  SetTimer(hTimerFrm,TIMER_CHKACTIVEWINDOW,25,(TIMERPROC)TimerFrmProc);

  return 0;
}
//---------------------------------------------------------------------------

//Wyladowanie wtyczki
extern "C" int __declspec(dllexport) __stdcall Unload()
{
  //Wyladowanie timera
  KillTimer(hTimerFrm,TIMER_CHKACTIVEWINDOW);
  //Usuwanie okna timera
  DestroyWindow(hTimerFrm);
  //Wyrejestowanie klasy okna timera
  UnregisterClass(L"TAlphaWindowsTimer",HInstance);
  //Wyladowanie wszystkich hookow
  PluginLink.UnhookEvent(OnBeforeUnload);
  PluginLink.UnhookEvent(OnRecvOldProc);
  PluginLink.UnhookEvent(OnThemeChanged);
  PluginLink.UnhookEvent(OnWindowEvent);
  //Usuwanie przezroczystosci i zdjemowanie hookow
  for(int Count=0;Count<30;Count++)
  {
	//Rekord zawiera uchwyt i stara procke
	if((WinProcTable[Count].hwnd)&&(WinProcTable[Count].OldProc))
	{
	  //Przywrocenie starej procki
	  if(WinProcTable[Count].CurrentProc==(WNDPROC)GetWindowLongPtr(WinProcTable[Count].hwnd, GWLP_WNDPROC))
	   SetWindowLongPtrW(WinProcTable[Count].hwnd, GWLP_WNDPROC,(LONG)ReciveOldProc(WinProcTable[Count].hwnd));
	  //Samo wyrejestrowanie hooka
	  else
	  {
		//Przekazanie starej procki przez API
		TPluginHook PluginHook;
		PluginHook.HookName = ALPHAWINDOWS_OLDPROC;
		PluginHook.wParam = (WPARAM)WinProcTable[Count].OldProc;
		PluginHook.lParam = (LPARAM)WinProcTable[Count].hwnd;
		PluginLink.CallService(AQQ_SYSTEM_SENDHOOK,(WPARAM)(&PluginHook),0);
		//Wyrejestrowanie hooka
		SetWindowLongPtrW(WinProcTable[Count].hwnd, GWLP_WNDPROC,(LONG)(WNDPROC)GetWindowLongPtr(WinProcTable[Count].hwnd, GWLP_WNDPROC));
	  }
	  //Usuwanie przezroczystosci
	  if((!ForceUnloadExecuted)&&(!FrmInstallAddonExist)) RemoveAlphaWnd(WinProcTable[Count].hwnd);
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

//Ustawienia wtyczki
extern "C" int __declspec(dllexport)__stdcall Settings()
{
  //Przypisanie uchwytu do formy ustawien
  if(!hSettingsForm)
  {
	Application->Handle = (HWND)SettingsForm;
	hSettingsForm = new TSettingsForm(Application);
  }
  //Pokaznie okna ustawien
  hSettingsForm->Show();
  
  return 0;
}
//---------------------------------------------------------------------------

//Informacje o wtyczce
extern "C" PPluginInfo __declspec(dllexport) __stdcall AQQPluginInfo(DWORD AQQVersion)
{
  PluginInfo.cbSize = sizeof(TPluginInfo);
  PluginInfo.ShortName = L"AlphaWindows";
  PluginInfo.Version = PLUGIN_MAKE_VERSION(1,0,0,0);
  PluginInfo.Description = L"Pozwala na ustawienie przeŸroczystoœci dla wszystkich dostêpnych w komunikatorze okien.";
  PluginInfo.Author = L"Krzysztof Grochocki (Beherit)";
  PluginInfo.AuthorMail = L"kontakt@beherit.pl";
  PluginInfo.Copyright = L"Krzysztof Grochocki (Beherit)";
  PluginInfo.Homepage = L"http://beherit.pl";

  return &PluginInfo;
}
//---------------------------------------------------------------------------
