/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimpeman.h"

#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emundo.h"
#include "executor.h"
#include "horizon2dseedpicker.h"
#include "horizon3dseedpicker.h"
#include "keyboardevent.h"
#include "mpeengine.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "seisdatapack.h"
#include "seispreload.h"
#include "selector.h"
#include "survinfo.h"
#include "keyboardevent.h"

#include "uicombobox.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uivispartserv.h"
#include "uimain.h"
#include "visemobjdisplay.h"
#include "vishorizondisplay.h"
#include "visrandomtrackdisplay.h"
#include "vismpe.h"
#include "vismpeseedcatcher.h"
#include "visselman.h"
#include "vistransform.h"
#include "vistransmgr.h"

using namespace MPE;

uiMPEMan::uiMPEMan( uiParent* p, uiVisPartServer* ps )
    : parent_(p)
    , clickcatcher_(0)
    , clickablesceneid_(-1)
    , visserv_(ps)
    , seedpickwason_(false)
    , oldactivevol_(false)
    , mpeintropending_(false)
    , cureventnr_(mUdf(int))
    , polyselstoppedseedpick_(false)
{
    engine().trackeraddremove.notify(
			mCB(this,uiMPEMan,trackerAddedRemovedCB) );
    SurveyInfo& si = const_cast<SurveyInfo&>( SI() );
    si.workRangeChg.notify( mCB(this,uiMPEMan,workAreaChgCB) );
    visBase::DM().selMan().selnotifier.notify(
	    mCB(this,uiMPEMan,treeItemSelCB) );
    visserv_->mouseEvent.notify( mCB(this,uiMPEMan,mouseEventCB) );
    visserv_->keyEvent.notify( mCB(this,uiMPEMan,keyEventCB) );
}


uiMPEMan::~uiMPEMan()
{
    deleteVisObjects();
    engine().trackeraddremove.remove(
			mCB(this,uiMPEMan,trackerAddedRemovedCB) );
    SurveyInfo& si = const_cast<SurveyInfo&>( SI() );
    si.workRangeChg.remove( mCB(this,uiMPEMan,workAreaChgCB) );
    visBase::DM().selMan().selnotifier.remove(
	    mCB(this,uiMPEMan,treeItemSelCB) );
    visserv_->mouseEvent.remove( mCB(this,uiMPEMan,mouseEventCB) );
    visserv_->keyEvent.remove( mCB(this,uiMPEMan,keyEventCB) );
}


static const int sStart = 0;
static const int sRetrack = 1;
static const int sStop = 2;
static const int sPoly = 3;
static const int sChild = 4;
static const int sParent = 5;
static const int sParPath = 6;
static const int sClear = 7;
static const int sDelete = 8;
static const int sUndo = 9;
static const int sRedo = 10;
static const int sLock = 11;
static const int sUnlock = 12;
static const int sSave = 13;
static const int sSaveAs = 14;
static const int sRest = 15;
static const int sFull = 16;
static const int sSett = 17;


void uiMPEMan::keyEventCB( CallBacker* )
{
    if ( MPE::engine().nrTrackersAlive() == 0 ) return;

    int action = -1;
    const KeyboardEvent& kev = visserv_->getKeyboardEvent();

    if ( KeyboardEvent::isUnDo(kev) )
	undo();
    else if ( KeyboardEvent::isReDo(kev) )
	redo();
    else if ( kev.key_ == OD::K )
    {
	if ( MPE::engine().trackingInProgress() )
	    action = sStop;
	else
	{
	    if ( OD::ctrlKeyboardButton(kev.modifier_) )
		action = sRetrack;
	    else if ( kev.modifier_==OD::NoButton )
		action = sStart;
	}
    }
    else if ( kev.key_ == OD::Y )
    {
	action = sPoly;
    }
    else if ( kev.key_ == OD::A )
	action = sClear;
    else if ( kev.key_==OD::D || kev.key_==OD::Delete )
	action = sDelete;
    else if ( kev.key_ == OD::L )
	action = sLock;
    else if ( kev.key_ == OD::U )
	action = sUnlock;
    else if ( kev.key_ == OD::S && OD::ctrlKeyboardButton(kev.modifier_) )
	action = sSave;
    else if ( kev.key_ == OD::S && OD::ctrlKeyboardButton(kev.modifier_)
				&& OD::shiftKeyboardButton(kev.modifier_) )
	action = sSaveAs;

    if ( action != -1 )
	handleAction( action );
}


