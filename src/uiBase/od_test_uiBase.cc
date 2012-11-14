/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "genc.h"

#include "uimain.h"

#include "uidialog.h"
#include "uibutton.h"
#include "uilabel.h"

#ifdef __msvc__
//#include "winmain.h"
#endif




int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    
    uiMain app( argc, argv );
    
    uiDialog dlg( uiDialog::Setup("Hello", "World", "NoHelpID" ));
    
    uiLabel* label1 = new uiLabel("Label text 1");
    dlg.addChild( label1 );
    
    uiLabel* label2 = new uiLabel("Label text 2");
    dlg.addChild( label2 );
    label2->attach(uiBaseObject::AlignedBelow, label1 );
    
    uiPushButton* button = new uiPushButton( &dlg, "Button", true );
    button->attach( uiBaseObject::AlignedBelow, label2 );
    dlg.go();
    
    
    return 0;
}
