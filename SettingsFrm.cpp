//---------------------------------------------------------------------------
// Copyright (C) 2013 Krzysztof Grochocki
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

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "SettingsFrm.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "sBevel"
#pragma link "sPageControl"
#pragma link "sTrackBar"
#pragma link "sSkinManager"
#pragma link "sSkinProvider"
#pragma link "sLabel"
#pragma link "sTabControl"
#pragma resource "*.dfm"
TSettingsForm *SettingsForm;
//---------------------------------------------------------------------------
__declspec(dllimport)UnicodeString GetPluginUserDir();
__declspec(dllimport)UnicodeString GetThemeDir();
__declspec(dllimport)UnicodeString GetThemeSkinDir();
__declspec(dllimport)bool ChkSkinEnabled();
__declspec(dllimport)bool ChkThemeAnimateWindows();
__declspec(dllimport)bool ChkThemeGlowing();
__declspec(dllimport)int GetHUE();
__declspec(dllimport)int GetSaturation();
__declspec(dllimport)void LoadSettings();
__declspec(dllimport)void SetAlpha();
__declspec(dllimport)void SetAlphaEx(int Value);
bool aLoadSettingsExec = false;
//---------------------------------------------------------------------------
__fastcall TSettingsForm::TSettingsForm(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::WMTransparency(TMessage &Message)
{
  Application->ProcessMessages();
  if(sSkinManager->Active) sSkinProvider->BorderForm->UpdateExBordersPos(true,(int)Message.LParam);
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::FormCreate(TObject *Sender)
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
	  sSkinManager->SkinDirectory = ThemeSkinDir;
	  sSkinManager->SkinName = "Skin.asz";
	  //Ustawianie animacji AlphaControls
	  if(ChkThemeAnimateWindows()) sSkinManager->AnimEffects->FormShow->Time = 200;
	  else sSkinManager->AnimEffects->FormShow->Time = 0;
	  sSkinManager->Effects->AllowGlowing = ChkThemeGlowing();
	  //Zmiana kolorystyki AlphaControls
	  sSkinManager->HueOffset = GetHUE();
	  sSkinManager->Saturation = GetSaturation();
	  //Aktywacja skorkowania AlphaControls
	  sSkinManager->Active = true;
	}
	//Brak pliku zaawansowanej stylizacji okien
	else sSkinManager->Active = false;
  }
  //Zaawansowana stylizacja okien wylaczona
  else sSkinManager->Active = false;
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::FormShow(TObject *Sender)
{
  //Odczyt ustawien wtyczki
  aLoadSettings->Execute();
  //Wylaczenie przycisku
  SaveButton->Enabled = false;
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::aExitExecute(TObject *Sender)
{
  //Przywrocenie ewentualnych zmian w przezroczystosci
  SetAlpha();
  //Zamkniecie formy
  Close();
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::aLoadSettingsExecute(TObject *Sender)
{
  aLoadSettingsExec = true;
  //Odczyt ustawien wtyczki
  TIniFile *Ini = new TIniFile(GetPluginUserDir()+"\\\\AlphaWindows\\\\Settings.ini");
  sTrackBar->Position = Ini->ReadInteger("Settings","AlphaValue",30);
  //Sprawdzenie pliku konfiguracyjnego kompozycji
  Ini = new TIniFile(GetThemeDir()+"\\\\theme.ini");
  //Pre-konfigurowana przezroczystosc w kompozycji
  if(Ini->ValueExists("AlphaWindows","AlphaValue"))
   sTrackBar->Enabled = false;
  //Wartosc przezroczystosci zdefiniowana przez wtyczke
  else
   sTrackBar->Enabled = true;
  delete Ini;
  aLoadSettingsExec = false;
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::aSaveSettingsExecute(TObject *Sender)
{
  TIniFile *Ini = new TIniFile(GetPluginUserDir()+"\\\\AlphaWindows\\\\Settings.ini");
  Ini->WriteInteger("Settings","AlphaValue",sTrackBar->Position);
  delete Ini;
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::SaveButtonClick(TObject *Sender)
{
  //Zapis ustawien
  aSaveSettings->Execute();
  //Zmiana ustawien w rdzeniu wtyczki
  LoadSettings();
  //Wprowadzenie zmian w zycie
  SetAlpha();
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::OKButtonClick(TObject *Sender)
{
  //Wylaczenie buttona
  SaveButton->Enabled = false;
  //Zapis ustawien
  aSaveSettings->Execute();
  //Zmiana ustawien w rdzeniu wtyczki
  LoadSettings();
  //Wprowadzenie zmian w zycie
  SetAlpha();
  //Zamkniecie formy
  Close();
}
//---------------------------------------------------------------------------

void __fastcall TSettingsForm::sTrackBarChange(TObject *Sender)
{
  //Pozycja nie jest zmieniana przy wczytywaniu ustawien
  if(!aLoadSettingsExec)
  {
	//Wlaczenie przycisku
	SaveButton->Enabled = true;
	//Ustawienie wartosci w labelu
	ValueLabel->Caption = sTrackBar->Position;
	//Podgl¹d zmian na zywo
	SetAlphaEx(255-sTrackBar->Position);
  }
}
//---------------------------------------------------------------------------
