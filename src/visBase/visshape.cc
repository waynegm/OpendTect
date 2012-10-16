/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2003
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visshape.h"

#include "errh.h"
#include "iopar.h"
#include "viscoord.h"
#include "visdataman.h"
#include "visdetail.h"
#include "visevent.h"
#include "visforegroundlifter.h"
#include "vismaterial.h"
#include "visnormals.h"
#include "vistexture2.h"
#include "vistexture3.h"
#include "vistexturecoords.h"

#include "Inventor/nodes/SoIndexedTriangleStripSet.h"
#include "Inventor/nodes/SoMaterialBinding.h"
#include "Inventor/nodes/SoNormalBinding.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoShapeHints.h"
#include "Inventor/nodes/SoSwitch.h"

#include "SoIndexedTriangleFanSet.h"

#include <osg/PrimitiveSet>
#include <osg/Switch>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Material>


namespace visBase
{

const char* Shape::sKeyOnOff() 			{ return  "Is on";	}
const char* Shape::sKeyTexture() 		{ return  "Texture";	}
const char* Shape::sKeyMaterial() 		{ return  "Material";	}


    
Shape::Shape( SoNode* shape )
    : shape_( shape )
    , onoff_( 0 )
    , texture2_( 0 )
    , texture3_( 0 )
    , material_( 0 )
    , root_( 0 )
    , materialbinding_( 0 )
    , lifter_( 0 )
    , lifterswitch_( 0 )
    , osgswitch_( new osg::Switch )
{
    
    osgswitch_->ref();
}


Shape::~Shape()
{
    if ( texture2_ ) texture2_->unRef();
    if ( texture3_ ) texture3_->unRef();
    if ( material_ ) material_->unRef();

    if ( getInventorNode() ) getInventorNode()->unref();
    if ( lifter_ ) lifter_->unRef();
    if ( osgswitch_ ) osgswitch_->unref();
}


void Shape::turnOnForegroundLifter( bool yn )
{ lifterswitch_->whichChild = yn ? 0 : SO_SWITCH_NONE; }


void Shape::turnOn(bool n)
{
    if ( osgswitch_ )
    {
	if ( n ) osgswitch_->setAllChildrenOn();
	else osgswitch_->setAllChildrenOff();
    }
    else
    {
	pErrMsg( "Turning off object without switch");
    }
}


bool Shape::isOn() const
{
    return !osgswitch_ ||
	   (osgswitch_->getNumChildren() && osgswitch_->getValue(0) );
}
    
    
void Shape::removeSwitch()
{
    if ( root_ )
    {
	root_->ref();
	onoff_->unref();
	onoff_ = 0;
    }
}


void Shape::setRenderCache(int mode)
{
    if ( !mode )
	root_->renderCaching = SoSeparator::OFF;
    else if ( mode==1 )
	root_->renderCaching = SoSeparator::ON;
    else
	root_->renderCaching = SoSeparator::AUTO;
}


int Shape::getRenderCache() const
{
    if ( root_->renderCaching.getValue()==SoSeparator::OFF )
	return 0;

    if ( root_->renderCaching.getValue()==SoSeparator::ON )
	return 1;
    
    return 2;
}


#define mDefSetGetItem(ownclass, clssname, variable, osgremove, osgset ) \
void ownclass::set##clssname( clssname* newitem ) \
{ \
    if ( variable ) \
    { \
	if ( variable->getInventorNode() ) \
	    removeNode( variable->getInventorNode() ); \
	else \
	{ osgremove; } \
	variable->unRef(); \
	variable = 0; \
    } \
 \
    if ( newitem ) \
    { \
	variable = newitem; \
	variable->ref(); \
	if ( variable->getInventorNode() ) \
	    insertNode( variable->getInventorNode() ); \
	else \
	{ osgset; } \
    } \
} \
 \
 \
clssname* ownclass::gt##clssname() const \
{ \
    return const_cast<clssname*>( variable ); \
}


mDefSetGetItem( Shape, Texture2, texture2_, , );
mDefSetGetItem( Shape, Texture3, texture3_, , );
mDefSetGetItem( Shape, Material, material_, , );


void Shape::setMaterialBinding( int nv )
{
    mDynamicCastGet( const IndexedShape*, isindexed, this );

    if ( !materialbinding_ )
    {
	materialbinding_ = new SoMaterialBinding;
	insertNode( materialbinding_ );
    }
    if ( nv==cOverallMaterialBinding() )
    {
	materialbinding_->value = SoMaterialBinding::OVERALL;
    }
    else if ( nv==cPerFaceMaterialBinding() )
    {
	materialbinding_->value = isindexed ?
	    SoMaterialBinding::PER_FACE_INDEXED : SoMaterialBinding::PER_FACE;
    }
    else if ( nv==cPerVertexMaterialBinding() )
    {
	materialbinding_->value = isindexed
	    ? SoMaterialBinding::PER_VERTEX_INDEXED
	    : SoMaterialBinding::PER_VERTEX;
    }
    else if ( nv==cPerPartMaterialBinding() )
    {
	materialbinding_->value = isindexed
	    ? SoMaterialBinding::PER_PART_INDEXED
	    : SoMaterialBinding::PER_PART;
    }
}


int Shape::getMaterialBinding() const
{
    if ( !materialbinding_ ) return cOverallMaterialBinding();

    if (materialbinding_->value.getValue()==SoMaterialBinding::PER_FACE ||
	materialbinding_->value.getValue()==SoMaterialBinding::PER_FACE_INDEXED)
	return cPerFaceMaterialBinding();

    if (materialbinding_->value.getValue()==SoMaterialBinding::PER_PART ||
	materialbinding_->value.getValue()==SoMaterialBinding::PER_PART_INDEXED)
	return cPerPartMaterialBinding();
    
    if (materialbinding_->value.getValue()==SoMaterialBinding::PER_VERTEX ||
	materialbinding_->value.getValue()==
	    SoMaterialBinding::PER_VERTEX_INDEXED)
	return cPerVertexMaterialBinding();

    return cOverallMaterialBinding();
}


void Shape::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    VisualObject::fillPar( iopar, saveids );

