/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "viswell.h"

#include "visdatagroup.h"
#include "visdrawstyle.h"
#include "viscoord.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "vispolyline.h"
#include "vistext.h"
#include "vistransform.h"

#include "coltabsequence.h"
#include "cubesampling.h"
#include "iopar.h"
#include "indexedshape.h"
#include "ranges.h"
#include "scaler.h"
#include "survinfo.h"
#include "zaxistransform.h"

#include <osg/Switch>
#include <osg/Material>
#include <osg/Node>
#include <osgGeo/MarkerSet>
#include <osgText/Text>

mCreateFactoryEntry( visBase::Well );

namespace visBase
{

static const int cMaxNrLogSamples = 2000;

const char* Well::linestylestr()	{ return "Line style"; }
const char* Well::showwelltopnmstr()	{ return "Show top name"; }
const char* Well::showwellbotnmstr()	{ return "Show bottom name"; }
const char* Well::showmarkerstr()	{ return "Show markers"; }
const char* Well::showmarknmstr()	{ return "Show markername"; }
const char* Well::markerszstr()		{ return "Marker size"; }
const char* Well::showlogsstr()		{ return "Show logs"; }
const char* Well::showlognmstr()	{ return "Show logname"; }
const char* Well::logwidthstr()		{ return "Screen width"; }

Well::Well()
    : VisualObjectImpl( false )
    , showmarkers_(true)
    , showlogs_(true)
    , transformation_(0)
    , zaxistransform_(0)
    , voiidx_(-1)
    , markerset_( new osgGeo::MarkerSet() )
{
    markerset_->ref();
    addChild( markerset_);

    track_ = PolyLine::create();
    track_->ref();  

    track_->addPrimitiveSet( Geometry::RangePrimitiveSet::create() );
    addChild( track_->osgNode() );

    track_->setMaterial( new Material );
    welltoptxt_ =  Text2::create();
    wellbottxt_ =  Text2::create();
    welltoptxt_->ref();
    wellbottxt_->ref();
    welltoptxt_->setMaterial( track_->getMaterial() );
    wellbottxt_->setMaterial( track_->getMaterial() );
    addChild( welltoptxt_->osgNode() );
    addChild( wellbottxt_->osgNode() );

    markernames_ = Text2::create();
    markernames_->ref();
    addChild( markernames_->osgNode() );

    //lognmswitch_ = new SoSwitch;
    ////lognmleft_ = Text2::create();
    ////lognmswitch_->addChild( lognmleft_->osgNode()() );
    ////lognmright_ = Text2::create();
    ////lognmswitch_->addChild( lognmright_->osgNode()() );
    //lognmswitch_->whichChild = 0;

    //constantlogsizefac_ = constantLogSizeFactor();

    //setRepeat(0);
}

Well::~Well()
{
    if ( transformation_ ) transformation_->unRef();

    removeChild( track_->osgNode() );
    track_->unRef();

    removeLogs();
    markerset_->unref();
}

void Well::setZAxisTransform( ZAxisTransform* zat, TaskRunner* )
{
    if ( zaxistransform_==zat ) 
	return;
    
    if ( zaxistransform_ )
    {
	zaxistransform_->unRef();
    }

    zaxistransform_ = zat;
    if ( zaxistransform_ )
    {
	zaxistransform_->ref();
    }
}

void Well::setTrack( const TypeSet<Coord3>& pts )
{
    CubeSampling cs( false );
    for ( int idx=0; idx<pts.size(); idx++ )
	cs.include( SI().transform(pts[idx]), (float) pts[idx].z );

    if ( zaxistransform_ && zaxistransform_->needsVolumeOfInterest() )
    {
	if ( voiidx_ < 0 )
	    voiidx_ = zaxistransform_->addVolumeOfInterest( cs, true );
	else
	    zaxistransform_->setVolumeOfInterest( voiidx_, cs, true );
	zaxistransform_->loadDataIfMissing( voiidx_ );
    }

    int ptidx = 0;
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	if ( !pts[idx].isDefined() )
	    continue;

	track_->getCoordinates()->setPos( ptidx, pts[idx] );
	ptidx++;
    }
    
    RefMan<Geometry::RangePrimitiveSet> rps = 0;
    if ( !track_->nrPrimitiveSets() )
    {
	rps = Geometry::RangePrimitiveSet::create();
	track_->addPrimitiveSet( rps );
	
    }
    else
    {
	rps = (Geometry::RangePrimitiveSet*) track_->getPrimitiveSet( 0 );
    }
    
