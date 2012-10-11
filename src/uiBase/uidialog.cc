/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          31/05/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uidialog.h"

#include <QDialog>


uiWindowBase::uiWindowBase( const char* nm )
    : uiGroup( nm, 0 )
{}

    
void uiWindowBase::setWindowTitle( const char* txt )
{ getWindow()->setWindowTitle(txt); }

void uiWindowBase::showMinMaxButtons()
{
    mQtclass(Qt)::WindowFlags flags = getWindow()->windowFlags();
    flags |= mQtclass(Qt)::WindowMinMaxButtonsHint;
    getWindow()->setWindowFlags( flags );
}


void uiWindowBase::showAlwaysOnTop()
{
    mQtclass(Qt)::WindowFlags flags = getWindow()->windowFlags();
    flags |= mQtclass(Qt)::WindowStaysOnTopHint;
    getWindow()->setWindowFlags( flags );
}


Threads::Mutex uiWindowBase::windowlistlock_;
ObjectSet<uiWindowBase>	uiWindowBase::windowlist_;


void uiWindowBase::addWindow( uiWindowBase* win )
{
    Threads::MutexLocker lock( windowlistlock_ );
    if ( !windowlist_.isPresent( win ) )
	windowlist_ += win;
}


void uiWindowBase::removeWindow( uiWindowBase* win )
{
    Threads::MutexLocker lock( windowlistlock_ );
    windowlist_ -= win;
}


uiDialog::uiDialog( const uiDialog::Setup& s )
    : uiWindowBase( s.wintitle_ )
    , setup_( s )
{
    qdialog_ = new QDialog;
    widget_ = new QWidget( qdialog_ );
    
    setWindowTitle( s.wintitle_ );
    //setTitleText( s.dlgtitle_ );
    ctrlstyle_ = DoAndLeave;
}

 /*
void uiDialog::setButtonText( Button but, const char* txt )
{
   
    switch ( but )
    {
        case OK		: setOkText( txt ); break;
        case CANCEL	: setCancelText( txt ); break;
        case SAVE	: enableSaveButton( txt ); break;
        case HELP	: pErrMsg("set help txt but"); break;
        case CREDITS: pErrMsg("set credits txt but");
        case TRANSLATE: pErrMsg("set transl txt but");
    }
  
}
     */


QWidget* uiDialog::getWindow()
{ return qdialog_; }


void uiDialog::setCtrlStyle( uiDialog::CtrlStyle cs )
{
    /*
    switch ( cs )
    {
    case DoAndLeave:
	setOkText( "&Ok" );
	setCancelText( "&Cancel" );
    break;
    case DoAndStay:
	setOkText( "&Go" );
	setCancelText( "&Dismiss" );
    break;
    case LeaveOnly:
	setOkText( mBody->finalised() ? "&Dismiss" : "" );
	setCancelText( "&Dismiss" );
    break;
    case DoAndProceed:
	setOkText( "&Proceed" );
	setCancelText( "&Dismiss" );
    break;
    }

     */
    ctrlstyle_ = cs;
}

/*
bool uiDialog::haveCredits() const
{
    return HelpViewer::hasSpecificCredits( helpID() );
}
 */


int uiDialog::go()
{
    addWindow( this );
    return qdialog_->exec();
}

/*
void uiDialog::setOkText( const char* txt )	{ mBody->setOkText(txt); }
void uiDialog::setCancelText( const char* txt )	{ mBody->setCancelText(txt);}
void uiDialog::enableSaveButton(const char* t)  { mBody->enableSaveButton(t); }
uiButton* uiDialog::button(Button b)		{ return mBody->button(b); }
void uiDialog::setSeparator( bool yn )		{ mBody->setSeparator(yn); }
bool uiDialog::separator() const		{ return mBody->separator(); }
void uiDialog::setHelpID( const char* id )	{ mBody->setHelpID(id); }
const char* uiDialog::helpID() const		{ return mBody->helpID(); }
int uiDialog::uiResult() const			{ return mBody->uiResult(); }
void uiDialog::setModal( bool yn )		{ mBody->setModal( yn ); }
bool uiDialog::isModal() const			{ return mBody->isModal(); }

void uiDialog::setButtonSensitive(uiDialog::Button b, bool s ) 
    { mBody->setButtonSensitive(b,s); }
void uiDialog::setSaveButtonChecked(bool b) 
    { mBody->setSaveButtonChecked(b); }
bool uiDialog::saveButtonChecked() const
    { return mBody->saveButtonChecked(); }
bool uiDialog::hasSaveButton() const
    { return mBody->hasSaveButton(); }
void uiDialog::setCaption( const char* txt )
    { caption_ = txt; mBody->setWindowTitle( txt ); }
*/