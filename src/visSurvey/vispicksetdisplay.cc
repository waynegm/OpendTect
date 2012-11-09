/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Feb 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vispicksetdisplay.h"

#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "pickset.h"
#include "picksettr.h"
#include "separstr.h"
#include "survinfo.h"
#include "visdrawstyle.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "visrandompos2body.h"
#include "zaxistransform.h"

mCreateFactoryEntry( visSurvey::PickSetDisplay );

namespace visSurvey {

const char* PickSetDisplay::sKeyNrPicks()       { return "No Picks"; }
const char* PickSetDisplay::sKeyPickPrefix()    { return "Pick "; }
const char* PickSetDisplay::sKeyDisplayBody()	{ return "Show Body"; }



PickSetDisplay::PickSetDisplay()
    : bodydisplay_( 0 )
    , shoulddisplaybody_( false )  
{}


PickSetDisplay::~PickSetDisplay()
{
    if ( bodydisplay_ ) 
	bodydisplay_->unRef();
    if ( scene_ )
	scene_->zstretchchange.remove(
		mCB(this,PickSetDisplay,sceneZChangeCB) );
}


void PickSetDisplay::getPickingMessage( BufferString& str ) const
{
    float area = set_ ? set_->getXYArea() : mUdf(float);
    const bool hasarea = !mIsUdf(area) && area>0;
    BufferString areastring;

    if ( hasarea )
    {
	areastring = "Area=";
	areastring += getAreaString( area, false, 0 );
    }

    str = "Picking (Nr picks="; str += set_ ? set_->size() : 0;
    if ( !areastring.isEmpty() ) { str +=", "; str += areastring; }
    str += ")";
}


void PickSetDisplay::setColor( Color nc )
{
    if ( set_ )
	set_->disp_.color_ = nc;
    
    if ( !bodydisplay_ ) return;
    
    if ( !bodydisplay_->getMaterial() )
	bodydisplay_->setMaterial( new visBase::Material );

    bodydisplay_->getMaterial()->setColor( nc );
}


void PickSetDisplay::setDisplayTransformation( const mVisTrans* newtr )
{
    visSurvey::LocationDisplay::setDisplayTransformation( newtr );
    if ( bodydisplay_ )
	bodydisplay_->setDisplayTransformation( newtr );
}


const mVisTrans* PickSetDisplay::getDisplayTransformation() const
{
    return transformation_;
}


bool PickSetDisplay::isBodyDisplayed() const
{
    return bodydisplay_ && shoulddisplaybody_;
}


void PickSetDisplay::displayBody( bool yn )
{
    shoulddisplaybody_ = yn;
    if ( bodydisplay_ )
	bodydisplay_->turnOn( yn );
}


bool PickSetDisplay::setBodyDisplay()
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );

    if ( !shoulddisplaybody_ || !set_ || !set_->size() )
	return false;
    
    if ( !bodydisplay_ )
    {
	bodydisplay_ = visBase::RandomPos2Body::create();
	bodydisplay_->ref();
	addChild( bodydisplay_->getInventorNode() );
    }
    
    if ( !bodydisplay_->getMaterial() )
	bodydisplay_->setMaterial( new visBase::Material );
    bodydisplay_->getMaterial()->setColor( set_->disp_.color_ );
    bodydisplay_->setDisplayTransformation( transformation_ );
    
    TypeSet<Coord3> picks;
    for ( int idx=0; idx<set_->size(); idx++ )
    {
	picks += (*set_)[idx].pos;
    	if ( datatransform_ )
	    picks[idx].z = datatransform_->transformBack( picks[idx] );
    }
    
    return  bodydisplay_->setPoints( picks );
}



visBase::VisualObject* PickSetDisplay::createLocation() const
{
    visBase::Marker* marker = visBase::Marker::create();
    marker->setType( (MarkerStyle3D::Type) set_->disp_.markertype_ );
    marker->setScreenSize( mCast(float,set_->disp_.pixsize_) );
    marker->setMaterial( 0 );
    if ( scene_ )
	marker->setZStretch( scene_->getZStretch()*scene_->getZScale()/2 );
    return marker;
}