    if ( material_ )
	iopar.set( sKeyMaterial(), material_->id() );

    int textureindex = -1;
    if ( texture2_ )
	textureindex = texture2_->id();
    else if ( texture3_ )
	textureindex = texture3_->id();

    if ( textureindex != -1 )
    {
	iopar.set( sKeyTexture(), textureindex );
	if ( saveids.indexOf(textureindex) == -1 )
	    saveids += textureindex;
    }

    iopar.setYN( sKeyOnOff(), isOn() );
}


int Shape::usePar( const IOPar& par )
{
    int res = VisualObject::usePar( par );
    if ( res!=1 ) return res;

    bool ison;
    if ( par.getYN( sKeyOnOff(), ison) )
	turnOn( ison );

    int textureindex;
    if ( par.get(sKeyTexture(),textureindex) && textureindex!=-1 )
    {
	if ( !DM().getObject(textureindex) )
	    return 0;

	Texture2* t2 = dynamic_cast<Texture2*>(DM().getObject(textureindex));
	Texture3* t3 = dynamic_cast<Texture3*>(DM().getObject(textureindex));

	if ( t2 ) setTexture2( t2 );
	else if ( t3 ) setTexture3( t3 );
	else return -1;
    }

    return 1;
}

	
SoNode* Shape::gtInvntrNode()
{ return onoff_ ? (SoNode*) onoff_ : (SoNode*) root_; }
    
    
osg::Node* Shape::gtOsgNode()
{ return osgswitch_; }


void Shape::insertNode( SoNode*  node )
{
    if ( root_ )
	root_->insertChild( node, 0 );
}


void Shape::replaceShape( SoNode* node )
{
    removeNode( shape_ );
    root_->addChild( node );
}


void Shape::removeNode( SoNode* node )
{
    while ( root_->findChild( node ) != -1 )
	root_->removeChild( node );
}

#define mVertexShapeConstructor( shp, geode ) \
    Shape( shp ) \
    , normals_( 0 ) \
    , coords_( 0 ) \
    , texturecoords_( 0 ) \
    , normalbinding_( 0 ) \
    , shapehints_( 0 ) \
    , geode_( geode ) \
    , node_( 0 ) \
    , osggeom_( 0 )
    
