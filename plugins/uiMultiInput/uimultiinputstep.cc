/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uimultiinputstep.h"

#include "uimsg.h"
#include "uitable.h"

#include "multiinputstep.h"

namespace VolProc
{

uiStepDialog* uiMultiInputStep::createInstance( uiParent* p, Step* step )
{
    mDynamicCastGet(MultiInputStep*,ms,step);
    return ms ? new uiMultiInputStep(p,ms) : 0; 
}

uiMultiInputStep::uiMultiInputStep( uiParent* p, MultiInputStep* step )
    : uiStepDialog(p,MultiInputStep::sFactoryDisplayName(), step )
{
    addMultiInputFld();
    addNameFld( multiinpfld_ );
}


bool uiMultiInputStep::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK(cb) )
	return false;

    return true;
}

} // namespace VolProc