void PickSetDisplay::setPosition( int idx, const Pick::Location& loc )
{
    mDynamicCastGet(visBase::Marker*,marker,group_->getObject(idx));
    marker->setCenterPos( loc.pos );
    marker->setDirection( loc.dir );
    BufferString dipvaluetext;
    loc.getText( "Dip", dipvaluetext );
    SeparString dipstr( dipvaluetext );
    const float inldip = dipstr.getFValue( 0 );
    const float crldip = dipstr.getFValue( 1 );
    marker->setDip( inldip, crldip );
}


Coord3 PickSetDisplay::getPosition( int loc ) const 
{
    mDynamicCastGet( visBase::Marker*, marker, group_->getObject(loc) );
    return marker->centerPos();
}


::Sphere PickSetDisplay::getDirection( int loc ) const 
{
    mDynamicCastGet( visBase::Marker*, marker, group_->getObject(loc) );
    return marker->getDirection();
}


void PickSetDisplay::locChg( CallBacker* cb )
{
    LocationDisplay::locChg( cb );
    setBodyDisplay();
}


void PickSetDisplay::setChg( CallBacker* cb )
{
    LocationDisplay::setChg( cb );
    setBodyDisplay();
}


void PickSetDisplay::dispChg( CallBacker* cb )
{
    mDynamicCastGet(visBase::Marker*,firstmarker,group_->getObject(0));
    if ( firstmarker )
    {
	const int oldpixsz = (int)(firstmarker->getScreenSize() + .5);
	if ( oldpixsz != set_->disp_.pixsize_ )
	{
	    if ( needline_ && polyline_ )
	    {
		LineStyle ls; ls.width_ = set_->disp_.pixsize_;
		polyline_->setLineStyle( ls );
	    }

	    for ( int idx=0; idx<group_->size(); idx++ )
	    {
		mDynamicCastGet(visBase::Marker*,marker,
				group_->getObject(idx));
		if ( marker )
		    marker->setScreenSize( mCast(float,set_->disp_.pixsize_) );
	    }
	}

	if ( (int)firstmarker->getType() != set_->disp_.markertype_ )
	{
	    for ( int idx=0; idx<group_->size(); idx++ )
	    {
		mDynamicCastGet(visBase::Marker*,marker,
				group_->getObject(idx))
		if ( marker )
		{
		    marker->setType(
			    (MarkerStyle3D::Type)set_->disp_.markertype_ );
		}
	    }
	}
    }

    LocationDisplay::dispChg( cb );
}


void PickSetDisplay::setScene( Scene* scn )
{
    SurveyObject::setScene( scn );
    if ( scene_ ) 
	scene_->zstretchchange.notify( 
		mCB(this,PickSetDisplay,sceneZChangeCB) );
}


void PickSetDisplay::sceneZChangeCB( CallBacker* )
{
    for ( int idx=0; idx<group_->size(); idx++ )
    {
	mDynamicCastGet(visBase::Marker*,marker,group_->getObject(idx));
	if ( marker && scene_ )
	    marker->setZStretch( scene_->getZStretch()*scene_->getZScale()/2 );
    }
}


int PickSetDisplay::isMarkerClick( const TypeSet<int>& path ) const
{
    for ( int idx=group_->size()-1; idx>=0; idx-- )
    {
	mDynamicCastGet(visBase::Marker*,marker,group_->getObject(idx));
	if ( !marker )
	    continue;

	if ( path.indexOf(marker->id())!=-1 )
	    return idx;
    }

    return -1;
}


void PickSetDisplay::fillPar( IOPar& par ) const
{
    LocationDisplay::fillPar( par );
    par.set( sKeyDisplayBody(), shoulddisplaybody_ );
}


int PickSetDisplay::usePar( const IOPar& par )
{
    int res =  visSurvey::LocationDisplay::usePar( par );
    if ( res != 1 ) return res;
    
    bool showbody = false;
    par.getYN( sKeyDisplayBody(), showbody );
    displayBody( showbody );

    return 1;
}

}; // namespace visSurvey
