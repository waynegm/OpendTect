/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: inituibase.cc 26544 2012-10-01 10:16:29Z ranojay.sen@dgbes.com $";

#include "moddepmgr.h"
#include "uiosgfont.h"

mDefModInitFn(uiCoin)
{
    mIfNotFirstTime( return );

    uiOsgFontCreator::initClass();
}
