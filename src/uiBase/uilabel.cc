/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          7/9/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uilabel.h"

#include "pixmap.h"

#include <qlabel.h> 

mUseQtnamespace

uiLabel::uiLabel( const char* txt, uiBaseObject* buddy )
    : uiBaseObject( txt )
    , qlabel_( new QLabel(QString(txt)) )
{
    if ( buddy )
    {
	if ( buddy->getNrWidgetRows()!=1 || buddy->getNrWidgetCols()!=1 )
	{
	    pErrMsg("Labels can only be attached to buddys with single widgets");
	    return;
	}
    
	qlabel_->setBuddy( buddy->getWidget(0,0) );
    }
    
    setTextSelectable( true );
}


void uiLabel::setText( const char* txt )
{ 
    qlabel_->setText( QString(txt) );
    setName( txt );
}


const char* uiLabel::text() const
{
    static BufferString txt;
    txt = mQStringToConstChar( qlabel_->text() );
    return txt.buf();
}


void uiLabel::setTextSelectable( bool yn ) 
{
    qlabel_->setTextInteractionFlags( yn ? Qt::TextSelectableByMouse :
					 Qt::NoTextInteraction );
}


void uiLabel::setPixmap( const ioPixmap& pixmap )
{
    qlabel_->setPixmap( *pixmap.qpixmap() );
}


void uiLabel::setAlignment( Alignment::HPos hal )
{
    Alignment al( hal, Alignment::VCenter );
    qlabel_->setAlignment( (Qt::AlignmentFlag)al.uiValue() );
}


Alignment::HPos uiLabel::alignment() const
{
    Alignment al;
    al.setUiValue( (int)qlabel_->alignment() );
    return al.hPos();
}


QWidget* uiLabel::getWidget(int, int)
{ return qlabel_; }