VertexShape::VertexShape( SoVertexShape* shape )
    : mVertexShapeConstructor( shape, 0 )
    , primitivetype_( Geometry::PrimitiveSet::Other )
{
    setCoordinates( Coordinates::create() );
}
    
    
VertexShape::VertexShape( Geometry::IndexedPrimitiveSet::PrimitiveType tp,
			  bool creategeode )
    : mVertexShapeConstructor( 0, creategeode ? new osg::Geode : 0 )
    , primitivetype_( tp )
{
    
    if ( geode_ )
    {
	geode_->ref();
	osggeom_ = new osg::Geometry;
	geode_->addDrawable( osggeom_ );
	osgswitch_->addChild( geode_ );
	node_ = geode_;
	
	if ( primitivetype_==Geometry::PrimitiveSet::Lines ||
	    primitivetype_==Geometry::PrimitiveSet::LineStrips )
	{
	    osggeom_->getOrCreateStateSet()->setMode( GL_LIGHTING,
						     osg::StateAttribute::OFF );
	}
    }
    
    setCoordinates( Coordinates::create() );
}


VertexShape::~VertexShape()
{
    if ( node_ ) node_->unref();
    if ( normals_ ) normals_->unRef();
    if ( coords_ ) coords_->unRef();
    if ( texturecoords_ ) texturecoords_->unRef();
    
    deepUnRef( primitivesets_ );
}
    
    
void VertexShape::removeSwitch()
{
    if ( osgswitch_ )
    {
	osgswitch_->unref();
	osgswitch_ = 0;
    }
    else
    {
	Shape::removeSwitch();
    }
}
    
    
void VertexShape::dirtyCoordinates()
{
    if ( osggeom_ )
    {
	osggeom_->dirtyDisplayList();
	osggeom_->dirtyBound();
    }
}

    
osg::Node* VertexShape::gtOsgNode()
{
    return osgswitch_ ? (osg::Node*) osgswitch_ : (osg::Node*) node_;
}
    


void VertexShape::setDisplayTransformation( const mVisTrans* tr )
{ coords_->setDisplayTransformation( tr ); }


const mVisTrans* VertexShape::getDisplayTransformation() const
{ return  coords_->getDisplayTransformation(); }


mDefSetGetItem( VertexShape, Coordinates, coords_,
if ( osggeom_ ) osggeom_->setVertexArray(0),
if ( osggeom_ ) osggeom_->setVertexArray(mGetOsgVec3Arr( coords_->osgArray())));
    
mDefSetGetItem( VertexShape, Normals, normals_,
if ( osggeom_ ) osggeom_->setNormalArray( 0 ),
if ( osggeom_ ) osggeom_->setNormalArray(mGetOsgVec3Arr(normals_->osgArray())));
    
mDefSetGetItem( VertexShape, TextureCoords, texturecoords_,
if ( osggeom_ ) osggeom_->setTexCoordArray( 0, 0 ),
if ( osggeom_ ) osggeom_->setTexCoordArray( 0,
mGetOsgVec2Arr(texturecoords_->osgArray())));



void VertexShape::setNormalPerFaceBinding( bool nv )
{
    mDynamicCastGet( const IndexedShape*, isindexed, this );

    if ( !normalbinding_ )
    {
	normalbinding_ = new SoNormalBinding;
	insertNode( normalbinding_ );
    }
    if ( nv )
    {
	normalbinding_->value = isindexed ?
	    SoNormalBinding::PER_FACE_INDEXED : SoNormalBinding::PER_FACE;
    }
    else
    {
	normalbinding_->value = isindexed
	    ? SoNormalBinding::PER_VERTEX_INDEXED
	    : SoNormalBinding::PER_VERTEX;
    }
}
    
    
bool VertexShape::getNormalPerFaceBinding() const
{
    if ( !normalbinding_ ) return true;
    return normalbinding_->value.getValue()==SoNormalBinding::PER_FACE ||
	   normalbinding_->value.getValue()==SoNormalBinding::PER_FACE_INDEXED;
}


#define mCheckCreateShapeHints() \
    return;

void VertexShape::setVertexOrdering( int nv )
{
}


int VertexShape::getVertexOrdering() const
{
    return cUnknownVertexOrdering();
}


void VertexShape::setFaceType( int ft )
{
}


int VertexShape::getFaceType() const
{
    return cUnknownFaceType();
}


void VertexShape::setShapeType( int st )
{
}


int VertexShape::getShapeType() const
{
    return cUnknownShapeType();
}


