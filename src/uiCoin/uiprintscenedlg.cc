/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          October 2002
 RCS:           $Id: uiprintscenedlg.cc,v 1.9 2004-01-30 10:22:19 bert Exp $
________________________________________________________________________

-*/

#include "uiprintscenedlg.h"
#include "uifileinput.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "ptrman.h"
#include "filegen.h"
#include "uiobj.h"
#include "uilabel.h"

#include "Inventor/SoOffscreenRenderer.h"

uiPrintSceneDlg::uiPrintSceneDlg( uiParent* p, SoNode* scene_ )
	: uiDialog(p, uiDialog::Setup("Print Scene",
		    		      "Enter filename and fileformat","50.0.1"))
	, scene(scene_)
    	, horwidthfld(0)
{
    PtrMan<SoOffscreenRenderer> r =
	new SoOffscreenRenderer(*(new SbViewportRegion));
    const int nrfiletypes = r->getNumWriteFiletypes();
    BufferStringSet filetypes;
    for ( int idx=0; idx<nrfiletypes; idx++ )
    {
	SbList<SbName> extlist;
	SbString fullname, description;
	r->getWriteFiletypeInfo( idx, extlist, fullname, description );
	filetypes.addIfNew( fullname.getString() );
    }

    if ( filetypes.size() == 0 )
    {
	new uiLabel( this,
	    "No output file types found.\n"
	    "Probably, there is no valid 'libsimage.so' installed." );
	return;
    }

    horwidthfld = new uiGenInput( this, "Size", IntInpSpec(800) );
    horwidthfld->setElemSzPol( uiObject::small );

    vertwidthfld = new uiGenInput( this, "x", IntInpSpec(600) );
    vertwidthfld->setElemSzPol( uiObject::small );
    vertwidthfld->attach( rightOf, horwidthfld );

    widthunitfld = new uiComboBox( this, "Unit" );
    widthunitfld->addItem("pixels");
    widthunitfld->addItem("cm");
    widthunitfld->addItem("inches");
    widthunitfld->attach( rightOf, vertwidthfld );

    resolutionfld = new uiGenInput( this, "Resolution (dpi)", IntInpSpec(300) );
    resolutionfld->setElemSzPol( uiObject::small );
    resolutionfld->attach( alignedBelow, horwidthfld );

    filetypesfld = new uiComboBox(this, "Filetypes" );
    filetypesfld->attach( alignedBelow, resolutionfld );
    for ( int idx=0; idx<filetypes.size(); idx++ )
	filetypesfld->addItem( filetypes.get(idx) );

    fileinputfld = new uiFileInput( this, "Select filename",
				    uiFileInput::Setup().forread(false) );
    BufferString datadirnm( GetDataDir() );
    BufferString dirnm = File_getFullPath( datadirnm, "Misc" );
    fileinputfld->setDefaultSelectionDir( dirnm );
    fileinputfld->attach( alignedBelow, filetypesfld );

    finaliseStart.notify( mCB(this,uiPrintSceneDlg,doFinalise) );
}


void uiPrintSceneDlg::doFinalise( CallBacker* )
{
} 


bool uiPrintSceneDlg::acceptOK( CallBacker* )
{
    if ( !horwidthfld ) return true;

    double horwidth = horwidthfld->getValue();
    double vertwidth = vertwidthfld->getValue();
    int widthunit = widthunitfld->currentItem();

    double resolution = resolutionfld->getValue();

    if ( widthunit==1 )
    {
	horwidth *= resolution/2.54;
	vertwidth *= resolution/2.54;
    }
    else if ( widthunit==2 )
    {
	horwidth *= resolution;
	vertwidth *= resolution;
    }
    
    SbViewportRegion viewport;
    SbVec2s maxres = SoOffscreenRenderer::getMaximumResolution();
    if ( horwidth>maxres[0] ) horwidth = maxres[0];
    if ( vertwidth>maxres[1] ) vertwidth = maxres[1];

    viewport.setWindowSize( SbVec2s( mNINT(horwidth), mNINT(vertwidth) ));
    viewport.setPixelsPerInch( resolution );

    PtrMan<SoOffscreenRenderer> r = new SoOffscreenRenderer(viewport);

    const char* filename = fileinputfld->fileName();
    if ( !filename || !filename[0] )
    { return false; }

    int filetypenr = filetypesfld->currentItem();

    SbList<SbName> extlist;
    SbString fullname, description;
    r->getWriteFiletypeInfo(filetypenr, extlist, fullname, description);

    if ( !r->render( scene ) )
	return false;

    if ( !r->writeToFile( filename, extlist[0].getString() ) )
    {
	uiMSG().error( "Couldn't write to specified file" );
	return false;
    }
    return true;
}
