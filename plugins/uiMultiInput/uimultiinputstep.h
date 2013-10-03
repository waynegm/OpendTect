#ifndef uimultiinputstep_h
#define uimultiinputstep_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		February 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uimultiinputmod.h"
#include "uivolprocstepdlg.h"
#include "multiinputstep.h"

class uiFileInput;
class uiPushButton;
class uiTable;

namespace VolProc
{

mExpClass(uiMultiInput) uiMultiInputStep : public uiStepDialog
{
public:
        mDefaultFactoryInstanciationBase(
		VolProc::MultiInputStep::sFactoryKeyword(),
		VolProc::MultiInputStep::sFactoryDisplayName())
		mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );

protected:

    			uiMultiInputStep(uiParent*,MultiInputStep*);
    static uiStepDialog* createInstance(uiParent*,Step*);

    bool		acceptOK(CallBacker*);
};

} // namespace VolProc

#endif