#define mAddAction(txt,sc,id,enab) \
{ \
    uiAction* action = new uiAction( txt ); \
    mnu.insertAction( action, id ); \
    action->setEnabled( enab ); \
}

void uiMPEMan::mouseEventCB( CallBacker* )
{
    if ( MPE::engine().nrTrackersAlive() == 0 ) return;

    const MouseEvent& mev = visserv_->getMouseEvent();
    if ( mev.ctrlStatus() && mev.rightButton() && !mev.isPressed() )
    {
	const int res = popupMenu();
	handleAction( res );
	visserv_->setSceneEventHandled();
    }
}


int uiMPEMan::popupMenu()
{
    EM::Horizon3D* hor3d = getSelectedHorizon3D();
    if ( !hor3d ) return -1;

    visSurvey::HorizonDisplay* hd = getSelectedDisplay();
    visSurvey::Scene* scene = hd ? hd->getScene() : 0;
    if ( !scene ) return -1;

    uiMenu mnu( tr("Tracking Menu") );
    const bool istracking = MPE::engine().trackingInProgress();
    if ( istracking )
	mAddAction( tr("Stop Tracking"), "k", sStop, true )
    else
    {
	const Coord3& clickedpos = scene->getMousePos( true );
	const bool haspos = !clickedpos.isUdf();
	mAddAction( tr("Start Tracking"), "k", sStart, true )
	mAddAction( tr("Retrack From Seeds"), "ctrl+k", sRetrack, true )
	mAddAction( tr("Define Polygon"), "y", sPoly, true )
	if ( haspos )
	{
	    mAddAction( tr("Select Children"), "", sChild, true )
	    mAddAction( tr("Select Parents"), "", sParent, true )
	    mAddAction( tr("Show Parents Path"), "", sParPath, true )
	}
	mAddAction( tr("Clear Selection"), "a", sClear, true )
	mAddAction( tr("Delete Selected"), "d", sDelete, true )
	mAddAction( tr("Undo"), "ctrl+z", sUndo, EM::EMM().undo().canUnDo())
	mAddAction( tr("Redo"), "ctrl+y", sRedo, EM::EMM().undo().canReDo())
	mAddAction( tr("Lock"), "l", sLock, true )
	mAddAction( tr("Unlock"), "u", sUnlock, true )
	mAddAction( tr("Save"), "ctrl+s", sSave, hor3d->isChanged() )
	mAddAction( tr("Save As ..."), "ctrl+shift+s", sSaveAs, true )
	if ( !hd->getOnlyAtSectionsDisplay() )
	    mAddAction( tr("Display Only at Sections"), "r", sRest, true )
	else
	    mAddAction( tr("Display in Full"), "r", sFull, true )
	mAddAction( tr("Show Settings ..."), "", sSett, true )
    }

    return mnu.exec();
}


void uiMPEMan::handleAction( int res )
{
    MPE::EMTracker* tracker = getSelectedTracker();
    EM::EMObject* emobj =
		tracker ? EM::EMM().getObject(tracker->objectID()) : 0;
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)
    if ( !hor3d ) return;

    visSurvey::HorizonDisplay* hd = getSelectedDisplay();
    visSurvey::Scene* scene = hd ? hd->getScene() : 0;
    if ( !scene ) return;

    const Coord3& clickedpos = scene->getMousePos( true );
    const TrcKey tk = SI().transform( clickedpos.coord() );

    switch ( res )
    {
    case sStart: startTracking(); break;
    case sRetrack: startRetrack(); break;
    case sStop: stopTracking(); break;
    case sPoly: changePolySelectionMode(); break;
    case sChild: hd->selectChildren(tk); break;
    case sParent: hd->selectParent(tk); break;
    case sParPath: showParentsPath(); break;
    case sClear: clearSelection(); break;
    case sDelete: deleteSelection(); break;
    case sUndo: undo(); break;
    case sRedo: redo(); break;
    case sLock: hor3d->lockAll(); break;
    case sUnlock: hor3d->unlockAll(); break;
    case sSave: visserv_->storeEMObject( false ); break;
    case sSaveAs: visserv_->storeEMObject( true ); break;
    case sRest: hd->setOnlyAtSectionsDisplay( true ); break;
    case sFull: hd->setOnlyAtSectionsDisplay( false ); break;
    case sSett: showSetupDlg(); break;

    default:
	break;
    }
}