IndexedShape::IndexedShape( SoIndexedShape* shape )
    : VertexShape( shape )
    , indexedshape_( shape )
{}
    
    
SoIndexedShape* createSoClass( Geometry::PrimitiveSet::PrimitiveType tp )
{
    switch ( tp ) {
	case Geometry::PrimitiveSet::TriangleStrip:
	    return new SoIndexedTriangleStripSet;
	case Geometry::PrimitiveSet::TriangleFan:
	    return new SoIndexedTriangleStripSet;
	    
	default:
	    break;
    }
    
    return 0;
}
    
    
IndexedShape::IndexedShape( Geometry::IndexedPrimitiveSet::PrimitiveType tp )
    : VertexShape( tp, true )
{
    indexedshape_ = (SoIndexedShape*) shape_;
}


void IndexedShape::replaceShape( SoNode* node )
{
    mDynamicCastGet( SoIndexedShape*, is, node );
    if ( !is ) return;

    indexedshape_ = is;
    Shape::replaceShape( node );
}


#define setGetIndex( resourcename, fieldname )  \
int IndexedShape::nr##resourcename##Index() const \
{ return indexedshape_->fieldname.getNum(); } \
 \
 \
void IndexedShape::set##resourcename##Index( int pos, int idx ) \
{ indexedshape_->fieldname.set1Value( pos, idx ); } \
 \
 \
void IndexedShape::remove##resourcename##IndexAfter(int pos) \
{  \
    if ( indexedshape_->fieldname.getNum()>pos+1 ) \
	indexedshape_->fieldname.deleteValues(pos+1); \
} \
 \
 \
int IndexedShape::get##resourcename##Index( int pos ) const \
{ return indexedshape_->fieldname[pos]; } \
 \
 \
void IndexedShape::set##resourcename##Indices( const int* ptr, int sz ) \
{ return indexedshape_->fieldname.setValuesPointer( sz, ptr ); } \
\
void IndexedShape::set##resourcename##Indices( const int* ptr, int sz, \
					       int start ) \
{ return indexedshape_->fieldname.setValues( start, sz, ptr ); } \


setGetIndex( Coord, coordIndex );
setGetIndex( TextureCoord, textureCoordIndex );
setGetIndex( Normal, normalIndex );
setGetIndex( Material, materialIndex );


void IndexedShape::copyCoordIndicesFrom( const IndexedShape& is )
{
    indexedshape_->coordIndex = is.indexedshape_->coordIndex;
}


int IndexedShape::getClosestCoordIndex( const EventInfo& ei ) const
{
    pErrMsg( "Not implemented in osg. Needed?");
    return -1;
}
    

class OSGPrimitiveSet
{
public:
    virtual osg::PrimitiveSet*	getPrimitiveSet()	= 0;
    
    static GLenum	getGLEnum(Geometry::PrimitiveSet::PrimitiveType tp)
    {
	switch ( tp )
	{
	    case Geometry::PrimitiveSet::Triangles:
		return GL_TRIANGLES;
	    case Geometry::PrimitiveSet::TriangleFan:
		return GL_TRIANGLE_FAN;
	    case Geometry::PrimitiveSet::TriangleStrip:
		return GL_TRIANGLE_STRIP;
	    case Geometry::PrimitiveSet::Lines:
		return GL_LINES;
	    case Geometry::PrimitiveSet::Points:
		return GL_POINTS;
	    case Geometry::PrimitiveSet::LineStrips:
		return GL_LINE_STRIP;
	    default:
		break;
	}
	
	return GL_POINTS;
    }
};
    
    
void VertexShape::addPrimitiveSet( Geometry::PrimitiveSet* p )
{
    p->ref();
    p->setPrimitiveType( primitivetype_ );
    
    mDynamicCastGet(OSGPrimitiveSet*, osgps, p );
    addPrimitiveSetToScene( osgps->getPrimitiveSet() );
    
    primitivesets_ += p;
}
    
    
void VertexShape::removePrimitiveSet( const Geometry::PrimitiveSet* p )
{
    const int pidx = primitivesets_.indexOf( p );
    mDynamicCastGet( OSGPrimitiveSet*, osgps,primitivesets_[pidx]  );
    removePrimitiveSetFromScene( osgps->getPrimitiveSet() );
    
    primitivesets_.remove( pidx )->unRef();
}
    
    
void VertexShape::addPrimitiveSetToScene( osg::PrimitiveSet* ps )
{
    osggeom_->addPrimitiveSet( ps );
}
    
    
int VertexShape::nrPrimitiveSets() const
{ return primitivesets_.size(); }


