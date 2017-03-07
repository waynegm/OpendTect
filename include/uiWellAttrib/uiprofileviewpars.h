#ifndef uiprofileviewparsgrp_h
#define uiprofileviewparsgrp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2017
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uigroup.h"
#include "uidialog.h"
#include "uidlggroup.h"

class uiCheckBox;
class uiListBox;

class ProfileViewPars;

mExpClass(uiWellAttrib) uiProfileViewParsGrp : public uiGroup
{ mODTextTranslationClass(uiProfileViewParsGrp);
public:
				uiProfileViewParsGrp(
					uiParent*, ProfileViewPars&);
    void			putToScreen();
    void			readFromScreen();
protected:

    ProfileViewPars&		vwpars_;

    uiCheckBox*			drawwellsbox_;
    uiCheckBox*			drawctrlsbox_;
    uiCheckBox*			drawmarkersbox_;
    uiCheckBox*			drawconnectionsbox_;
    uiCheckBox*			drawwellnamesbox_;
    uiCheckBox*			drawmarkernamesbox_;
    uiCheckBox*			drawctrlprofmrkrnmsbox_;
    uiListBox*			mrkrlistbox_;

    void			drwWllChg(CallBacker*);
    void			drwMrkrChg(CallBacker*);

};


mExpClass(uiWellAttrib) uiProfileViewParsDlgGrp : public uiDlgGroup
{ mODTextTranslationClass(uiProfileViewParsDlgGrp);
public:
				uiProfileViewParsDlgGrp(
					uiParent*, ProfileViewPars&);
    void			readFromScreen();
protected:
    uiProfileViewParsGrp*	viewparsgrp_;
};


mExpClass(uiWellAttrib) uiProfileViewParsDlg : public uiDialog
{ mODTextTranslationClass(uiProfileViewParsDlg);
public:
				uiProfileViewParsDlg(
					uiParent*, ProfileViewPars&);
protected:

    uiProfileViewParsGrp*	viewparsgrp_;

    bool			acceptOK(CallBacker*);
};

#endif