void uiMPEMan::startTracking()
{
    uiString errmsg;
    if ( !MPE::engine().startTracking(errmsg) && !errmsg.isEmpty() )
	uiMSG().error( errmsg );
}


void uiMPEMan::startRetrack()
{
    uiString errmsg;
    if ( !MPE::engine().startRetrack(errmsg) && !errmsg.isEmpty() )
	uiMSG().error( errmsg );
}


void uiMPEMan::stopTracking()
{
    MPE::engine().stopTracking();
}


void uiMPEMan::deleteVisObjects()
{
    if ( clickcatcher_ )
    {
	if ( clickablesceneid_>=0 )
	    visserv_->removeObject( clickcatcher_->id(), clickablesceneid_ );

	clickcatcher_->click.remove( mCB(this,uiMPEMan,seedClick) );
	clickcatcher_->setEditor( 0 );
	clickcatcher_->unRef();
	clickcatcher_ = 0;
	clickablesceneid_ = -1;
    }
}


#define mSeedClickReturn() \
{ endSeedClickEvent(emobj);  return; }

void uiMPEMan::seedClick( CallBacker* )
{
    EM::EMObject* emobj = 0;
    MPE::Engine& engine = MPE::engine();
    if ( engine.trackingInProgress() )
	mSeedClickReturn();

    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
	mSeedClickReturn();

    emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj )
	mSeedClickReturn();

    while ( emobj->hasBurstAlert() )
	emobj->setBurstAlert( false );

    const int trackerid =
		MPE::engine().getTrackerByObject( tracker->objectID() );

    const int clickedobject = clickcatcher_->info().getObjID();
    if ( clickedobject == -1 )
	mSeedClickReturn();

    if ( !clickcatcher_->info().isLegalClick() )
    {
	visBase::DataObject* dataobj = visserv_->getObject( clickedobject );
	mDynamicCastGet( visSurvey::RandomTrackDisplay*, randomdisp, dataobj );

	if ( tracker->is2D() && !clickcatcher_->info().getObjLineName() )
	    uiMSG().error( tr("2D tracking cannot handle picks on 3D lines.") );
	else if ( !tracker->is2D() && clickcatcher_->info().getObjLineName() )
	    uiMSG().error( tr("3D tracking cannot handle picks on 2D lines.") );
	else if ( randomdisp )
	    uiMSG().error( emobj->getTypeStr(),
			   tr("Tracking cannot handle picks on random lines."));
	else if ( clickcatcher_->info().getObjCS().nrZ()==1 &&
		  !clickcatcher_->info().getObjCS().isEmpty() )
	    uiMSG().error( emobj->getTypeStr(),
			   tr("Tracking cannot handle picks on time slices.") );
	mSeedClickReturn();
    }

    const EM::PosID pid = clickcatcher_->info().getNode();
    TrcKeyZSampling newvolume;
    if ( pid.objectID()!=emobj->id() && pid.objectID()!=-1 )
	mSeedClickReturn();

    const Attrib::SelSpec* clickedas =
	clickcatcher_->info().getObjDataSelSpec();
    if ( !clickedas )
	mSeedClickReturn();

    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker || !seedpicker->canSetSectionID() ||
	 !seedpicker->setSectionID(emobj->sectionID(0)) )
    {
	mSeedClickReturn();
    }

    if ( clickcatcher_->info().isDoubleClicked() )
    {
	seedpicker->endSeedPick( true );
	mSeedClickReturn();
    }

    const MPE::SectionTracker* sectiontracker =
	tracker->getSectionTracker(emobj->sectionID(0), true);
    const Attrib::SelSpec* trackedatsel = sectiontracker
	? sectiontracker->adjuster()->getAttributeSel(0)
	: 0;

    if ( !visserv_->isTrackingSetupActive() && (seedpicker->nrSeeds()==0) )
    {
	if ( trackedatsel &&
	     (seedpicker->getSeedConnectMode()!=seedpicker->DrawBetweenSeeds) )
	{
	    bool chanceoferror = false;
	    if ( !trackedatsel->is2D() || trackedatsel->isStored() )
		chanceoferror = !engine.isSelSpecSame(*trackedatsel,*clickedas);
	    else
		chanceoferror = !FixedString(clickedas->defString())
				.startsWith( trackedatsel->defString() );

	    if ( chanceoferror )
	    {
		uiMSG().error(tr("Saved setup has different attribute. \n"
				 "Either change setup attribute or change\n"
				 "display attribute you want to track on"));
		mSeedClickReturn();
	    }
	}
    }

    if ( seedpicker->nrSeeds() > 0 )
    {
	if ( trackedatsel &&
	     (seedpicker->getSeedConnectMode()!=seedpicker->DrawBetweenSeeds) )
	{
	    bool chanceoferror = false;
	    if ( !trackedatsel->is2D() || trackedatsel->isStored() )
		chanceoferror = !engine.isSelSpecSame(*trackedatsel,*clickedas);
	    else
		chanceoferror = !FixedString(clickedas->defString())
				.startsWith( trackedatsel->defString() );

	    if ( chanceoferror )
	    {
		uiString warnmsg = tr("Setup suggests tracking is done on: "
				      "'%1'\nbut what you see is: '%2'.\n"
				      "To continue seed picking either "
				      "change displayed attribute or\n"
				      "change input data in Tracking Setup.")
				 .arg(trackedatsel->userRef())
				 .arg(clickedas->userRef());
		uiMSG().error( warnmsg );
		mSeedClickReturn();
	    }
	}
    }

    Coord3 seedpos;
    if ( pid.objectID() == -1 )
    {
	visSurvey::Scene* scene = visSurvey::STM().currentScene();
	seedpos = clickcatcher_->info().getPos();
	scene->getTempZStretchTransform()->transformBack( seedpos );
	scene->getUTM2DisplayTransform()->transformBack( seedpos );
    }
    else
    {
	seedpos = emobj->getPos(pid);
    }

    bool shiftclicked = clickcatcher_->info().isShiftClicked();

    if ( pid.objectID()==-1 && !shiftclicked &&
	 clickcatcher_->activateSower(emobj->preferredColor(),
				     seedpicker->getSeedPickArea()) )
    {
	 mSeedClickReturn();
    }

    if ( tracker->is2D() )
    {
	Pos::GeomID geomid = clickcatcher_->info().getGeomID();
	engine.setActive2DLine( geomid );

	mDynamicCastGet( MPE::Horizon2DSeedPicker*, h2dsp, seedpicker );
	DataPack::ID datapackid = clickcatcher_->info().getObjDataPackID();

	if ( clickedas && h2dsp )
	    h2dsp->setSelSpec( clickedas );

	if ( !clickedas || !h2dsp || !h2dsp->canAddSeed(*clickedas) )
	{
	    uiMSG().error(tr("2D tracking requires attribute from setup "
			     "to be displayed"));
	    mSeedClickReturn();
	}
	if ( datapackid > DataPack::cNoID() )
	    engine.setAttribData( *clickedas, datapackid );

	h2dsp->setLine( geomid );
	if ( !h2dsp->startSeedPick() )
	    mSeedClickReturn();
    }
    else
    {
	if ( !seedpicker->startSeedPick() )
	    mSeedClickReturn();

	newvolume = clickcatcher_->info().getObjCS();
	if ( newvolume.isEmpty() || !newvolume.isDefined() )
	    mSeedClickReturn();

	if ( newvolume != engine.activeVolume() )
	{
	    if ( oldactivevol_.isEmpty() )
	    {
		engine.swapCacheAndItsBackup();
		oldactivevol_ = engine.activeVolume();
	    }

	    NotifyStopper notifystopper( engine.activevolumechange );
	    engine.setActiveVolume( newvolume );
	    notifystopper.restore();

	    if ( clickedas )
	    {
		DataPack::ID datapackid =
				clickcatcher_->info().getObjDataPackID();
		if ( datapackid > DataPack::cNoID() )
		    engine.setAttribData( *clickedas, datapackid );
	    }

	    seedpicker->setSelSpec( clickedas );

	    engine.setOneActiveTracker( tracker );
	    engine.activevolumechange.trigger();
	}
    }

    seedpicker->setSowerMode( clickcatcher_->sequentSowing() );
    if ( mIsUdf(cureventnr_) && clickcatcher_->moreToSow() )
	shiftclicked = true;  // 1st seed sown is "tracking buffer" only

    beginSeedClickEvent( emobj );

    if ( pid.objectID()!=-1 || !clickcatcher_->info().getPickedNode().isUdf() )
    {
	const bool ctrlclicked = clickcatcher_->info().isCtrlClicked();
	if ( !clickcatcher_->info().getPickedNode().isUdf() )
	{
	    const EM::PosID nextpid =
		seedpicker->replaceSeed( clickcatcher_->info().getPickedNode(),
					 seedpos );
	    clickcatcher_->info().setPickedNode( nextpid );
	}
	if ( !shiftclicked && !ctrlclicked &&
	     seedpicker->getSeedConnectMode()==EMSeedPicker::DrawBetweenSeeds )
	{
	    if ( clickcatcher_->info().getPickedNode().isUdf() )
		clickcatcher_->info().setPickedNode( pid );
	}
	else if ( shiftclicked && ctrlclicked )
	{
	    if ( seedpicker->removeSeed( pid, true, false ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, false );
	}
	else if ( shiftclicked || ctrlclicked )
	{
	    if ( seedpicker->removeSeed( pid, true, true ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, false );
	}
	else
	{
	    if ( seedpicker->addSeed( seedpos, false ) )
		engine.updateFlatCubesContainer( newvolume, trackerid, true );
	}
    }
    else if ( seedpicker->addSeed(seedpos,shiftclicked) )
	engine.updateFlatCubesContainer( newvolume, trackerid, true );

    if ( !clickcatcher_->moreToSow() )
	endSeedClickEvent( emobj );


    // below is for double click event.
    // after double click we do return on line 251. next click reaches here, we
    // need tell seedpicker to prepare to start new trick line.
    if ( seedpicker->isSeedPickEnded() )
	seedpicker->endSeedPick( false );
}


void uiMPEMan::beginSeedClickEvent( EM::EMObject* emobj )
{
    if ( mIsUdf(cureventnr_) )
    {
	cureventnr_ = EM::EMM().undo().currentEventID();
	MouseCursorManager::setOverride( MouseCursor::Wait );
	if ( emobj )
	    emobj->setBurstAlert( true );
    }
}


void uiMPEMan::endSeedClickEvent( EM::EMObject* emobj )
{
    clickcatcher_->stopSowing();

    if ( !mIsUdf(cureventnr_) )
    {
	if ( emobj )
	    emobj->setBurstAlert( false );

	MouseCursorManager::restoreOverride();
	setUndoLevel( cureventnr_ );
	cureventnr_ = mUdf(int);
    }
}


void uiMPEMan::changePolySelectionMode()
{
    const bool topolymode = !visserv_->isSelectionModeOn();
    if ( topolymode )
    {
	visserv_->setViewMode( false );
	visserv_->setSelectionMode( uiVisPartServer::Polygon );
    }

    visserv_->turnSelectionModeOn( topolymode );
    turnSeedPickingOn( !topolymode );
}


void uiMPEMan::clearSelection()
{
    visSurvey::HorizonDisplay* hd = getSelectedDisplay();
    if ( visserv_->isSelectionModeOn() )
    {
	visserv_->turnSelectionModeOn( false );
	turnSeedPickingOn( true );
	if ( hd ) hd->clearSelections();
    }
    else
    {
	EM::Horizon3D* hor3d = getSelectedHorizon3D();
	if ( hor3d ) hor3d->resetChildren();

	if ( hd )
	{
	    hd->showSelections( false );
	    hd->showParentLine( false );
	}
    }
}


void uiMPEMan::deleteSelection()
{
    visSurvey::HorizonDisplay* hd = getSelectedDisplay();
    if ( visserv_->isSelectionModeOn() )
    {
	removeInPolygon();
	visserv_->turnSelectionModeOn( false );
	turnSeedPickingOn( true );
	if ( hd ) hd->clearSelections();
    }
    else
    {
	EM::Horizon3D* hor3d = getSelectedHorizon3D();
	if ( hor3d ) hor3d->deleteChildren();
	if ( hd ) hd->showSelections( false );
    }
}


void uiMPEMan::showParentsPath()
{ visserv_->sendVisEvent( uiVisPartServer::evShowMPEParentPath() ); }

void uiMPEMan::showSetupDlg()
{ visserv_->sendVisEvent( uiVisPartServer::evShowMPESetupDlg() ); }


bool uiMPEMan::isSeedPickingOn() const
{
    return clickcatcher_ && clickcatcher_->isOn();
}


bool uiMPEMan::isPickingWhileSetupUp() const
{
    return isSeedPickingOn() &&
	visserv_->isTrackingSetupActive();
}


void uiMPEMan::turnSeedPickingOn( bool yn )
{
    polyselstoppedseedpick_ = false;

    if ( !yn && clickcatcher_ )
	clickcatcher_->setEditor( 0 );

    if ( isSeedPickingOn() == yn )
	return;

    MPE::EMTracker* tracker = getSelectedTracker();

    if ( yn )
    {
	visserv_->setViewMode( false );

	updateClickCatcher();
	clickcatcher_->turnOn( true );

	const EM::EMObject* emobj =
			tracker ? EM::EMM().getObject(tracker->objectID()) : 0;

	if ( emobj )
	    clickcatcher_->setTrackerType( emobj->getTypeStr() );
    }
    else
    {
	MPE::EMSeedPicker* seedpicker =
		tracker ? tracker->getSeedPicker(true) : 0;
	if ( seedpicker )
	    seedpicker->stopSeedPick();

	if ( clickcatcher_ )
	    clickcatcher_->turnOn( false );
    }

    visserv_->sendVisEvent( uiVisPartServer::evPickingStatusChange() );
}


void uiMPEMan::updateClickCatcher()
{
    if ( !clickcatcher_ )
    {
	TypeSet<int> catcherids;
	visserv_->findObject( typeid(visSurvey::MPEClickCatcher),
			     catcherids );
	if ( catcherids.size() )
	{
	    visBase::DataObject* dobj = visserv_->getObject( catcherids[0] );
	    clickcatcher_ = reinterpret_cast<visSurvey::MPEClickCatcher*>(dobj);
	}
	else
	{
	    clickcatcher_ = visSurvey::MPEClickCatcher::create();
	}
	clickcatcher_->ref();
	clickcatcher_->click.notify(mCB(this,uiMPEMan,seedClick));
	clickcatcher_->turnOn( false );
    }

    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size() != 1 )
	return;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
		     surface, visserv_->getObject(selectedids[0]) );
    clickcatcher_->setEditor( surface ? surface->getEditor() : 0 );

    const int newsceneid = visserv_->getSceneID( selectedids[0] );
    if ( newsceneid<0 || newsceneid == clickablesceneid_ )
	return;

    visserv_->removeObject( clickcatcher_->id(), clickablesceneid_ );
    visserv_->addObject( clickcatcher_, newsceneid, false );
    clickablesceneid_ = newsceneid;
}