    rps->setRange( Interval<int>( 0, ptidx-1 ) );
}

void Well::setTrackProperties( Color& col, int width)
{
    LineStyle lst;
    lst.color_ = col;
    lst.width_ = width;
    setLineStyle( lst );
}

void Well::setLineStyle( const LineStyle& lst )
{
    track_->setLineStyle( lst );
}


const LineStyle& Well::lineStyle() const
{
    return track_->lineStyle();
}

void Well::setText( Text* tx, const char* chr, Coord3* pos,
		    const FontData& fnt )
{
    tx->setText( chr );
    tx->setFontData( fnt ); 
    if ( !SI().zRange(true).includes(pos->z, false) ) 
	pos->z = SI().zRange(true).limitValue( pos->z ); 
    tx->setPosition( *pos );
    tx->setJustification( Text::Left );
}

void Well::setWellName( const TrackParams& tp )
{
    BufferString label( tp.name_, " track" );
    track_->setName( label );

    if ( welltoptxt_->nrTexts()<1 )
	welltoptxt_->addText();

    if ( wellbottxt_->nrTexts()<1 )
	wellbottxt_->addText();
   
    setText(welltoptxt_->text(0),tp.isdispabove_ ? tp.name_ : "",tp.toppos_, 
	    tp.font_);
    setText(wellbottxt_->text(0),tp.isdispbelow_ ? tp.name_ : "",tp.botpos_, 
	    tp.font_);

}

void Well::showWellTopName( bool yn )
{
    welltoptxt_->turnOn( yn );
}

void Well::showWellBotName( bool yn )
{
    wellbottxt_->turnOn( yn );
}

bool Well::wellTopNameShown() const
{
    return welltoptxt_->isOn();
}

bool Well::wellBotNameShown() const
{
   return wellbottxt_->isOn();
}

void Well::setMarkerSet(const MarkerParams& mp)
{
    osgGeo::MarkerSet::MarkerType shape;
    
    switch ( mp.shapeint_ )
    {
    case 0:
	shape = osgGeo::MarkerSet::Cylinder;
	markerset_->setHeight(mp.cylinderheight_*10);
    	break;
    case 1:
	shape = osgGeo::MarkerSet::Box;  // it should be "Cone"
	break;
    case 2:
	shape = osgGeo::MarkerSet::Sphere;
	break;
    case 3:
	shape = osgGeo::MarkerSet::Box;
	break;
    default:
	return;
    }

    markerset_->setRadius(mp.size_*10);
    markerset_->setShape(shape);
    markerset_->setDetail(0.5f);
    markerset_->setMinimumScale(0);
    markerset_->setMaximumScale(1.5f);
    markerset_->setAutoTransformRotateMode(osg::AutoTransform::NO_ROTATION);
}

void Well::addMarker( const MarkerParams& mp )
{
    setMarkerSet( mp );

    Coord3 markerpos = *mp.pos_;
    if ( zaxistransform_ )
	  markerpos.z = zaxistransform_->transform( markerpos );
    if ( mIsUdf(markerpos.z) )
	  return;
    Coord3 disppos;
    if ( transformation_ )
        disppos = transformation_->transform( markerpos );

    markerset_->getVertexArray()->push_back( osg::Vec3f(disppos.x,disppos.y,
		disppos.z) );
    markerset_->getColorArray()->push_back( Conv::to<osg::Vec4>(mp.col_));

    const int textidx = markernames_->addText();
    Text* txt = markernames_->text( textidx );
    txt->setColor( mp.namecol_ );
    setText(txt,mp.name_,mp.pos_,mp.font_);

    return;
}

void Well::removeAllMarkers() 
{
    markerset_->getVertexArray()->clear();
    markerset_->getColorArray()->clear();
    markerset_->getVertexArray()->dirty();
    markernames_->removeAll();
}

void Well::setMarkerScreenSize( int size )
{
    markersize_ = size;
    for ( int idx=0; idx<markergroup_->size(); idx++ )
    {
	mDynamicCastGet(Marker*,marker,markergroup_->getObject(idx))
	marker->setScreenSize( size );
    }
}

int Well::markerScreenSize() const
{
    mDynamicCastGet(Marker*,marker,markergroup_->getObject(0))
    return marker ? mNINT32(marker->getScreenSize()) : markersize_;
}

bool Well::canShowMarkers() const
{
    return markerset_->getVertexArray()->size(); 
}

void Well::showMarkers( bool yn )
{
    if ( yn==markersShown() )
	return;

    if ( yn )
	addChild( markerset_ );
    else
	removeChild( markerset_ );
}

