//---------------------------------------------------------------------------
#ifndef SettingsFrmH
#define SettingsFrmH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include "sBevel.hpp"
#include <Vcl.ExtCtrls.hpp>
#include "sPageControl.hpp"
#include <Vcl.ComCtrls.hpp>
#include "sTrackBar.hpp"
#include "sSkinManager.hpp"
#include "sSkinProvider.hpp"
#include <System.Actions.hpp>
#include <Vcl.ActnList.hpp>
#include "sLabel.hpp"
#include "sTabControl.hpp"
//---------------------------------------------------------------------------
class TSettingsForm : public TForm
{
__published:	// IDE-managed Components
	TsBevel *Bevel;
	TButton *OKButton;
	TButton *CancelButton;
	TButton *SaveButton;
	TsTrackBar *sTrackBar;
	TsSkinManager *sSkinManager;
	TActionList *ActionList;
	TAction *aExit;
	TAction *aLoadSettings;
	TAction *aSaveSettings;
	TsLabel *MoreLabel;
	TsTabControl *sTabControl;
	TsLabel *LessLabel;
	TsLabel *AlphaLabel;
	TsLabel *ValueLabel;
	TsSkinProvider *sSkinProvider;
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall aExitExecute(TObject *Sender);
	void __fastcall aLoadSettingsExecute(TObject *Sender);
	void __fastcall aSaveSettingsExecute(TObject *Sender);
	void __fastcall SaveButtonClick(TObject *Sender);
	void __fastcall OKButtonClick(TObject *Sender);
	void __fastcall sTrackBarChange(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TSettingsForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TSettingsForm *SettingsForm;
//---------------------------------------------------------------------------
#endif
