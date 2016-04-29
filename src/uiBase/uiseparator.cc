/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/1/2001
________________________________________________________________________

-*/


#include "uiseparator.h"

#include <QFrame>

mUseQtnamespace

uiSeparator::uiSeparator( uiParent* p, const char* txt,
			  OD::Orientation ori, bool raised )
    : uiObject(p,txt)
    , qframe_(new QFrame)
{
    qframe_->setFrameStyle(
	(ori==OD::Horizontal ? QFrame::HLine : QFrame::VLine) |
	(raised ? QFrame::Raised : QFrame::Sunken) );
    qframe_->setLineWidth( 1 );
    qframe_->setMidLineWidth( 0 );
}


void uiSeparator::setRaised( bool yn )
{
    qframe_->setFrameShadow( yn ? QFrame::Raised : QFrame::Sunken );
}
