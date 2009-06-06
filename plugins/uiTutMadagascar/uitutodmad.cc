/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          June  2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uitutodmad.cc,v 1.2 2009-06-06 12:58:09 cvshelene Exp $";

#include "uitutodmad.h"

#include "filegen.h"
#include "madio.h"
#include "madstream.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "uifileinput.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewstdcontrol.h"
#include "uigeninput.h"
#include "uimsg.h"

uiTutODMad::uiTutODMad( uiParent* p )
    : uiDialog( p, Setup("Display Madagascar data dialog",
			 "Specify data settings", mNoHelpID) )
    , iop_("Madagascar display parameters")
{
    uiFileInput::Setup setup;
    setup.defseldir( ODMad::FileSpec::defPath() );
    setup.forread( true );
    maddatafld_ = new uiFileInput( this, "Data file", setup );
    maddatafld_->setFilter( "*.rsf" );

    dowigglesfld_ = new uiGenInput ( this, "Display",
			     BoolInpSpec(true,"Wiggles","Variable density") );
    dowigglesfld_->attach( alignedBelow, maddatafld_ );
}


bool uiTutODMad::acceptOK( CallBacker* )
{
    iop_.clear();
    iop_.set( IOPar::compKey("Input",sKey::Type), ODMad::sKeyMadagascar() );
    const BufferString fnm = maddatafld_->fileName();
    if ( fnm.isEmpty() || !File_exists(fnm) )
    {
	uiMSG().error( "Please select an existing input file" );
	return false;
    }

    iop_.set( IOPar::compKey("Input",sKey::FileName), fnm );
    ODMad::MadStream madstream_( iop_ );
    if ( !madstream_.isOK() )
    {
	uiMSG().error( madstream_.errMsg() );
	return false;
    }

    const int trcsize = madstream_.getNrSamples();
    float* arr = new float[trcsize];
    madstream_.writeTraces( false );

    Seis::GeomType geom = madstream_.is2D()
			    ? madstream_.isPS() ? Seis::LinePS : Seis::Line
			    : madstream_.isPS() ? Seis::VolPS : Seis::Vol;
    SeisTrcBuf* trcbuf = madstream_.getTrcBuf();
    SeisTrcInfo::Fld seisifld = (SeisTrcInfo::Fld)0;
    if ( trcbuf && trcbuf->get(0) && trcbuf->get(1) )
	seisifld = (SeisTrcInfo::Fld) trcbuf->get(0)->info().
	    			getDefaultAxisFld(geom,&trcbuf->get(1)->info());
    bufdtpack_ = new SeisTrcBufDataPack( trcbuf, geom, seisifld,
	    				 "Madagascar data" );
    bufdtpack_->trcBufArr2D().setBufMine( true );
    madstream_.setStorBufMine( false );
    DPM(DataPackMgr::FlatID()).add( bufdtpack_ );
    createAndDisplay2DViewer();
    return true;

}


void uiTutODMad::createAndDisplay2DViewer()
{
    uiFlatViewMainWin* fvwin = new uiFlatViewMainWin( 0, 
		    uiFlatViewMainWin::Setup("Madagascar data",true) );
    uiFlatViewer& vwr = fvwin->viewer();
    bool dowiggles = dowigglesfld_->getBoolValue();
    vwr.setPack( dowiggles, bufdtpack_->id(), false, true );
    FlatView::Appearance& app = vwr.appearance();
    app.annot_.setAxesAnnot( true );
    app.setDarkBG( false );
    app.setGeoDefaults( true );
    vwr.setInitialSize( uiSize(600,400) );
    fvwin->addControl(
	    new uiFlatViewStdControl(vwr,uiFlatViewStdControl::Setup(fvwin)) );
    fvwin->show();
}
