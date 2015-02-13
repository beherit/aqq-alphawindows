//---------------------------------------------------------------------------
// Copyright (C) 2013-2015 Krzysztof Grochocki
//
// This file is part of AlphaWindows
//
// AlphaWindows is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// AlphaWindows is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Radio; see the file COPYING. If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street,
// Boston, MA 02110-1301, USA.
//---------------------------------------------------------------------------

#include <vcl.h>
#include <windows.h>
#include <process.h>
#include <inifiles.hpp>
#include <IdHashMessageDigest.hpp>
#include <PluginAPI.h>
#include <LangAPI.hpp>
#pragma hdrstop
#include "SettingsFrm.h"
#define WM_ALPHAWINDOWS (WM_USER + 666)

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
//---------------------------------------------------------------------------
//Tablica z uchwytem i stara procka okna
struct TWinProcTable
{
	HWND hwnd;
	WNDPROC OldProc;
};
DynamicArray<TWinProcTable> WinProcTable;
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
//Uchwyt do okna z chmurka informacyjna powiadomien
HWND hFrmNewsWidget;
//Uchwyt do okna autoryzacji
HWND hFrmSocialAuth;
//Uchwyt do okna "Daj mi znac"
HWND hFrmInfoAlert;
//Uchwyt do okna tworzenia wycinka
HWND hFrmPos;
//Uchwyt do okna odtwarzacza YouTube
HWND hFrmYTMovie;
//Uchwyt do okna VOIP
HWND hFrmVOIP;
//Uchwyt do okna pogladu kamerki
HWND hFrmVideoPreview;
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
#define TIMER_CLEARTABLE 20
#define TIMER_FRMMAIN_SETALPHA 30
#define TIMER_SETALPHA 40
//FORWARD-TIMER--------------------------------------------------------------
LRESULT CALLBACK TimerFrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//FORWARD-WINDOW-PROC--------------------------------------------------------
LRESULT CALLBACK FrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//FORWARD-AQQ-HOOKS----------------------------------------------------------
INT_PTR __stdcall OnBeforeUnload(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnColorChange(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnLangCodeChanged(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnTabChanged(WPARAM wParam, LPARAM lPaam);
INT_PTR __stdcall OnThemeChanged(WPARAM wParam, LPARAM lPaam);
INT_PTR __stdcall OnWindowEvent(WPARAM wParam, LPARAM lParam);
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

//Sprawdzanie czy wlaczona jest zaawansowana stylizacja okien
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

//Pobieranie ustawien koloru AlphaControls
int GetHUE()
{
	return (int)PluginLink.CallService(AQQ_SYSTEM_COLORGETHUE,0,0);
}
//---------------------------------------------------------------------------
int GetSaturation()
{
	return (int)PluginLink.CallService(AQQ_SYSTEM_COLORGETSATURATION,0,0);
}
//---------------------------------------------------------------------------
int GetBrightness()
{
	return (int)PluginLink.CallService(AQQ_SYSTEM_COLORGETBRIGHTNESS,0,0);
}
//---------------------------------------------------------------------------

//Pobieranie starej procki na podstawie uchwytu okna
WNDPROC ReciveOldProc(HWND hwnd)
{
	//Przejrzenie calej tablicy
	for(int Count=0;Count<WinProcTable.Length;Count++)
	{
		if(WinProcTable[Count].hwnd==hwnd)
			return WinProcTable[Count].OldProc;
	}
	return NULL;
}
//---------------------------------------------------------------------------

//Pobieranie indeksu rekordu na podstawie uchwytu okna
int ReciveIdx(HWND hwnd)
{
	//Przejrzenie calej tablicy
	for(int Count=0;Count<WinProcTable.Length;Count++)
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
	//Okno jest widoczne
	if(IsWindowVisible(hwnd))
	{
		//Ustawienie przezroczystosci
		PluginLink.CallService(AQQ_WINDOW_TRANSPARENT,(WPARAM)hwnd,(LPARAM)AlphaValue);
		//Pobieranie klasy okna
		wchar_t* ClassName = new wchar_t[128];
		GetClassNameW(hwnd, ClassName, 128);
		//Wylaczanie przezroczystosci dla danych okien
		if((hwnd==hFrmPos)||(hwnd==hFrmYTMovie)||(hwnd==hFrmVOIP)||(hwnd==hFrmVideoPreview)||((UnicodeString)ClassName=="ShockwaveFlashFullScreen"))
			Vcl::Forms::SetLayeredWindowAttributes((int)hwnd, 0, 255, LWA_ALPHA);
		//Wysylanie informacji o zmianie przezroczystosci do okna z wtyczki
		if(hAppHandle!=(HINSTANCE)GetWindowLong(hwnd,GWLP_HINSTANCE))
			SendMessage(hwnd,WM_ALPHAWINDOWS,0,(LPARAM)AlphaValue);
	}
}
//---------------------------------------------------------------------------

//Ustawienie przezroczystosci dla wszystkich okien
void SetAlpha()
{
	//Przejrzenie calej tablicy
	for(int Count=0;Count<WinProcTable.Length;Count++)
		SetAlphaWnd(WinProcTable[Count].hwnd);
}
//---------------------------------------------------------------------------
void SetAlphaW()
{
	//Przejrzenie calej tablicy
	for(int Count=0;Count<WinProcTable.Length;Count++)
	{
		//Okno nie jest chmurka informacyjna, oknem z chmurka informacyjna powiadomien, oknem autoryzacji lub oknem "Daj mi znac"
		if((WinProcTable[Count].hwnd!=hFrmMiniStatus)&&(WinProcTable[Count].hwnd!=hFrmNewsWidget)&&(WinProcTable[Count].hwnd!=hFrmSocialAuth)&&(WinProcTable[Count].hwnd!=hFrmInfoAlert))
			SetAlphaWnd(WinProcTable[Count].hwnd);
	}
}
//---------------------------------------------------------------------------

//Ustawienie przezroczystosci dla wszystkich okien z podaniem wartosci
void SetAlphaEx(int Value)
{
	//Przejrzenie calej tablicy
	for(int Count=0;Count<WinProcTable.Length;Count++)
	{
		//Okno jest widoczne oraz nie jest to okno wtyczki AlphaWindow
		if((IsWindowVisible(WinProcTable[Count].hwnd))&&(WinProcTable[Count].hwnd!=hSettingsForm->Handle))
		{
			//Okno nie jest chmurka informacyjna, oknem z chmurka informacyjna powiadomien, oknem autoryzacji lub oknem "Daj mi znac"
			if((WinProcTable[Count].hwnd!=hFrmMiniStatus)&&(WinProcTable[Count].hwnd!=hFrmNewsWidget)&&(WinProcTable[Count].hwnd!=hFrmSocialAuth)&&(WinProcTable[Count].hwnd!=hFrmInfoAlert))
			{
				//Ustawienie przezroczystosci
				PluginLink.CallService(AQQ_WINDOW_TRANSPARENT,(WPARAM)WinProcTable[Count].hwnd,(LPARAM)Value);
				//Pobieranie klasy okna
				wchar_t* ClassName = new wchar_t[128];
				GetClassNameW(WinProcTable[Count].hwnd, ClassName, 128);
				//Wylaczanie przezroczystosci dla danych okien
				if((WinProcTable[Count].hwnd==hFrmPos)||(WinProcTable[Count].hwnd==hFrmYTMovie)||(WinProcTable[Count].hwnd==hFrmVOIP)||(WinProcTable[Count].hwnd==hFrmVideoPreview)||((UnicodeString)ClassName=="ShockwaveFlashFullScreen"))
					Vcl::Forms::SetLayeredWindowAttributes((int)WinProcTable[Count].hwnd, 0, 255, LWA_ALPHA);
				//Wysylanie informacji o zmianie przezroczystosci do okna z wtyczki
				if(hAppHandle!=(HINSTANCE)GetWindowLong(WinProcTable[Count].hwnd,GWLP_HINSTANCE))
					SendMessage(WinProcTable[Count].hwnd,WM_ALPHAWINDOWS,0,(LPARAM)Value);
			}
		}
	}
}
//---------------------------------------------------------------------------

//Usuwanie przezroczystosci z danego okna
void RemoveAlphaWnd(HWND hwnd)
{
	//Okno jest widoczne
	if(IsWindowVisible(hwnd))
	{
		//Ustawienie przezroczystosci
		PluginLink.CallService(AQQ_WINDOW_TRANSPARENT,(WPARAM)hwnd,255);
		//Wysylanie informacji o zmianie przezroczystosci do okna z wtyczki
		if(hAppHandle!=(HINSTANCE)GetWindowLong(hwnd,GWLP_HINSTANCE))
			SendMessage(hwnd,WM_ALPHAWINDOWS,0,(LPARAM)255);
	}
}
//---------------------------------------------------------------------------

//Szukanie uchwytu do okna kontaktow + uchwytu do aplikacji
bool CALLBACK FindFrmMain(HWND hwnd, LPARAM lParam)
{
	//Pobranie PID okna
	DWORD PID;
	GetWindowThreadProcessId(hwnd, &PID);
	//Okno pochodzace z komunikatora
	if(PID==ProcessPID)
	{
		//Pobranie klasy okna
		wchar_t WClassName[128];
		GetClassNameW(hwnd, WClassName, sizeof(WClassName));
		//Okno kontaktow
		if((UnicodeString)WClassName=="TfrmMain")
		{
			//Przypisanie uchwytu
			hFrmMain = hwnd;
			//Pobranie uchwytu do aplikacji
			hAppHandle = (HINSTANCE)GetWindowLong(hFrmMain,GWLP_HINSTANCE);
			//Zakonczenie enumeracji
			return false;
		}
	}
	return true;
}
//---------------------------------------------------------------------------

//Szukanie uchwytow do okien komunikatora
bool CALLBACK EnumAppWindows(HWND hwnd, LPARAM lParam)
{
	//Pobranie PID okna
	DWORD PID;
	GetWindowThreadProcessId(hwnd, &PID);
	//Okno pochodzace z komunikatora
	if(PID==ProcessPID)
	{
		//Pobranie klasy okna
		wchar_t WClassName[128];
		GetClassNameW(hwnd, WClassName, sizeof(WClassName));
		//Okno rozmowy
		if((UnicodeString)WClassName=="TfrmSend") hFrmSend = hwnd;
		//Okno chmurki informacyjnej
		else if((UnicodeString)WClassName=="TfrmMiniStatus") hFrmMiniStatus = hwnd;
		//Okno chmurki informacyjnej powiadomien
		else if((UnicodeString)WClassName=="TfrmNewsWidget") hFrmNewsWidget = hwnd;
		//Okno autoryzacji
		else if((UnicodeString)WClassName=="TfrmSocialAuth") hFrmSocialAuth = hwnd;
		//Okno "Daj mi znac"
		else if((UnicodeString)WClassName=="TfrmInfoAlert") hFrmInfoAlert = hwnd;
		//Okno tworzenia wycinka
		else if((UnicodeString)WClassName=="TfrmPos") hFrmPos = hwnd;
		//Okno odtwarzacza YouTube
		else if((UnicodeString)WClassName=="TfrmYTMovie") hFrmYTMovie = hwnd;
		//Okno VOIP
		else if((UnicodeString)WClassName=="TfrmVOIP") hFrmVOIP = hwnd;
		//Okno pogladu kamerki
		else if((UnicodeString)WClassName=="TfrmVideoPreview") hFrmVideoPreview = hwnd;
		//Znaleziony uchwyt jest prawdziwym oknem
		if((IsWindow(hwnd))&&(IsWindowVisible(hwnd))&&((UnicodeString)WClassName!="TForm")&&((UnicodeString)WClassName!="Internet Explorer_Hidden"))
		{
			//Ustawienie przezroczystosci
			SetAlphaWnd(hwnd);
			//Ustawianie nowego rekordu
			int Idx = WinProcTable.Length;
			WinProcTable.Length = WinProcTable.Length + 1;
			//Zapisanie uchwytu do okna
			WinProcTable[Idx].hwnd = hwnd;
			//Nie jest to okno wtyczki
			if(hAppHandle==(HINSTANCE)GetWindowLong(hwnd,GWLP_HINSTANCE))
			{
				//Zmiana procki okna + zapisanie starej procki
				WinProcTable[Idx].OldProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC,(LONG)FrmProc);
			}
			else WinProcTable[Idx].OldProc = NULL;
		}
	}

	return true;
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
			if(hActiveFrm!=hLastActiveFrm)
			{
				//Zmienna trzymajaca PID procesu
				DWORD PID;
				//Pobranie PID procesu poprzedniego okna
				GetWindowThreadProcessId(hLastActiveFrm, &PID);
				//Jezeli poprzednie okno bylo oknem z wtyczki
				if((PID==ProcessPID)&&(hAppHandle!=(HINSTANCE)GetWindowLong(hLastActiveFrm,GWLP_HINSTANCE))&&(IsWindowVisible(hLastActiveFrm)))
					//Ustawienie przezroczystysci obramowania okna
					SendMessage(hLastActiveFrm,WM_ALPHAWINDOWS,0,(LPARAM)AlphaValue);
				//Pobranie PID procesu aktywnego okna
				GetWindowThreadProcessId(hActiveFrm, &PID);
				//Okno pochodzi z komunikatora
				if(PID==ProcessPID)
				{
					//Pobranie klasy okna
					wchar_t WClassName[128];
					GetClassNameW(hActiveFrm, WClassName, sizeof(WClassName));
					//Okno kontaktow
					if((UnicodeString)WClassName=="TfrmMain") hFrmMain = hActiveFrm;
					//Okno rozmowy
					else if((UnicodeString)WClassName=="TfrmSend") hFrmSend = hActiveFrm;
					//Okno chmurki informacyjnej
					else if((UnicodeString)WClassName=="TfrmMiniStatus") hFrmMiniStatus = hActiveFrm;
					//Okna chmurki informacyjnej powiadomien
					else if((UnicodeString)WClassName=="TfrmNewsWidget") hFrmNewsWidget = hActiveFrm;
					//Okno autoryzacji
					else if((UnicodeString)WClassName=="TfrmSocialAuth") hFrmSocialAuth = hActiveFrm;
					//Okno "Daj mi znac"
					else if((UnicodeString)WClassName=="TfrmInfoAlert") hFrmInfoAlert = hActiveFrm;
					//Okno tworzenia wycinka
					else if((UnicodeString)WClassName=="TfrmPos") hFrmPos = hActiveFrm;
					//Okno odtwarzacza YouTube
					else if((UnicodeString)WClassName=="TfrmYTMovie") hFrmYTMovie = hActiveFrm;
					//Okno VOIP
					else if((UnicodeString)WClassName=="TfrmVOIP") hFrmVOIP = hActiveFrm;
					//Okno pogladu kamerki
					else if((UnicodeString)WClassName=="TfrmVideoPreview") hFrmVideoPreview = hActiveFrm;
					//Zabezpieczenie przed bialym tlem po zamknieciu okna
					if(((IsWindowVisible(hLastActiveFrm))||(!hLastActiveFrm))&&(hActiveFrm!=hFrmMiniStatus)&&(hActiveFrm!=hFrmNewsWidget)&&(hActiveFrm!=hFrmSocialAuth)&&(hActiveFrm!=hFrmInfoAlert)&&(hActiveFrm!=hFrmMain))
					{
						//Ustawienie przezroczystysci okna
						SetAlphaWnd(hActiveFrm);
						//Nie jest to okno wtyczki
						if(hAppHandle==(HINSTANCE)GetWindowLong(hActiveFrm,GWLP_HINSTANCE))
						{
							//Nowa procka dla okna nie istnieje
							if(!ReciveOldProc(hActiveFrm))
							{
								//Ustawianie nowego rekordu
								int Idx = WinProcTable.Length;
								WinProcTable.Length = WinProcTable.Length + 1;
								//Zapisanie uchwytu do okna
								WinProcTable[Idx].hwnd = hActiveFrm;
								//Zmiana procki okna + zapisanie starej procki
								WinProcTable[Idx].OldProc = (WNDPROC)SetWindowLongPtrW(hActiveFrm, GWLP_WNDPROC,(LONG)FrmProc);
							}
						}
						//Okno pochochodzi z wtyczki
						else
						{
							//Uchwyt do okna nie zostal jeszcze zapisany
							if(ReciveIdx(hActiveFrm)==-1)
							{
								//Ustawianie nowego rekordu
								int Idx = WinProcTable.Length;
								WinProcTable.Length = WinProcTable.Length + 1;
								//Zapisanie uchwytu do okna
								WinProcTable[Idx].hwnd = hActiveFrm;
								//Zapisanie pustej procki okna
								WinProcTable[Idx].OldProc = NULL;
							}
						}
					}
				}
				//Zapamietywanie ostatnio aktywnego okna
				hLastActiveFrm = hActiveFrm;
			}
		}
		//Czyszczenie tablicy z nieistniejacych okien
		if(wParam==TIMER_CLEARTABLE)
		{
			//Przejrzenie calej tablicy
			for(int Count=0;Count<WinProcTable.Length;Count++)
			{
				//Nie jest to okno wtyczki
				if(hAppHandle==(HINSTANCE)GetWindowLong(WinProcTable[Count].hwnd,GWLP_HINSTANCE))
				{
					//Okno juz nie nie istnieje
					if(!IsWindow(WinProcTable[Count].hwnd))
					{
						//Przywrocenie starej procki
						SetWindowLongPtrW(WinProcTable[Count].hwnd, GWLP_WNDPROC,(LONG)ReciveOldProc(hwnd));
						//Przesuniecie ostatniego rekordu
						WinProcTable[Count].hwnd = WinProcTable[WinProcTable.Length-1].hwnd;
						WinProcTable[Count].OldProc = WinProcTable[WinProcTable.Length-1].OldProc;
						WinProcTable.Length = WinProcTable.Length - 1;
					}
				}
				//Okno wtyczki
				else
				{
					//Okno jest niewidoczne
					if(!IsWindowVisible(WinProcTable[Count].hwnd))
					{
						//Przesuniecie ostatniego rekordu
						WinProcTable[Count].hwnd = WinProcTable[WinProcTable.Length-1].hwnd;
						WinProcTable[Count].OldProc = WinProcTable[WinProcTable.Length-1].OldProc;
						WinProcTable.Length = WinProcTable.Length - 1;
					}
        }
			}
    }
		//Ustawienie przezroczystosci okna kontaktow
		if(wParam==TIMER_FRMMAIN_SETALPHA)
		{
			//Zatrzymanie timera
			KillTimer(hTimerFrm,TIMER_FRMMAIN_SETALPHA);
			//Ustawienie przezroczystosci
			SetAlphaWnd(hFrmMain);
		}
		//Ustawienie przezroczystosci dla wszystkich okien
		if(wParam==TIMER_SETALPHA)
		{
			//Zatrzymanie timera
			KillTimer(hTimerFrm,TIMER_SETALPHA);
			//Ustawienie przezroczystosci
			SetAlphaW();
		}

		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

LRESULT CALLBACK FrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//Komunikator nie jest zamykany
	if(!ForceUnloadExecuted)
	{
		//Ustawienie przezroczystosci dla chmurki informacyjnej, okna z chmurka informacyjna powiadomien, okna autoryzacji, okna "Daj mi znac"
		if((hwnd==hFrmMiniStatus)||(hwnd==hFrmNewsWidget)||(hwnd==hFrmInfoAlert))
		{
			if(uMsg==WM_NCPAINT) SetAlphaWnd(hwnd);
		}
		//Ustawienie przezroczystosci dla pozostalych okien
		else
		{
			if((uMsg==WM_ACTIVATE)&&((wParam==WA_ACTIVE)||(wParam==WA_CLICKACTIVE)))
			{
				if(hwnd==hFrmMain) SetTimer(hTimerFrm,TIMER_FRMMAIN_SETALPHA,25,(TIMERPROC)TimerFrmProc);
				else SetAlphaWnd(hwnd);
			}
			if((uMsg==WM_ACTIVATE)&&(wParam==WA_INACTIVE)) SetAlphaWnd(hwnd);
			if(uMsg==WM_ACTIVATEAPP) SetTimer(hTimerFrm,TIMER_SETALPHA,25,(TIMERPROC)TimerFrmProc);
			if(uMsg==WM_SETFOCUS) SetAlphaWnd(hwnd);
			if(uMsg==WM_PAINT)
			{
				if(hwnd==hFrmMain) SetTimer(hTimerFrm,TIMER_FRMMAIN_SETALPHA,25,(TIMERPROC)TimerFrmProc);
				else SetAlphaWnd(hwnd);
			}
			if(uMsg==WM_NCPAINT) SetAlphaWnd(hwnd);
			if((uMsg==WM_ERASEBKGND)&&(GetForegroundWindow()==hwnd)&&(hwnd!=hLastActiveFrm)) SetAlphaWnd(hwnd);
			if(uMsg==WM_SETICON) SetTimer(hTimerFrm,TIMER_SETALPHA,25,(TIMERPROC)TimerFrmProc);
		}
		//Usuniecie przezroczystosci
		if(uMsg==WM_CLOSE)
		{
			//Usuniecie przezroczystosci
			RemoveAlphaWnd(hwnd);
		}
	}
	//Przypisanie starej procki do okna
	if(uMsg==WM_CLOSE)
	{
		//Przywrocenie starej procki
		SetWindowLongPtrW(hwnd, GWLP_WNDPROC,(LONG)ReciveOldProc(hwnd));
		//Wskazany uchwyt znajduje sie w tablicy
		if(ReciveIdx(hwnd)!=-1)
		{
			//Pobranie indeksu rekordu
			int Idx = ReciveIdx(hwnd);
			//Zapisanie starej procki do zwrotu
			WNDPROC OldProc = WinProcTable[Idx].OldProc;
			//Przesuniecie ostatniego rekordu
			WinProcTable[Idx].hwnd = WinProcTable[WinProcTable.Length-1].hwnd;
			WinProcTable[Idx].OldProc = WinProcTable[WinProcTable.Length-1].OldProc;
			WinProcTable.Length = WinProcTable.Length - 1;
			//Zwrot w funkcji
			return CallWindowProc(OldProc, hwnd, uMsg, wParam, lParam);
		}
	}

	return CallWindowProc(ReciveOldProc(hwnd), hwnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

//Hook na wylaczenie komunikatora poprzez usera
INT_PTR __stdcall OnBeforeUnload(WPARAM wParam, LPARAM lParam)
{
	//Info o rozpoczeciu procedury zamykania komunikatora
	ForceUnloadExecuted = true;

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zmiane kolorystyki AlphaControls
INT_PTR __stdcall OnColorChange(WPARAM wParam, LPARAM lParam)
{
	//Okno ustawien zostalo juz stworzone
	if(hSettingsForm)
	{
		//Wlaczona zaawansowana stylizacja okien
		if(ChkSkinEnabled())
		{
			TPluginColorChange ColorChange = *(PPluginColorChange)wParam;
			hSettingsForm->sSkinManager->HueOffset = ColorChange.Hue;
			hSettingsForm->sSkinManager->Saturation = ColorChange.Saturation;
			hSettingsForm->sSkinManager->Brightness = ColorChange.Brightness;
		}
	}

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zmiane lokalizacji
INT_PTR __stdcall OnLangCodeChanged(WPARAM wParam, LPARAM lParam)
{
	//Czyszczenie cache lokalizacji
	ClearLngCache();
	//Pobranie sciezki do katalogu prywatnego uzytkownika
	UnicodeString PluginUserDir = GetPluginUserDir();
	//Ustawienie sciezki lokalizacji wtyczki
	UnicodeString LangCode = (wchar_t*)lParam;
	LangPath = PluginUserDir + "\\\\Languages\\\\AlphaWindows\\\\" + LangCode + "\\\\";
	if(!DirectoryExists(LangPath))
	{
		LangCode = (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETDEFLANGCODE,0,0);
		LangPath = PluginUserDir + "\\\\Languages\\\\AlphaWindows\\\\" + LangCode + "\\\\";
	}
	//Aktualizacja lokalizacji form wtyczki
	for(int i=0;i<Screen->FormCount;i++)
		LangForm(Screen->Forms[i]);

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zmiane zakladki w oknie kontaktow
INT_PTR __stdcall OnTabChanged(WPARAM wParam, LPARAM lParam)
{
	//Ustawienie przezroczystosci wszystkich okien
	SetTimer(hTimerFrm,TIMER_SETALPHA,25,(TIMERPROC)TimerFrmProc);

	return 0;
}
//---------------------------------------------------------------------------

//Hook na zmiane kompozycji
INT_PTR __stdcall OnThemeChanged(WPARAM wParam, LPARAM lParam)
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
				//Dane pliku zaawansowanej stylizacji okien
				ThemeSkinDir = StringReplace(ThemeSkinDir, "\\\\", "\\", TReplaceFlags() << rfReplaceAll);
				hSettingsForm->sSkinManager->SkinDirectory = ThemeSkinDir;
				hSettingsForm->sSkinManager->SkinName = "Skin.asz";
				//Ustawianie animacji AlphaControls
				if(ChkThemeAnimateWindows()) hSettingsForm->sSkinManager->AnimEffects->FormShow->Time = 200;
				else hSettingsForm->sSkinManager->AnimEffects->FormShow->Time = 0;
				hSettingsForm->sSkinManager->Effects->AllowGlowing = ChkThemeGlowing();
				//Zmiana kolorystyki AlphaControls
				hSettingsForm->sSkinManager->HueOffset = GetHUE();
				hSettingsForm->sSkinManager->Saturation = GetSaturation();
				hSettingsForm->sSkinManager->Brightness = GetBrightness();
				//Aktywacja skorkowania AlphaControls
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
INT_PTR __stdcall OnWindowEvent(WPARAM wParam, LPARAM lParam)
{
	if(!ForceUnloadExecuted)
	{
		//Pobranie informacji o oknie i eventcie
		TPluginWindowEvent WindowEvent = *(PPluginWindowEvent)lParam;
		int Event = WindowEvent.WindowEvent;
		UnicodeString ClassName = (wchar_t*)WindowEvent.ClassName;
		//Otwarcie okna
		if(Event==WINDOW_EVENT_CREATE)
		{
			//Pobranie uchwytu do okna
			HWND hwnd = (HWND)(int)WindowEvent.Handle;
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
			//Okno chmurki informacyjnej powiadomien
			else if(ClassName=="TfrmNewsWidget") hFrmNewsWidget = hwnd;
			//Okno autoryzacji
			else if(ClassName=="TfrmSocialAuth") hFrmSocialAuth = hwnd;
			//Okno "Daj mi znac"
			else if(ClassName=="TfrmInfoAlert") hFrmInfoAlert = hwnd;
			//Okno tworzenia wycinka
			else if(ClassName=="TfrmPos") hFrmPos = hwnd;
			//Okno odtwarzacza YouTube
			else if(ClassName=="TfrmYTMovie") hFrmYTMovie = hwnd;
			//Okno VOIP
			else if(ClassName=="TfrmVOIP") hFrmVOIP = hwnd;
			//Okno pogladu kamerki
			else if(ClassName=="TfrmVideoPreview") hFrmVideoPreview = hwnd;
			//Okno instalowania dodatkow
			else if(ClassName=="TfrmInstallAddon") FrmInstallAddonExist = true;
			//Nowa procka dla okna nie istnieje
			if(!ReciveOldProc(hwnd))
			{
				//Ustawianie nowego rekordu
				int Idx = WinProcTable.Length;
				WinProcTable.Length = WinProcTable.Length + 1;
				//Zapisanie uchwytu do okna
				WinProcTable[Idx].hwnd = hwnd;
				//Zmiana procki okna + zapisanie starej procki
				WinProcTable[Idx].OldProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC,(LONG)FrmProc);
			}
		}
		//Zamkniecie okna
		if(Event==WINDOW_EVENT_CLOSE)
		{
			//Pobranie uchwytu do okna
			HWND hwnd = (HWND)(int)WindowEvent.Handle;
			//Okno rozmowy
			if(ClassName=="TfrmSend") hFrmSend = NULL;
			//Okno chmurki informacyjnej
			else if(ClassName=="TfrmMiniStatus") hFrmMiniStatus = NULL;
			//Okno chmurki informacyjnej powiadomien
			else if(ClassName=="TfrmNewsWidget") hFrmNewsWidget = NULL;
			//Okno autoryzacji
			else if(ClassName=="TfrmSocialAuth") hFrmSocialAuth = NULL;
			//Okno "Daj mi znac"
			else if(ClassName=="TfrmInfoAlert") hFrmInfoAlert = NULL;
			//Okno tworzenia wycinka
			else if(ClassName=="TfrmPos") hFrmPos = NULL;
			//Okno odtwarzacza YouTube
			else if(ClassName=="TfrmYTMovie") hFrmYTMovie = NULL;
			//Okno VOIP
			else if(ClassName=="TfrmVOIP") hFrmVOIP = NULL;
			//Okno pogladu kamerki
			else if(ClassName=="TfrmVideoPreview") hFrmVideoPreview = NULL;
			//Okno instalowania dodatkow
			else if(ClassName=="TfrmInstallAddon") FrmInstallAddonExist = false;
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
	else return 0;
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
		Ini = new TIniFile(GetPluginUserDir()+"\\\\AlphaWindows\\\\Settings.ini");
		if(Ini->ReadBool("Settings","IgnoreTheme",false))
			AlphaValue = 255 - Ini->ReadInteger("Settings","AlphaValue",30);
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

extern "C" INT_PTR __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
	//Linkowanie wtyczki z komunikatorem
	PluginLink = *Link;
	//Pobranie PID procesu AQQ
	ProcessPID = getpid();
	//Wylaczanie przezroczystosci w rdzeniu AQQ
	TStrings* IniList = new TStringList();
	IniList->SetText((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_FETCHSETUP,0,0));
	TMemIniFile *Settings = new TMemIniFile(ChangeFileExt(Application->ExeName, ".INI"));
	Settings->SetStrings(IniList);
	delete IniList;
	UnicodeString AlphaMsg = Settings->ReadString("View","AlphaMsg","0");
	UnicodeString AlphaMain = Settings->ReadString("View","AlphaMain","0");
	delete Settings;
	if((StrToBool(AlphaMsg))||(StrToBool(AlphaMain)))
	{
		//Nowe ustawienia
		TSaveSetup SaveSetup;
		SaveSetup.Section = L"View";
		SaveSetup.Ident = L"AlphaMsg";
		SaveSetup.Value = L"0";
		//Zapis ustawien
		PluginLink.CallService(AQQ_FUNCTION_SAVESETUP,1,(LPARAM)(&SaveSetup));
		//Nowe ustawienia
		SaveSetup.Section = L"View";
		SaveSetup.Ident = L"AlphaMain";
		SaveSetup.Value = L"0";
		//Zapis ustawien
		PluginLink.CallService(AQQ_FUNCTION_SAVESETUP,1,(LPARAM)(&SaveSetup));
		//Odswiezenie ustawien
		PluginLink.CallService(AQQ_FUNCTION_REFRESHSETUP,0,0);
	}
	//Pobranie sciezki do prywatnego folderu wtyczek
	UnicodeString PluginUserDir = GetPluginUserDir();
	//Tworzenie katalogow lokalizacji
	if(!DirectoryExists(PluginUserDir+"\\\\Languages"))
		CreateDir(PluginUserDir+"\\\\Languages");
	if(!DirectoryExists(PluginUserDir+"\\\\Languages\\\\AlphaWindows"))
		CreateDir(PluginUserDir+"\\\\Languages\\\\AlphaWindows");
	if(!DirectoryExists(PluginUserDir+"\\\\Languages\\\\AlphaWindows\\\\EN"))
		CreateDir(PluginUserDir+"\\\\Languages\\\\AlphaWindows\\\\EN");
	if(!DirectoryExists(PluginUserDir+"\\\\Languages\\\\AlphaWindows\\\\PL"))
		CreateDir(PluginUserDir+"\\\\Languages\\\\AlphaWindows\\\\PL");
	//Wypakowanie plikow lokalizacji
	//D1EC5E8C3EE53CCE4D1BCF95A9ECC393
	if(!FileExists(PluginUserDir+"\\\\Languages\\\\AlphaWindows\\\\EN\\\\TSettingsForm.lng"))
		ExtractRes((PluginUserDir+"\\\\Languages\\\\AlphaWindows\\\\EN\\\\TSettingsForm.lng").w_str(),L"EN_SETTINGSFRM",L"DATA");
	else if(MD5File(PluginUserDir+"\\\\Languages\\\\AlphaWindows\\\\EN\\\\TSettingsForm.lng")!="D1EC5E8C3EE53CCE4D1BCF95A9ECC393")
		ExtractRes((PluginUserDir+"\\\\Languages\\\\AlphaWindows\\\\EN\\\\TSettingsForm.lng").w_str(),L"EN_SETTINGSFRM",L"DATA");
	//DF2D4466E3044A387B9D3DD8A1F108EB
	if(!FileExists(PluginUserDir+"\\\\Languages\\\\AlphaWindows\\\\PL\\\\TSettingsForm.lng"))
		ExtractRes((PluginUserDir+"\\\\Languages\\\\AlphaWindows\\\\PL\\\\TSettingsForm.lng").w_str(),L"PL_SETTINGSFRM",L"DATA");
	else if(MD5File(PluginUserDir+"\\\\Languages\\\\AlphaWindows\\\\PL\\\\TSettingsForm.lng")!="DF2D4466E3044A387B9D3DD8A1F108EB")
		ExtractRes((PluginUserDir+"\\\\Languages\\\\AlphaWindows\\\\PL\\\\TSettingsForm.lng").w_str(),L"PL_SETTINGSFRM",L"DATA");
	//Ustawienie sciezki lokalizacji wtyczki
	UnicodeString LangCode = (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETLANGCODE,0,0);
	LangPath = PluginUserDir + "\\\\Languages\\\\AlphaWindows\\\\" + LangCode + "\\\\";
	if(!DirectoryExists(LangPath))
	{
		LangCode = (wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETDEFLANGCODE,0,0);
		LangPath = PluginUserDir + "\\\\Languages\\\\AlphaWindows\\\\" + LangCode + "\\\\";
	}
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
		ExtractRes((PluginUserDir + "\\\\Shared\\\\AlphaWindows.dll.png").w_str(),L"SHARED",L"DATA");
	else if(MD5File(PluginUserDir + "\\\\Shared\\\\AlphaWindows.dll.png")!="38B9FF3EF3526B3E5DC4FCA991BB3D81")
		ExtractRes((PluginUserDir + "\\\\Shared\\\\AlphaWindows.dll.png").w_str(),L"SHARED",L"DATA");
	//Hook na wylaczenie komunikatora poprzez usera
	PluginLink.HookEvent(AQQ_SYSTEM_BEFOREUNLOAD,OnBeforeUnload);
	//Hook na zmiane kolorystyki AlphaControls
	PluginLink.HookEvent(AQQ_SYSTEM_COLORCHANGEV2,OnColorChange);
	//Hook na zmiane lokalizacji
	PluginLink.HookEvent(AQQ_SYSTEM_LANGCODE_CHANGED,OnLangCodeChanged);
	//Hook na zmiane zakladki w oknie kontaktow
	PluginLink.HookEvent(AQQ_SYSTEM_TABCHANGE, OnTabChanged);
	//Hook na zmiane kompozycji
	PluginLink.HookEvent(AQQ_SYSTEM_THEMECHANGED,OnThemeChanged);
	//Hook na zamkniecie/otwarcie okien
	PluginLink.HookEvent(AQQ_SYSTEM_WINDOWEVENT,OnWindowEvent);
	//Wszystkie moduly zostaly zaladowane
	if(PluginLink.CallService(AQQ_SYSTEM_MODULESLOADED,0,0))
	{
		//Szukanie uchwytu do okna kontaktow + uchwytu do aplikacji
		EnumWindows((WNDENUMPROC)FindFrmMain,0);
		//Szukanie uchwytow do okien komunikatora
		EnumWindows((WNDENUMPROC)EnumAppWindows,0);
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
	//Timer na czyszczenie tablicy z nieistniejacych okien
	SetTimer(hTimerFrm,TIMER_CLEARTABLE,1000,(TIMERPROC)TimerFrmProc);

	return 0;
}
//---------------------------------------------------------------------------

//Wyladowanie wtyczki
extern "C" INT_PTR __declspec(dllexport) __stdcall Unload()
{
	//Wyladowanie timerow
	KillTimer(hTimerFrm,TIMER_CHKACTIVEWINDOW);
	KillTimer(hTimerFrm,TIMER_CLEARTABLE);
	//Usuwanie okna timera
	DestroyWindow(hTimerFrm);
	//Wyrejestowanie klasy okna timera
	UnregisterClass(L"TAlphaWindowsTimer",HInstance);
	//Wyladowanie wszystkich hookow
	PluginLink.UnhookEvent(OnBeforeUnload);
	PluginLink.UnhookEvent(OnColorChange);
	PluginLink.UnhookEvent(OnLangCodeChanged);
	PluginLink.UnhookEvent(OnTabChanged);
	PluginLink.UnhookEvent(OnThemeChanged);
	PluginLink.UnhookEvent(OnWindowEvent);
	//Usuwanie przezroczystosci i zdjemowanie hookow
	for(int Count=0;Count<WinProcTable.Length;Count++)
	{
		//Rekord zawiera uchwyt do okna
		if(WinProcTable[Count].hwnd)
		{
			//Rekord zawiera stara procke okna
			if(WinProcTable[Count].OldProc)
			{
				//Przywrocenie starej procki
				SetWindowLongPtrW(WinProcTable[Count].hwnd, GWLP_WNDPROC,(LONG)WinProcTable[Count].OldProc);
			}
			//Usuwanie przezroczystosci
			if((!ForceUnloadExecuted)&&(!FrmInstallAddonExist)) RemoveAlphaWnd(WinProcTable[Count].hwnd);
		}
	}

	return 0;
}
//---------------------------------------------------------------------------

//Ustawienia wtyczki
extern "C" INT_PTR __declspec(dllexport)__stdcall Settings()
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
	PluginInfo.Version = PLUGIN_MAKE_VERSION(1,3,0,0);
	PluginInfo.Description = L"Ustawia przeŸroczystoœci dla wszystkich dostêpnych w komunikatorze okien, komunikatów oraz nawet dla okien pochodz¹cych z wtyczek.";
	PluginInfo.Author = L"Krzysztof Grochocki";
	PluginInfo.AuthorMail = L"kontakt@beherit.pl";
	PluginInfo.Copyright = L"Krzysztof Grochocki";
	PluginInfo.Homepage = L"http://beherit.pl";
	PluginInfo.Flag = 0;
	PluginInfo.ReplaceDefaultModule = 0;

	return &PluginInfo;
}
//---------------------------------------------------------------------------
