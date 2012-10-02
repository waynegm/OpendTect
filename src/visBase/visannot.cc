/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visannot.h"
#include "vistext.h"
#include "visdatagroup.h"
#include "vispickstyle.h"
#include "vismaterial.h"
#include "ranges.h"
#include "visosg.h"
#include "samplingdata.h"
#include "axislayout.h"
#include "iopar.h"
#include "survinfo.h"

#include "SoOD.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Switch>

mCreateFactoryEntry( visBase::Annotation );

namespace visBase
{

const char* Annotation::textprefixstr()	    { return "Text "; }
const char* Annotation::cornerprefixstr()   { return "Corner "; }
const char* Annotation::showtextstr()	    { return "Show Text"; }
const char* Annotation::showscalestr()	    { return "Show Scale"; }

Annotation::Annotation()
    : VisualObjectImpl( false )
    , pickstyle_(PickStyle::create())
    , geode_( new osg::Geode )
    , axisnames_( Text2::create() )
    , axisannot_( Text2::create() )
{
    removeSwitch();
    addChild( geode_ );
    
    annotscale_[0] = annotscale_[1] = annotscale_[2] = 1;

    annotcolor_ = Color::White();
    
    pickstyle_->ref();
    addChild( pickstyle_->getInventorNode() );
    pickstyle_->setStyle( PickStyle::Unpickable );

    float pos[8][3] =
    {
	 { 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 0 },
	 { 1, 0, 0 }, { 1, 0, 1 }, { 1, 1, 1 }, { 1, 1, 0 }
    };

    const osg::Vec3* ptr = (osg::Vec3*) pos;
    osg::ref_ptr<osg::Vec3Array> coords = new osg::Vec3Array( 8, ptr );
    box_ = new osg::Geometry;
    box_->setName( "Box" );

    box_->setVertexArray( coords );

    GLubyte indices[] = { 0, 1, 1, 2, 2, 3, 3, 0,
			   4, 5, 5, 6, 6, 7, 7, 4,
			   0, 4, 1, 5, 2, 6, 3, 7 };
    box_->addPrimitiveSet(
	    new osg::DrawElementsUByte( GL_LINES, 24, indices  ) );
    
    box_->setColorBinding( osg::Geometry::BIND_OVERALL );

    geode_->addDrawable( box_ );

    addChild( axisannot_->osgNode() );
    addChild( axisnames_->osgNode() );
       
#define mAddText \
    text = new Text; \
    { \
	const int txtidx = axisnames_->addText(); \
	text = axisnames_->text( txtidx ); \
	text->setJustification( Text::Right ); \
    }
    
    Text* text = 0; mAddText mAddText mAddText
    
    gridlines_ = new osg::Geometry;
    
    osg::ref_ptr<osg::Vec3Array> gridlinecoords = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> gridlinenormals = new osg::Vec3Array;

    gridlines_->setVertexArray( gridlinecoords );
    gridlines_->setNormalArray( gridlinenormals );

    updateTextPos();
}


Annotation::~Annotation()
{
    pickstyle_->unRef();
}


#define mImplSwitches( str, node ) \
void Annotation::show##str( bool yn ) \
{ \
    if ( yn==is##str##Shown() ) \
	return; \
    if ( yn ) \
    { \
	addChild( node ); \
    } \
    else \
    { \
	removeChild( node ); \
    } \
} \
 \
 \
bool Annotation::is##str##Shown() const \
{ \
    return childIndex( node )!=-1; \
}
    
    
mImplSwitches( Text, axisnames_->osgNode() );
mImplSwitches( Scale, axisannot_->osgNode() );
mImplSwitches( GridLines, geode_ );


void Annotation::setCubeSampling( const CubeSampling& cs )
{
    const Interval<int> inlrg = cs.hrg.inlRange();
    const Interval<int> crlrg = cs.hrg.crlRange();
    const Interval<float>& zrg = cs.zrg;

    setCorner( 0, inlrg.start, crlrg.start, zrg.start );
    setCorner( 1, inlrg.stop, crlrg.start, zrg.start );
    setCorner( 2, inlrg.stop, crlrg.stop, zrg.start );
    setCorner( 3, inlrg.start, crlrg.stop, zrg.start );
    setCorner( 4, inlrg.start, crlrg.start, zrg.stop );
    setCorner( 5, inlrg.stop, crlrg.start, zrg.stop );
    setCorner( 6, inlrg.stop, crlrg.stop, zrg.stop );
    setCorner( 7, inlrg.start, crlrg.stop, zrg.stop );

    updateTextPos();
    updateGridLines();
}



void Annotation::setCorner( int idx, float x, float y, float z )
{
     osg::Vec3& coord =
	 ((osg::Vec3*) box_->getVertexArray()->getDataPointer())[idx];

     coord = osg::Vec3f( x, y, z );
}


void Annotation::setText( int dim, const char* string )
{
    axisnames_->text(dim)->setText( string );
}

    
/*
void Annotation::setTextColor( int dim, const Color& col )
{
    Text2* text = (Text2*)texts_->getObject( dim );
    if ( text )
	text->getMaterial()->setColor( col );
}


void Annotation::updateTextColor( const Color& col )
{
    annotcolor_ = col;
    for ( int idx=0; idx<3; idx++ )
    {
	setTextColor( idx, annotcolor_ );
    }
    updateTextPos();
}

*/

void Annotation::updateGridLines()
{
    osg::Vec3Array* normals = mGetOsgVec3Arr( gridlines_->getNormalArray() );
    osg::Vec3Array* coords = mGetOsgVec3Arr( gridlines_->getVertexArray() );
    
    normals->resize( 0 );
    coords->resize( 0 );
    
    for ( int dim=0; dim<3; dim++ )
    {
	osg::Vec3 p0;
	osg::Vec3 p1;
	getAxisCoords( dim, p0, p1 );
	Interval<float> range( p0[dim], p1[dim] );
	const SamplingData<float> sd = AxisLayout<float>( range ).sd_;
	
	osg::Vec3f corners[4];
	const osg::Vec3f* cornercoords = (const osg::Vec3f*)
		box_->getVertexArray()->getDataPointer();
	
	if ( dim==0 )
	{
	    corners[0] = cornercoords[ 0 ];
	    corners[1] = cornercoords[ 3 ];
	    corners[2] = cornercoords[ 7 ];
	    corners[3] = cornercoords[ 4 ];
	}
	else if ( dim==1 )
	{
	    corners[0] = cornercoords[ 0 ];
	    corners[1] = cornercoords[ 1 ];
	    corners[2] = cornercoords[ 5 ];
	    corners[3] = cornercoords[ 4 ];
	}
	else
	{
	    corners[0] = cornercoords[ 0 ];
	    corners[1] = cornercoords[ 1 ];
	    corners[2] = cornercoords[ 2 ];
	    corners[3] = cornercoords[ 3 ];
	}
	
	for ( int idx=0; ; idx++ )
	{
	    const float val = sd.atIndex(idx);
	    if ( val <= range.start )		continue;
	    else if ( val > range.stop )	break;
	    
	    corners[0][dim] = corners[1][dim] = corners[2][dim] =
			      corners[3][dim] = val;
	    	    
#define mAddOneLine( c1, c2, norm ) \
coords->push_back( corners[c1] ); \
normals->push_back( norm ); \
coords->push_back( corners[c2] ); \
normals->push_back( norm );
	    
	    mAddOneLine( 0, 1, osg::Vec3(0,0,1) );
	    mAddOneLine( 1, 2, osg::Vec3(0,0,1) );
	    mAddOneLine( 2, 3, osg::Vec3(0,0,1) );
	    mAddOneLine( 3, 0, osg::Vec3(0,0,1) );
	}
    }
    
    
    if ( !gridlines_->getNumPrimitiveSets() )
    {
	gridlines_->addPrimitiveSet(
	    new osg::DrawArrays( osg::PrimitiveSet::LINES, 0, coords->size() ));
    }
    else
    {
	osg::DrawArrays* primitiveset =
	    (osg::DrawArrays*) gridlines_->getPrimitiveSet( 0 );
	primitiveset->setCount( coords->size() );
    }
}


void Annotation::getAxisCoords( int dim, osg::Vec3& p0, osg::Vec3& p1 ) const
{
    int pidx0;
    int pidx1;

    if ( dim==0)
    {
	pidx0 = 0;
	pidx1 = 1;
    }
    else if ( dim==1 )
    {
	pidx0 = 0;
	pidx1 = 3;
    }
    else
    {
	pidx0 = 0;
	pidx1 = 4;
    }

    const osg::Vec3f* cornercoords = (const osg::Vec3f*)
	box_->getVertexArray()->getDataPointer();

    p0 = cornercoords[pidx0];
    p1 = cornercoords[pidx1];
}


void Annotation::updateTextPos()
{
    int curscale = 0;
    for ( int dim=0; dim<3; dim++ )
    {
	osg::Vec3 p0;
	osg::Vec3 p1;
	getAxisCoords( dim, p0, p1 );

	osg::Vec3 tp;
	tp[0] = (p0[0]+p1[0]) / 2;
	tp[1] = (p0[1]+p1[1]) / 2;
	tp[2] = (p0[2]+p1[2]) / 2;

	axisnames_->text(dim)->setPosition( tp );
	
	Interval<float> range( p0[dim], p1[dim] );
	const SamplingData<float> sd = AxisLayout<float>( range ).sd_;
	
	for ( int idx=0; ; idx++ )
	{
	    float val = sd.atIndex(idx);
	    if ( val <= range.start )		continue;
	    else if ( val > range.stop )	break;

	    
	    if ( curscale>=axisannot_->nrTexts() )
	    {
		axisannot_->addText();
	    }
	    
	    Text* text = axisannot_->text(curscale++);
	    
	    osg::Vec3 pos( p0 );
	    pos[dim] = val;
	    float displayval = val;
	    displayval *= annotscale_[dim];

	    text->setPosition( pos );
	    text->setText( toString(displayval) );
	}
    }
    
    for ( int idx=axisannot_->nrTexts()-1; idx>=curscale; idx-- )
    {
	axisannot_->removeText( axisannot_->text(idx) );
    }
    
}


void visBase::Annotation::setAnnotScale( int dim, int nv )
{
    annotscale_[dim] = nv;
    updateTextPos();
}


void Annotation::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    BufferString key;
    
    const osg::Vec3f* cornercoords =
	(const osg::Vec3f*) box_->getVertexArray()->getDataPointer();
    
    for ( int idx=0; idx<8; idx++ )
    {
	key = cornerprefixstr();
	key += idx;
	const osg::Vec3 pos = cornercoords[idx];
	par.set( key, pos[0], pos[1], pos[2] );
    }

    for ( int dim=0; dim<3; dim++ )
    {
	key = textprefixstr();
	key += dim;
	const Text* text = axisnames_->text(dim);
	if ( !text ) continue;

	BufferString str;
	text->getText( str );
	par.set( key, str.buf() );
    }

    par.setYN( showtextstr(), isTextShown() );
    par.setYN( showscalestr(), isScaleShown() );
}


int Annotation::usePar( const IOPar& par )
{
    int res = VisualObjectImpl::usePar( par );
    if ( res != 1 ) return res;

    BufferString key;
    for ( int idx=0; idx<8; idx++ )
    {
	key = cornerprefixstr();
	key += idx;

	double x, y, z;
	if ( !par.get( key, x, y, z ) )
	    return -1;

	setCorner( idx, x, y, z );
    }

    for ( int idx=0; idx<3; idx++ )
    {
	key = textprefixstr();
	key += idx;

	const char* text = par.find( key );
	if ( !text ) return -1;

	setText( idx, text );
    }

    bool yn = true;
    par.getYN( showtextstr(), yn );
    showText( yn );

    yn = true;
    par.getYN( showscalestr(), yn );
    showScale( yn );

    return 1;
}

}; //namespace
