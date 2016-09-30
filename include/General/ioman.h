#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		3-8-1995
________________________________________________________________________

-*/

#include "dbman.h"

mDeprecated typedef DBMan IOMan;
mGlobal(General) mDeprecated inline DBMan&	IOM()	{ return DBM(); }
