/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2000
________________________________________________________________________

-*/

#include "uistatusbar.h"
#include "uimainwin.h"
#include "uimain.h"
#include "uistrings.h"

#include "uibody.h"

#include <qstatusbar.h>
#include <qlabel.h>
#include <qtooltip.h>

mUseQtnamespace

uiStatusBar::uiStatusBar( uiMainWin* parnt, const char* nm )
    : uiSingleWidgetObject(parnt,nm)
    , statusbar_( new QStatusBar )
{
#ifndef __mac__ //TODO: Bugfix for gripper on Mac
    statusbar_->setSizeGripEnabled( false );
#endif

    setSingleWidget( statusbar_ );
}


uiStatusBar::~uiStatusBar()
{
}


int uiStatusBar::nrFields() const
{
    return statusbar_->children().size();
}


void uiStatusBar::setEmpty( int startat )
{
    const int nrflds = nrFields();
    for ( int idx=startat; idx<nrflds; idx++ )
	message( uiStrings::sEmptyString(), idx, -1 );
}


void uiStatusBar::message( const uiString& msg, int idx, int msecs )
{
    if ( labels_.validIdx(idx) && labels_[idx] )
        labels_[idx]->setText(msg.getQString());
    else if ( !msg.isEmpty() )
        statusbar_->showMessage( msg.getQString(), msecs<0?0:msecs );
    else
        statusbar_->clearMessage();
    
    uiMain::theMain().flushX();
}


void uiStatusBar::setBGColor( int idx, const Color& col )
{
    QWidget* widget = 0;
    if ( labels_.validIdx(idx) && labels_[idx] )
        widget = labels_[idx];
    else
        widget = statusbar_;
    
    const QColor qcol( col.r(),col.g(), col.b() );
    QPalette palette;
    palette.setColor( widget->backgroundRole(), qcol );
    widget->setPalette( palette );
}


QWidget* uiStatusBar::getWidget(int)
{ return statusbar_; }


Color uiStatusBar::getBGColor( int idx ) const
{
    const QWidget* widget = 0;
    if ( labels_.validIdx(idx) && labels_[idx] )
        widget = labels_[idx];
    else
        widget = statusbar_;
    
    const QBrush& qbr = widget->palette().brush( widget->backgroundRole() );
    const QColor& qc = qbr.color();
    return Color( qc.red(), qc.green(), qc.blue() );
}


int uiStatusBar::addMsgFld( const uiString& lbltxt, const uiString& tooltip,
			    OD::Alignment::HPos al, int stretch )
{
    QLabel* lbl = new QLabel( lbltxt.getQString() );
    int idx = labels_.size();
    labels_ += lbl;
    
    if ( !lbltxt.isEmpty() )
    {
        QLabel* txtlbl = new QLabel( lbltxt.getQString() );
        lbl->setBuddy( txtlbl );
        
        statusbar_->addWidget( txtlbl );
        txtlbl->setFrameStyle(QFrame::NoFrame);
    }
    
    statusbar_->addWidget( lbl, stretch );

    setToolTip( idx, tooltip );
    setTxtAlign( idx, al );

    return idx;
}

int uiStatusBar::addMsgFld( const uiString& tooltip,
			    OD::Alignment::HPos al, int stretch )
{
    return addMsgFld( uiStrings::sEmptyString(), tooltip, al, stretch );
}


bool uiStatusBar::addObject( uiObject* obj )
{
    if ( !obj )
	return false;
    QWidget* qw = obj->getWidget(0);
    if ( !qw )
	return false;

    statusbar_->addWidget( qw );
    return true;
}


void uiStatusBar::setToolTip( int idx, const uiString& tooltip )
{
    if ( !labels_.validIdx(idx) ) return;

    if ( !tooltip.isEmpty() )
	labels_[idx]->setToolTip( tooltip.getQString() );
}


void uiStatusBar::setTxtAlign( int idx, OD::Alignment::HPos hal )
{
    if ( !labels_.validIdx(idx) ) return;

    OD::Alignment al( hal );
    labels_[idx]->setAlignment( (Qt::Alignment)al.hPos() );
}


void uiStatusBar::setLabelTxt( int idx, const uiString& lbltxt )
{
    if ( !labels_.validIdx(idx) ) return;

    QLabel* lbl = dynamic_cast<QLabel*>(labels_[idx]->buddy());

    if ( lbl ) lbl->setText( lbltxt.getQString() );
}