Geometry::PrimitiveSet* VertexShape::getPrimitiveSet( int idx )
{
    return primitivesets_[idx];
}
    
    
void VertexShape::removePrimitiveSetFromScene( const osg::PrimitiveSet* ps )
{
    const int idx = osggeom_->getPrimitiveSetIndex( ps );
    osggeom_->removePrimitiveSet( idx );
}

#define mImplOsgFuncs \
osg::PrimitiveSet* getPrimitiveSet() { return element_.get(); } \
void setPrimitiveType( Geometry::PrimitiveSet::PrimitiveType tp ) \
{ \
    Geometry::PrimitiveSet::setPrimitiveType( tp ); \
    element_->setMode( getGLEnum( getPrimitiveType() )); \
}

    
template <class T>
class OSGIndexedPrimitiveSet : public Geometry::IndexedPrimitiveSet,
			       public OSGPrimitiveSet
{
public:
			OSGIndexedPrimitiveSet()
    			    : element_( new T ) {}
    
			mImplOsgFuncs
    virtual void	setEmpty()
    			{ element_->erase(element_->begin(), element_->end() ); }
    virtual void	append( int ) {}
    virtual int		pop() { return 0; }
    virtual int		size() const { return 0; }
    virtual int		get(int) const { return 0; }
    virtual int		set(int,int) { return 0; }
    void		set(const int* ptr, int num)
    {
	element_->clear();
	element_->reserve( num );
	for ( int idx=0; idx<num; idx++, ptr++ )
	    element_->push_back( *ptr );
    }
    void		append(const int* ptr, int num)
    {
	element_->reserve( size()+num );
	for ( int idx=0; idx<num; idx++, ptr++ )
	    element_->push_back( *ptr );
    }

    
    osg::ref_ptr<T>	element_;
};

    
class OSGRangePrimitiveSet : public Geometry::RangePrimitiveSet,
			     public OSGPrimitiveSet
{
public:
			OSGRangePrimitiveSet()
			    : element_( new osg::DrawArrays )
			{}
    
			mImplOsgFuncs
    
    int			size() const 	   { return element_->getCount();}
    int			get(int idx) const { return element_->getFirst()+idx;}

    void		setRange( const Interval<int>& rg )
    {
	element_->setFirst( rg.start );
	element_->setCount( rg.width()+1 );
    }
    
    Interval<int>	getRange() const
    {
	const int first = element_->getFirst();
	return Interval<int>( first, first+element_->getCount()-1 );
    }
    
    osg::ref_ptr<osg::DrawArrays>	element_;
};
    
    
    

class CoinIndexedPrimitiveSet : public Geometry::IndexedPrimitiveSet
{
public:
    virtual void	setEmpty() { indices_.erase(); }
    virtual void	append( int index ) { indices_ += index; }
    virtual int		pop()
    			{
			    const int idx = size()-1;
			    if ( idx<0 ) return mUdf(int);
			    const int res = indices_[idx];
			    indices_.remove(idx);
			    return res;
			}
    
    virtual int		size() const { return indices_.size(); }
    virtual int		get(int pos) const { return indices_[pos]; }
    virtual int		set(int pos,int index)
			{ return indices_[pos] = index; }
    virtual void	set( const int* ptr,int num)
    {
	indices_ = TypeSet<int>( ptr, num );
    }
    
    virtual void	append( const int* ptr, int num )
    { indices_.append( ptr, num ); }
    
    TypeSet<int>	indices_;
};
    
    
class CoinRangePrimitiveSet : public Geometry::RangePrimitiveSet
{
public:
		   	CoinRangePrimitiveSet() : rg_(mUdf(int), mUdf(int) ) {}
    
    int			size() const 			{ return rg_.width()+1;}
    int			get(int idx) const 		{ return rg_.start+idx;}
    void		setRange(const Interval<int>& rg) { rg_ = rg; }
    Interval<int>	getRange() const 		{ return rg_; }
    
    Interval<int>	rg_;
};
    

Geometry::PrimitiveSet*
    PrimitiveSetCreator::doCreate( bool indexed, bool large )
{
    if ( indexed )
	return large
	       ? (Geometry::IndexedPrimitiveSet*)
		new OSGIndexedPrimitiveSet<osg::DrawElementsUInt>
	       : (Geometry::IndexedPrimitiveSet*)
		new OSGIndexedPrimitiveSet<osg::DrawElementsUShort>;
    
    return new OSGRangePrimitiveSet;
}
    


    
} // namespace visBase
