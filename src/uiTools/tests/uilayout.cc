/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uilayout.h"
#include "testprog.h"

#include "uidialog.h"
#include "uilabel.h"
#include "uigroup.h"
#include "uimsg.h"

#include "uimain.h"

static bool showdlg = false;
static bool res = true;

bool testLayout( uiMain& app )
{
    
    uiDialog* dlg = new uiDialog( 0,
        uiDialog::Setup( toUiString("Test dialog"), mNoDlgTitle, mNoHelpKey ));
    
    uiLabel* label1 = new uiLabel( dlg, toUiString("Label 1" ));
    uiLabel* label2 = new uiLabel( dlg, toUiString("Label 2" ));
    label2->attach( alignedBelow, label1 );
 /*
    const uiLayoutMgr* mgr = dlg.getLayoutMgr();
    		    
    RowCol origin, span;
    mRunStandardTest( mgr->computeLayout(label1, origin, span ) &&
                      origin==RowCol(0,0) && span==RowCol(1,1),
                     "Label 1 layout" );
    
    mRunStandardTest( mgr->computeLayout(label2, origin, span ) &&
                     origin==RowCol(1,0) && span==RowCol(1,1),
                     "Label 1 layout" );
    */
    if ( showdlg )
    {
        app.setTopLevel( dlg );
        dlg->show();
    }
    
  
    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();
    
    showdlg = clparser.hasKey("show");
    
    uiMain app( argc, argv );

    if ( !testLayout( app ) )
        ExitProgram( 1 );

    return ExitProgram( 0 );
}
