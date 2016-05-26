/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
________________________________________________________________________

-*/


#include "uisplitter.h"

#include "uigroup.h"

#include <QSplitter>

mUseQtnamespace


uiSplitter::uiSplitter( uiParent* p, const char* txt, bool hor )
    : uiSingleWidgetObject(p, txt )
    , qsplitter_( new QSplitter( getParentWidget(p)))
{
    qsplitter_->setOrientation( hor ? Qt::Horizontal : Qt::Vertical );
    setStretch( 2, 2 );
}


void uiSplitter::addGroup( uiGroup* grp )
{
    if ( grp )
	qsplitter_->addWidget( grp->getWidget(0) );
}
