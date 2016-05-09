/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Arend Lammertink
 Date:		2001
________________________________________________________________________

-*/


#include "uiprogressbar.h"

#include	<QProgressBar>

mUseQtnamespace



uiProgressBar::uiProgressBar( uiParent* p, const char* txt, 
			      int totsteps, int progr )
    : uiSingleWidgetObject(p,txt)
    , body_( new QProgressBar( getParentWidget(p) ) )
{
    setSingleWidget( body_ );
    
    setStretch( 1, 0 );
    setHSzPol( uiObject::MedVar );
    
    setProgress( progr );
    setTotalSteps( totsteps );
    
}


void uiProgressBar::setProgress( int progr )
{ body_->setValue( progr ); } 

int uiProgressBar::progress() const
{ return body_->value(); }

void uiProgressBar::setTotalSteps( int tstp )
{ body_->setMaximum( tstp > 1 ? tstp : 1 ); } 

int uiProgressBar::totalSteps() const
{ return body_->maximum(); }