bool Well::markersShown() const
{
    return childIndex( markerset_ )!=-1;
}

void Well::showMarkerName( bool yn )
{ 
    markernames_->turnOn(yn);
}

bool Well::markerNameShown() const
{ 
    return markernames_->isOn();
}

#define mGetLoopSize(nrsamp,step)\
{\
    if ( nrsamp > cMaxNrLogSamples )\
    {\
	step = (float)nrsamp/cMaxNrLogSamples;\
	nrsamp = cMaxNrLogSamples;\
    }\
}

void Well::setLogData( const TypeSet<Coord3Value>& crdvals, 
		       const LogParams& lp )
{
    /*
    const bool rev = lp.range_.start > lp.range_.stop;

    Interval<float> rg = lp.valrange_; 
    const bool isfullfilled = lp.isleftfilled_ && lp.isrightfilled_; 
    const bool fillrev = !isfullfilled &&  
		      ( ( lp.lognr_ == 1 && lp.isleftfilled_ && !rev )
		     || ( lp.lognr_ == 1 && lp.isrightfilled_ && rev )
		     || ( lp.lognr_ == 2 && lp.isrightfilled_ && !rev )
		     || ( lp.lognr_ == 2 && lp.isleftfilled_ && rev ) );

    for ( int idx=0; idx<log_.size(); idx++ )
    {
	log_[idx]->setRevScale( rev, lp.lognr_ );
	log_[idx]->setFillRevScale( fillrev, lp.lognr_ );
    }
    rg.sort();
    LinScaler scaler( rg.start, 0, rg.stop, 100 );
    
    int nrsamp = crdvals.size();
    float step = 1;
    mGetLoopSize( nrsamp, step );
    int validx = 0;
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	const int index = mNINT32(idx*step);
	const float val = isfullfilled ? 100 : 
	    		getValue( crdvals, index, lp.islogarithmic_, scaler );
	const Coord3& pos = getPos( crdvals, index );
	if ( mIsUdf( pos.z ) || mIsUdf( val ) )
	    continue;   
	
	for ( int lidx=0; lidx<log_.size(); lidx++ )
	    log_[lidx]->setLogValue( validx, 
			    SbVec3f((float) pos.x,(float) pos.y,(float) pos.z), 
			    val, lp.lognr_ );
	validx++;
    }
    showLog( showlogs_, lp.lognr_ );
    */
}

#define mSclogval(val)\
{\
    val += 1;\
    val = ::log( val );\
}
void Well::setFilledLogData( const TypeSet<Coord3Value>& crdvals, 
			     const LogParams& lp )
{
  /*  Interval<float> rg = lp.valfillrange_;
    rg.sort();
    float minval = rg.start;
    float maxval = rg.stop;

    LinScaler scaler( minval, 0, maxval, 100 );
    int nrsamp = crdvals.size();
    float step = 1;
    mGetLoopSize( nrsamp, step );
    for ( int idx=0; idx<nrsamp; idx++ )
    {
	const int index = mNINT32(idx*step);
	const float val = getValue( crdvals, index, lp.islogarithmic_, scaler );
	const Coord3& pos = getPos( crdvals, index );
	if ( mIsUdf( pos.z ) || mIsUdf( val ) )
	    continue;   
	for ( int lidx=0; lidx<log_.size(); lidx++ )
	    log_[lidx]->setFillLogValue( idx, val, lp.lognr_ );
    }

    Interval<float> selrg = lp.fillrange_;
    selrg.sort();
    float rgstop = (float) scaler.scale( selrg.stop );
    float rgstart = (float) scaler.scale( selrg.start );
    if ( lp.islogarithmic_ )
    {
	mSclogval( rgstop ); 
	mSclogval( rgstart ); 
    }
    
    for ( int logidx=0; logidx<log_.size(); logidx++ )
	log_[logidx]->setFillExtrValue( rgstop, rgstart, lp.lognr_ );

    showLog( showlogs_, lp.lognr_ );*/
}

Coord3 Well::getPos( const TypeSet<Coord3Value>& crdv, int idx ) const 
{
    const Coord3Value& cv = crdv[idx];
    Coord3 crd = cv.coord;
    if ( zaxistransform_ )
	crd.z = zaxistransform_->transform( crd );
    if ( mIsUdf(crd.z) )
	return crd;

    Coord3 pos( 0,0,0 );
    if ( transformation_ )
	pos = transformation_->transform( crd );
    return pos;
}

