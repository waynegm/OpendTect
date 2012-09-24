/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID mUnusedVar = "$Id$";

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

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoIndexedLineSet.h"
#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoNormal.h"
#include "Inventor/nodes/SoNormalBinding.h"
#include "Inventor/nodes/SoSwitch.h"

#include "SoOD.h"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Switch>

#include <osgGeo/OneSideSwitch>

mCreateFactoryEntry( visBase::Annotation );

namespace visBase
{

const char* Annotation::textprefixstr()	    { return "Text "; }
const char* Annotation::cornerprefixstr()   { return "Corner "; }
const char* Annotation::showtextstr()	    { return "Show Text"; }
const char* Annotation::showscalestr()	    { return "Show Scale"; }

Annotation::Annotation()
    : VisualObjectImpl( false )
    , coords_(new SoCoordinate3)
    , pickstyle_(PickStyle::create())
    , texts_(0)
    , box_( new osg::Geode )
{
    annotscale_[0] = annotscale_[1] = annotscale_[2] = 1;

    annotcolor_ = Color::White();
    pickstyle_->ref();
    addChild( pickstyle_->getInventorNode() );
    pickstyle_->setStyle( PickStyle::Unpickable );

    addChild( coords_ );

    float pos[8][3] =
    {
	 { 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 0 },
	 { 1, 0, 0 }, { 1, 0, 1 }, { 1, 1, 1 }, { 1, 1, 0 }
    };

    const osg::Vec3* ptr = (osg::Vec3*) pos;
    osg::ref_ptr<osg::Vec3Array> coords = new osg::Vec3Array( 8, ptr );
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;

    geometry->setVertexArray( coords );

    GLubyte indices[] = { 0, 1, 1, 2, 2, 3, 3, 0,
			   4, 5, 5, 6, 6, 7, 7, 4,
			   0, 4, 1, 5, 2, 6, 3, 7 };
    geometry->addPrimitiveSet(
	    new osg::DrawElementsUByte( GL_LINES, 24, indices  ) );
    
    geometry->setColorBinding( osg::Geometry::BIND_OVERALL );

    box_->addDrawable( geometry );
    addChild( box_ );
    
    texts_ = DataObjectGroup::create();
    texts_->ref();
    addChild( texts_->osgNode() );
    
#define mAddText \
    text = Text2::create(); text->setJustification( Text2::Right ); \
    texts_->addObject( text );

    Text2* text = 0; mAddText mAddText mAddText

    scalegroup_ = new osg::Group;
    scalegroup_->ref();
    addChild( scalegroup_ );
    
    DataObjectGroup* scale = DataObjectGroup::create();
    scale->ref();
    scalegroup_->addChild( scale->osgNode() );
    scales_ += scale;
    
    scale = DataObjectGroup::create();
    scale->ref();
    scalegroup_->addChild( scale->osgNode() );
    scales_ += scale;
    
    scale = DataObjectGroup::create();
    scale->ref();
    scalegroup_->addChild( scale->osgNode() );
    scales_ += scale;
    
    gridlines_ = new osg::Geode;
    gridlines_->ref();
    
    osg::ref_ptr<osg::Geometry> gridgeom = new osg::Geometry;
    gridlines_->addDrawable( gridgeom );
    
    osg::ref_ptr<osg::Vec3Array> gridlinecoords = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> gridlinenormals = new osg::Vec3Array;

    gridgeom->setVertexArray( gridlinecoords );
    gridgeom->setNormalArray( gridlinenormals );

    updateTextPos();
}


Annotation::~Annotation()
{
    scalegroup_->unref();
    gridlines_->unref();
    
    scales_[0]->unRef();
    scales_[1]->unRef();
    scales_[2]->unRef();
    texts_->unRef();
    pickstyle_->unRef();
}


#define mImplSwitches( str, node ) \
void Annotation::show##str( bool yn ) \
{ \
    if ( yn==is##str##Shown() ) \
	return; \
    if ( yn ) \
	addChild( node ); \
    else \
	removeChild( node ); \
} \
 \
 \
bool Annotation::is##str##Shown() const \
{ \
    return childIndex( node ) !=-1; \
}
    
    
mImplSwitches( Text, texts_->osgNode() );
mImplSwitches( Scale, scalegroup_ );
mImplSwitches( GridLines, gridlines_ );


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
    if ( box_ && box_->getNumDrawables() )
    {
	 osg::ref_ptr<osg::Geometry> geometry =
	     (osg::Geometry*) box_->getDrawable( 0 );

	 osg::Vec3& coord =
	     ((osg::Vec3*) geometry->getVertexArray()->getDataPointer())[idx];

	 coord = osg::Vec3f( x, y, z );
    }


    float c[3] = { x, y, z };
    coords_->point.setValues( idx, 1, &c );
}


Coord3 Annotation::getCorner( int idx ) const
{
    const SbVec3f pos = coords_->point[idx];
    Coord3 res( pos[0], pos[1], pos[2] );
    return res;
}


void Annotation::setText( int dim, const char* string )
{
    Text2* text = (Text2*)texts_->getObject( dim );
    if ( text )
	text->setText( string );
}


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


void Annotation::updateTextPos()
{
    updateTextPos( 0 );
    updateTextPos( 1 );
    updateTextPos( 2 );
}


void Annotation::updateGridLines()
{
    /*
    osg::ref_ptr<osg::Geometry> geometry =
	(osg::Geometry*) gridlines_->getDrawable( 0 );
    
    osg::Vec3Array* normals = mGetOsgVec3Arr( geometry->getNormalArray() );
    osg::Vec3Array* coords = mGetOsgVec3Arr( geometry->getVertexArray() );

    
    if ( !gridlines_.size() )
	return;

    for ( int idx=0; idx<gridlines_.size(); idx++ )
	gridlines_[idx]->coordIndex.setNum( 0 );

    gridlinecoords_->point.setNum( 0 );

    updateGridLines( 0 );
    updateGridLines( 1 );
    updateGridLines( 2 );
    
    void Annotation::updateGridLines(int dim)
    {
	SbVec3f p0;
	SbVec3f p1;
	getAxisCoords( dim, p0, p1 );
	Interval<float> range( p0[dim], p1[dim] );
	const SamplingData<float> sd = AxisLayout<float>( range ).sd_;
	
	SbVec3f corners[4];
	int gridlineidxs[4];
	if ( dim==0 )
	{
	    corners[0] = coords_->point[ 0 ];
	    corners[1] = coords_->point[ 3 ];
	    corners[2] = coords_->point[ 7 ];
	    corners[3] = coords_->point[ 4 ];
	    gridlineidxs[0] = 0; gridlineidxs[1] = 3;
	    gridlineidxs[2] = 1; gridlineidxs[3] = 2;
	}
	else if ( dim==1 )
	{
	    corners[0] = coords_->point[ 0 ];
	    corners[1] = coords_->point[ 1 ];
	    corners[2] = coords_->point[ 5 ];
	    corners[3] = coords_->point[ 4 ];
	    gridlineidxs[0] = 0; gridlineidxs[1] = 5;
	    gridlineidxs[2] = 1; gridlineidxs[3] = 4;
	}
	else
	{
	    corners[0] = coords_->point[ 0 ];
	    corners[1] = coords_->point[ 1 ];
	    corners[2] = coords_->point[ 2 ];
	    corners[3] = coords_->point[ 3 ];
	    gridlineidxs[0] = 2; gridlineidxs[1] = 5;
	    gridlineidxs[2] = 3; gridlineidxs[3] = 4;
	}
	
	int ci = gridlinecoords_->point.getNum();
	
	for ( int idx=0; ; idx++ )
	{
	    const float val = sd.atIndex(idx);
	    if ( val <= range.start )	continue;
	    else if ( val > range.stop )	break;
	    
	    corners[0][dim]=corners[1][dim]=corners[2][dim]=corners[3][dim] = val;
	    gridlinecoords_->point.setValues( ci, 4, corners );
	    SoIndexedLineSet* lineset;
	    
#define mAddOneLine( c1, c2 ) \
lineset = gridlines_[gridlineidxs[c1]]; \
lineset->coordIndex.set1Value( lineset->coordIndex.getNum(), ci+c1 ); \
lineset->coordIndex.set1Value( lineset->coordIndex.getNum(), ci+c2 ); \
lineset->coordIndex.set1Value( lineset->coordIndex.getNum(), -1 )
	    
	    mAddOneLine( 0, 1 );
	    mAddOneLine( 1, 2 );
	    mAddOneLine( 2, 3 );
	    mAddOneLine( 3, 0 );
	    
	    ci += 4;
	}
    }
    
    */


}


void Annotation::getAxisCoords( int dim, SbVec3f& p0, SbVec3f& p1 ) const
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

