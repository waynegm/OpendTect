#ifndef uiicons_h
#define uiicons_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "commondefs.h"

class ioPixmap;

namespace uiIcon
{
    mGlobal(uiBase) const char*		save()		{ return "save"; }
    mGlobal(uiBase) const char*		saveAs()	{ return "saveas"; }
    mGlobal(uiBase) const char*		openObject()	{ return "openstorage";}
    mGlobal(uiBase) const char*		newObject()	{ return "newstorage"; }
    mGlobal(uiBase) const char*		removeObject()	{ return "trashcan"; }

    mGlobal(uiBase) const char*		None()		{ return "-"; }
					//!< Avoids pErrMsg
};


#endif