float Well::getValue( const TypeSet<Coord3Value>& crdvals, int idx, 
		      bool sclog, const LinScaler& scaler ) const
{
    const Coord3Value& cv = crdvals[idx];
    float val = (float) scaler.scale( cv.value );
    if ( val < 0 || mIsUdf(val) ) val = 0;
    if ( val > 100 ) val = 100;
    if ( sclog ) mSclogval(val);

    return val;
}

void Well::clearLog( int lognr )
{
 /*   for ( int idx=0; idx<log_.size(); idx++ )
        log_[idx]->clearLog( lognr );*/
}

void Well::hideUnwantedLogs( int lognr, int rpt )
{ 
  /*  for ( int idx=rpt; idx<log_.size(); idx++)
	showOneLog(false, lognr, idx);	*/
}

void Well::removeLogs()
{
  /*  for ( int idx=log_.size()-1; idx>=0; idx-- )
    {
	log_[idx]->unrefNoDelete();
	removeChild( log_[idx]  );
	log_.removeSingle( idx );
    }*/
}

void Well::setRepeat( int rpt )
{
 //   if ( rpt < 0 || mIsUdf(rpt) ) rpt = 0; 
 //   const int lsz = log_.size();

 //   for ( int idx=lsz; idx<rpt; idx++ )
 //   {
	//log_ += new SoPlaneWellLog;
	//log_[idx]->setLogConstantSize( log_[0]->logConstantSize() );
	//log_[idx]->setLogConstantSizeFactor( constantlogsizefac_ );
	//addChild( log_[idx] );
 //   }
}

float Well::constantLogSizeFactor() const
{
   return .0;
    //const int inlnr = SI().inlRange( true ).nrSteps();
    //const int crlnr = SI().crlRange( true ).nrSteps();
    //const float survfac = sqrt( (float)(crlnr*crlnr + inlnr*inlnr) );
    //return survfac * 43; //hack 43 best factor based on F3_Demo
}

void Well::setOverlapp( float ovlap, int lognr )
{
 /*   ovlap = 100 - ovlap;
    if ( ovlap < 0.0 || mIsUdf(ovlap)  ) ovlap = 0.0;
    if ( ovlap > 100.0 ) ovlap = 100.0;
    for ( int idx=0; idx<log_.size(); idx++ )
        log_[idx]->setShift( idx*ovlap, lognr );*/
}


void Well::setLogFill( bool fill, int lognr )
{
 /*   if ( log_.size() > 0 )
	log_[0]->setLogFill( fill, lognr );*/
}


void Well::setLogStyle( bool style, int lognr )
{
 /*   for ( int idx=0; idx<log_.size(); idx++ )
	log_[idx]->setLogStyle( (1-style), lognr );*/
}

void Well::setLogColor( const Color& col, int lognr )
{
//#define col2f(rgb) float(col.rgb())/255
//    for ( int idx=0; idx<log_.size(); idx++ )
//        log_[idx]->setLineColor( SbVec3f(col2f(r),col2f(g),col2f(b)), lognr );
}

const Color& Well::logColor( int lognr ) const
{
    static Color color;
 /*   const SbVec3f& col = log_[0]->lineColor( lognr );
    const int r = mNINT32(col[0]*255);
    const int g = mNINT32(col[1]*255);
    const int b = mNINT32(col[2]*255);
    color.set( (unsigned char)r, (unsigned char)g, (unsigned char)b );*/
    return color;
}

#define scolors2f(rgb) float(lp.seiscolor_.rgb())/255
#define colors2f(rgb) float(col.rgb())/255
void Well::setLogFillColorTab( const LogParams& lp, int lognr )
{
  /*  int seqidx = ColTab::SM().indexOf( lp.seqname_ );
    if ( seqidx<0 || mIsUdf(seqidx) ) seqidx = 0;
    const ColTab::Sequence* seq = ColTab::SM().get( seqidx );

    float colors[256][3];
    for ( int idx=0; idx<256; idx++ )
    {
	const bool issinglecol = ( !lp.iswelllog_ || 
	    		(lp.iswelllog_ && lp.issinglcol_ ) );
	float colstep = lp.iscoltabflipped_ ? 1-(float)idx/255 : (float)idx/255;
	Color col = seq->color( colstep );
	colors[idx][0] = issinglecol ? scolors2f(r) : colors2f(r);
	colors[idx][1] = issinglecol ? scolors2f(g) : colors2f(g);
	colors[idx][2] = issinglecol ? scolors2f(b) : colors2f(b);
    }

    for ( int idx=0; idx<log_.size(); idx++ )
	log_[idx]->setFilledLogColorTab( colors, lognr );*/
}