void uiMPEMan::treeItemSelCB( CallBacker* )
{
    validateSeedConMode();
    updateClickCatcher();
}


void uiMPEMan::validateSeedConMode()
{
    if ( visserv_->isTrackingSetupActive() )
	return;
    MPE::EMTracker* tracker = getSelectedTracker();
    if ( !tracker )
	return;
    const EM::EMObject* emobj = EM::EMM().getObject( tracker->objectID() );
    if ( !emobj )
	return;
    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(true);
    if ( !seedpicker ) return;

    const SectionTracker* sectiontracker =
			tracker->getSectionTracker( emobj->sectionID(0), true );
    const bool setupavailable = sectiontracker &&
				sectiontracker->hasInitializedSetup();
    if ( setupavailable || !seedpicker->doesModeUseSetup() )
	return;

    const int defaultmode = seedpicker->defaultSeedConMode( false );
    seedpicker->setSeedConnectMode( defaultmode );
}


void uiMPEMan::undo()
{
    MouseCursorChanger mcc( MouseCursor::Wait );
    uiString errmsg;
    engine().undo( errmsg );
    if ( !errmsg.isEmpty() )
	uiMSG().message( errmsg );
}


void uiMPEMan::redo()
{
    MouseCursorChanger mcc( MouseCursor::Wait );
    uiString errmsg;
    engine().redo( errmsg );
    if ( !errmsg.isEmpty() )
	uiMSG().message( errmsg );
}