    p0 = coords_->point[pidx0];
    p1 = coords_->point[pidx1];
}


void Annotation::updateTextPos( int dim )
{
    SbVec3f p0;
    SbVec3f p1;
    getAxisCoords( dim, p0, p1 );

    SbVec3f tp;
    tp[0] = (p0[0]+p1[0]) / 2;
    tp[1] = (p0[1]+p1[1]) / 2;
    tp[2] = (p0[2]+p1[2]) / 2;

    ((Text2*)texts_->getObject(dim))
			->setPosition( Coord3(tp[0],tp[1],tp[2]) );

    Interval<float> range( p0[dim], p1[dim] );
    const SamplingData<float> sd = AxisLayout<float>( range ).sd_;
    scales_[dim]->removeAll();

    for ( int idx=0; ; idx++ )
    {
	float val = sd.atIndex(idx);
	if ( val <= range.start )	continue;
	else if ( val > range.stop )	break;

	Text2* text = Text2::create();
	scales_[dim]->addObject( text );
	Coord3 pos( p0[0], p0[1], p0[2] );
	pos[dim] = val;
	float displayval = val;
	displayval *= annotscale_[dim];

	text->setPosition( pos );
	text->setText( toString(displayval) );
	text->getMaterial()->setColor( annotcolor_ );
    }
}


void visBase::Annotation::setAnnotScale( int dim, int nv )
{
    annotscale_[dim] = nv;
    updateTextPos( dim );
}


Text2* visBase::Annotation::getText( int dim, int textnr )
{
    DataObjectGroup* group = 0;
    group = scales_[dim];
    mDynamicCastGet(Text2*,text,group->getObject(textnr));
    return text;
}


void Annotation::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( par, saveids );

    BufferString key;
    for ( int idx=0; idx<8; idx++ )
    {
	key = cornerprefixstr();
	key += idx;
	Coord3 pos = getCorner( idx );
	par.set( key, pos.x, pos.y, pos.z );
    }

    for ( int idx=0; idx<3; idx++ )
    {
	key = textprefixstr();
	key += idx;
	Text2* text = (Text2*)texts_->getObject( idx );
	if ( !text ) continue;

	par.set( key, (const char*)text->getText() );
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