void Well::setLogLineDisplayed( bool isdisp, int lognr )
{
 /*   for ( int idx=0; idx<log_.size(); idx++ )
        log_[idx]->setLineDisplayed( isdisp, lognr );*/
}

bool Well::logLineDisplayed( int lognr ) const 
{
    return true;
 //   return log_.size() ? log_[0]->lineWidth( lognr ) : 0 ;
}


void Well::setLogLineWidth( float width, int lognr )
{
 /*   for ( int idx=0; idx<log_.size(); idx++ )
        log_[idx]->setLineWidth( width, lognr );*/
}

float Well::logLineWidth( int lognr ) const
{
   return .0;
    // return log_.size() ? log_[0]->lineWidth( lognr ) : 0 ;
}

void Well::setLogWidth( int width, int lognr )
{
   /* if (lognr == 1)
    {
	for ( int i=0; i<log_.size(); i++ )
	    log_[i]->screenWidth1.setValue( (float)width );
    }
    else
    {
	for ( int i=0; i<log_.size(); i++ )
	    log_[i]->screenWidth2.setValue( (float)width );
    }*/
}


int Well::logWidth() const
{
   return .0;
    //return log_.size() ? mNINT32(log_[0]->screenWidth1.getValue()) 
		  //    || mNINT32(log_[0]->screenWidth2.getValue()) : false;
}

void Well::showLogs( bool yn )
{
    //showlogs_ = yn;
    //for ( int idx=0; idx<log_.size(); idx++ )
    //{
    //    log_[idx]->showLog( yn, 1 );
    //    log_[idx]->showLog( yn, 2 );
    //}
}

void Well::showLog( bool yn, int lognr )
{
  /*  for ( int idx=0; idx<log_.size(); idx++ )
        log_[idx]->showLog( yn, lognr );*/
}

void Well::showOneLog( bool yn, int lognr, int idx )
{
  //  log_[idx]->showLog( yn, lognr );
}

bool Well::logsShown() const
{
    return .0;
    //  return log_.size() ? log_[0]->logShown(1) || log_[0]->logShown(2) : false;
}

void Well::setLogConstantSize( bool yn )
{
 /*   for ( int idx=0; idx<log_.size(); idx++ )
	log_[idx]->setLogConstantSize( yn );*/
}

bool Well::logConstantSize() const
{
  return .0;
    //  return log_.size() ? log_[0]->logConstantSize() : true;
}

void Well::showLogName( bool yn )
{} 

bool Well::logNameShown() const
{ return false; }

void Well::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ )
	transformation_->unRef();
    transformation_ = nt;
    if ( transformation_ )
	transformation_->ref();

    track_->setDisplayTransformation( transformation_ );

    wellbottxt_->setDisplayTransformation( transformation_ );
    welltoptxt_->setDisplayTransformation( transformation_ );
    markernames_->setDisplayTransformation( transformation_ );
}

const mVisTrans* Well::getDisplayTransformation() const
{ return transformation_; }

void Well::fillPar( IOPar& par ) const
{
    BufferString linestyle;
    lineStyle().toString( linestyle );
    par.set( linestylestr(), linestyle );

    par.setYN( showwelltopnmstr(), welltoptxt_->isOn() );
    par.setYN( showwellbotnmstr(), wellbottxt_->isOn() );
    par.setYN( showmarkerstr(), markersShown() );
    par.setYN( showmarknmstr(), markerNameShown() );
    par.setYN( showlogsstr(), logsShown() );
    par.setYN( showlognmstr(), logNameShown() );
    par.set( markerszstr(), markersize_ );
    par.set( logwidthstr(), logWidth() );
}

int Well::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res!=1 ) return res;

    BufferString linestyle;
    if ( par.get(linestylestr(),linestyle) )
    {
	LineStyle lst;
	lst.fromString( linestyle );
	setLineStyle( lst );
    }

#define mParGetYN(str,func) \
    doshow = true; \
    par.getYN(str,doshow); \
    func( doshow );

    bool doshow;
    mParGetYN(showwelltopnmstr(),showWellTopName);
    mParGetYN(showwellbotnmstr(),showWellBotName);
    mParGetYN(showmarkerstr(),showMarkers);	showmarkers_ = doshow;
    mParGetYN(showmarknmstr(),showMarkerName);  showlogs_ = doshow;
    mParGetYN(showlogsstr(),showLogs);
    mParGetYN(showlognmstr(),showLogName);

    par.get( markerszstr(), markersize_ );
    setMarkerScreenSize( markersize_ );

    return 1;
}

}; // namespace visBase