MPE::EMTracker* uiMPEMan::getSelectedTracker()
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size()!=1 || visserv_->isLocked(selectedids[0]) )
	return 0;

    mDynamicCastGet( visSurvey::EMObjectDisplay*,
				surface, visserv_->getObject(selectedids[0]) );
    if ( !surface ) return 0;
    const EM::ObjectID oid = surface->getObjectID();
    const int trackerid = MPE::engine().getTrackerByObject( oid );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    if ( tracker && tracker->isEnabled() )
	return tracker;

    return 0;
}


visSurvey::HorizonDisplay* uiMPEMan::getSelectedDisplay()
{
    const TypeSet<int>& selectedids = visBase::DM().selMan().selected();
    if ( selectedids.size() != 1 )
	return 0;

    mDynamicCastGet(visSurvey::HorizonDisplay*,hd,
		    visserv_->getObject(selectedids[0]))
    return hd;
}


EM::Horizon3D* uiMPEMan::getSelectedHorizon3D()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    EM::EMObject* emobj =
		tracker ? EM::EMM().getObject(tracker->objectID()) : 0;
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)
    return hor3d;
}


#define mAddSeedConModeItems( seedconmodefld_, typ ) \
    if ( emobj && EM##typ##TranslatorGroup::keyword() == emobj->getTypeStr() ) \
    { \
	seedconmodefld_->setEmpty(); \
	for ( int idx=0; idx<typ##SeedPicker::nrSeedConnectModes(); idx++ ) \
	{ \
	    seedconmodefld_-> \
		    addItem( typ##SeedPicker::seedConModeText(idx,true) ); \
	} \
	if ( typ##SeedPicker::nrSeedConnectModes()<=0 ) \
	    seedconmodefld_->addItem("No seed mode"); \
    }


void uiMPEMan::updateSeedPickState()
{
    MPE::EMTracker* tracker = getSelectedTracker();
    MPE::EMSeedPicker* seedpicker = tracker ? tracker->getSeedPicker(true) : 0;

    if ( !seedpicker )
    {
	if ( isSeedPickingOn() )
	{
	    turnSeedPickingOn( false );
	    seedpickwason_ = true;
	}
	return;
    }

    if ( seedpickwason_ )
    {
	seedpickwason_ = false;
	turnSeedPickingOn( true );
    }
}


void uiMPEMan::trackerAddedRemovedCB( CallBacker* )
{
    if ( !engine().nrTrackersAlive() )
    {
	seedpickwason_ = false;
	engine().setActiveVolume( TrcKeyZSampling() );
    }
}


void uiMPEMan::visObjectLockedCB( CallBacker* )
{
}


void uiMPEMan::trackFromSeedsOnly()
{}

void uiMPEMan::trackFromSeedsAndEdges()
{}

void uiMPEMan::removeInPolygon()
{ visserv_->removeSelection(); }


void uiMPEMan::workAreaChgCB( CallBacker* )
{
    if ( !SI().sampling(true).includes( engine().activeVolume() ) )
    {
	engine().setActiveVolume( SI().sampling(true) );
    }
}


void uiMPEMan::initFromDisplay()
{
    // compatibility for session files where box outside workarea
    workAreaChgCB(0);
}


void uiMPEMan::setUndoLevel( int preveventnr )
{
    Undo& emundo = EM::EMM().undo();
    const int currentevent = emundo.currentEventID();
    if ( currentevent != preveventnr )
	    emundo.setUserInteractionEnd(currentevent);
}

