/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uivismod.h"
#include "moddepmgr.h"
#include "visdata.h"
#include "uimain.h"

mDefModInitFn(uiVis)
{
    mIfNotFirstTime( return );

    visBase::DataObject::setVisualizationThread( uiMain::theMain().thread() );
}
