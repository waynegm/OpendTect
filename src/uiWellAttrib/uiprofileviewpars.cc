/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		March 2017
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiprofileviewpars.h"
#include "uibutton.h"
#include "uilistbox.h"
#include "uilabel.h"

#include "profileviewpars.h"
#include "stratlevel.h"

#   define mAddDrawBox(nm,txt) \
    draw##nm##sbox_ = new uiCheckBox( this, txt ); \

# define mFillDrawBox( nm ) \
    draw##nm##sbox_->setChecked( vwpars_.draw##nm##s_ );


uiProfileViewParsGrp::uiProfileViewParsGrp( uiParent* p, ProfileViewPars& pars )
    : uiGroup(p)
    , vwpars_(pars)
{
    uiLabel* lbl = new uiLabel( this, tr("Draw") );
    mAddDrawBox( ctrl, tr("Control profiles") );
    mAddDrawBox( well, uiStrings::sWells() );
    mAddDrawBox( marker, uiStrings::sMarker(mPlural) );
    mAddDrawBox( connection, tr("Marker connections") );
    mAddDrawBox( wellname, tr("Display well names") );
    mAddDrawBox( markername,tr("Display marker names") );
    mAddDrawBox( ctrlprofmrkrnm, tr("Display Marker Name at Control Profile") );

    drawctrlsbox_->attach( alignedBelow, lbl );
    drawctrlprofmrkrnmsbox_->attach( rightOf, drawctrlsbox_ );
    drawwellsbox_->attach( alignedBelow, drawctrlsbox_ );
    drawwellsbox_->activated.notify( mCB(this,uiProfileViewParsGrp,drwWllChg));

    drawwellnamesbox_->attach( alignedBelow, drawctrlprofmrkrnmsbox_ );
    drawwellnamesbox_->setSensitive( drawwellsbox_->isChecked() );
    drawmarkersbox_->attach( alignedBelow, drawwellsbox_ );
    drawmarkersbox_->activated.notify(
	    mCB(this,uiProfileViewParsGrp,drwMrkrChg));
    drawmarkernamesbox_->attach( alignedBelow, drawwellnamesbox_ );
    drawmarkernamesbox_->setSensitive( drawmarkersbox_->isChecked() );
    drawconnectionsbox_->attach( alignedBelow, drawmarkersbox_ );

    uiListBox::Setup su( OD::ChooseZeroOrMore, tr("Available Markers"),
			 uiListBox::AboveLeft );
    mrkrlistbox_ = new uiListBox( this, su, "Available Markers" );
    mrkrlistbox_->attach( alignedBelow, drawconnectionsbox_ );

    putToScreen();
}


void uiProfileViewParsGrp::drwWllChg( CallBacker* )
{
    const bool ischecked = drawwellsbox_->isChecked();
    drawwellnamesbox_->setChecked( ischecked );
    drawwellnamesbox_->setSensitive( ischecked );
    if ( !ischecked )
	drawmarkersbox_->setChecked( false );
}


void uiProfileViewParsGrp::drwMrkrChg( CallBacker* )
{
    const bool ischecked = drawmarkersbox_->isChecked();
    drawmarkernamesbox_->setChecked( ischecked );
    drawmarkernamesbox_->setSensitive( ischecked );
    if ( !ischecked )
    {
	drawctrlprofmrkrnmsbox_->setChecked( false );
	drawctrlprofmrkrnmsbox_->setSensitive( false );
    }
    else
	drawctrlprofmrkrnmsbox_->setSensitive( true );
}

#   define mReadDrawBox(nm) \
    vwpars_.draw##nm##s_ = draw##nm##sbox_->isChecked()

void uiProfileViewParsGrp::putToScreen()
{
    mFillDrawBox( ctrl );
    mFillDrawBox( well );
    mFillDrawBox( marker );
    mFillDrawBox( connection );
    mFillDrawBox( wellname );
    mFillDrawBox( markername );
    mFillDrawBox( ctrlprofmrkrnm );

    const ObjectSet<Strat::Level>& lvls = Strat::LVLS().levels();
    BufferStringSet lvlnmset;
    for ( int idx=0; idx<lvls.size(); idx++ )
	lvlnmset.add( lvls[idx]->name() );

    mrkrlistbox_->addItems( lvlnmset );
    if ( !vwpars_.selmrkrnms_.isEmpty() )
	mrkrlistbox_->setChosen( vwpars_.selmrkrnms_ );
    else
	mrkrlistbox_->chooseAll();
}


void uiProfileViewParsGrp::readFromScreen()
{
    mReadDrawBox( well );
    mReadDrawBox( ctrl );
    mReadDrawBox( marker );
    mReadDrawBox( connection );
    mReadDrawBox( ctrlprofmrkrnm );
    if ( vwpars_.drawwells_ )
	mReadDrawBox( wellname );
    else
	vwpars_.drawwellnames_ = false;
    if ( vwpars_.drawmarkers_ )
	mReadDrawBox( markername );
    else
	vwpars_.drawmarkernames_ = false;

    mrkrlistbox_->getChosen( vwpars_.selmrkrnms_ );
}


uiProfileViewParsDlgGrp::uiProfileViewParsDlgGrp( uiParent* p,
						  ProfileViewPars& pars )
    : uiDlgGroup(p,tr("Profile View"))
{
    viewparsgrp_ = new uiProfileViewParsGrp( this, pars );
}


void uiProfileViewParsDlgGrp::readFromScreen()
{
    viewparsgrp_->readFromScreen();
}


uiProfileViewParsDlg::uiProfileViewParsDlg( uiParent* p, ProfileViewPars& pars )
    : uiDialog(p,uiDialog::Setup(tr(""),tr(""),mNoHelpKey))
{
    viewparsgrp_ = new uiProfileViewParsGrp( this, pars );
}


bool uiProfileViewParsDlg::acceptOK( CallBacker* )
{
    viewparsgrp_->readFromScreen();
    return true;
}
