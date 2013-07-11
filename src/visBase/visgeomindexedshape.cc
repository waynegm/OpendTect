/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          August 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "visgeomindexedshape.h"

#include "datapointset.h"
#include "datacoldef.h"
#include "posvecdataset.h"
#include "indexedshape.h"
#include "viscoord.h"
#include "vismaterial.h"
#include "visnormals.h"
#include "vistexturecoords.h"
#include "visdrawstyle.h"

#include <osg/Geometry>
#include <osg/Geode>
#include <osg/UserDataContainer>
#include <osg/LightModel>

#include "vistransform.h"

#define mNrMaterials		256
#define mNrMaterialSteps	255
#define mUndefMaterial		255

mCreateFactoryEntry( visBase::GeomIndexedShape );

namespace visBase
{

GeomIndexedShape::GeomIndexedShape()
    : VisualObjectImpl( true )
    , shape_( 0 )
    , lineradius_( -1 )
    , lineconstantonscreen_( false )
    , linemaxsize_( -1 )
    , vtexshape_( VertexShape::create() )
    , colorhandler_( new ColorHandler )
    , colortableenabled_( false )
    , singlematerial_( new Material )
    , coltabmaterial_( new Material )
{
    singlematerial_->ref();
    coltabmaterial_->ref();

    vtexshape_->ref();
    addChild( vtexshape_->osgNode() );

    vtexshape_->setMaterial( singlematerial_ );
    singlematerial_->setColorMode( visBase::Material::Off );
    coltabmaterial_->setColorMode( visBase::Material::Diffuse );
    vtexshape_->setPrimitiveType( Geometry::PrimitiveSet::Triangles );

    renderOneSide( 0 );

    if ( getMaterial() )
	getMaterial()->change.notify( mCB(this,GeomIndexedShape,matChangeCB) );
}


GeomIndexedShape::~GeomIndexedShape()
{
    singlematerial_->unRef();
    coltabmaterial_->unRef();
    delete colorhandler_;

    vtexshape_->unRef();

    if ( getMaterial() )
	getMaterial()->change.remove( mCB(this,GeomIndexedShape,matChangeCB) );

}


GeomIndexedShape::ColorHandler::ColorHandler()
    : material_( new visBase::Material )
    , attributecache_( 0 )
{
    material_->ref();
}


GeomIndexedShape::ColorHandler::~ColorHandler()
{
    material_->unRef();
}


void GeomIndexedShape::renderOneSide( int side )
{
    renderside_ = side;

    if ( !renderside_ )
    {
	vtexshape_->setTwoSidedLight( false );
    }
    else
    {
	pErrMsg("Not implemented");
    }
}


void GeomIndexedShape::setMaterial( Material* mat )
{
    if ( !vtexshape_  || !mat ) return;

    if ( getMaterial() )
	getMaterial()->change.notify( mCB(this,GeomIndexedShape,matChangeCB) );

    colorhandler_->material_->setPropertiesFrom( *mat );

    mat->change.notify( mCB(this,GeomIndexedShape,matChangeCB) );
}


void GeomIndexedShape::updateMaterialFrom( const Material* mat )
{
    if ( !mat ) return;

    singlematerial_->setFrom( *mat);

    if ( isColTabEnabled() && colorhandler_ )
    {
	colorhandler_->material_->setPropertiesFrom( *mat );
	mapAttributeToColorTableMaterial();
    }
    enableColTab( colortableenabled_ );
}


void GeomIndexedShape::matChangeCB( CallBacker* )
{
    updateMaterialFrom( getMaterial() );
}


void GeomIndexedShape::updateGeometryMaterial() 
{
    if ( getMaterial() )
    {
	colorhandler_->material_->setPropertiesFrom( *getMaterial() );
	mapAttributeToColorTableMaterial();
	vtexshape_->setMaterial( coltabmaterial_ );
    }
}


void GeomIndexedShape::enableColTab( bool yn )
{
    if ( !vtexshape_->getMaterial() ) return;
    
    if ( yn )
	    vtexshape_->setMaterial( coltabmaterial_ );
    else 
	vtexshape_->setMaterial( singlematerial_ );

    colortableenabled_  = yn;
}


bool GeomIndexedShape::isColTabEnabled() const
{
    return colortableenabled_;
}


void GeomIndexedShape::setDataMapper( const ColTab::MapperSetup& setup,
				      TaskRunner* tr )
{
    if ( setup!=colorhandler_->mapper_.setup_ )
    {
	colorhandler_->mapper_.setup_ = setup;
	if ( setup.type_!=ColTab::MapperSetup::Fixed )
	    reClip();
    }
}


const ColTab::MapperSetup* GeomIndexedShape::getDataMapper() const
{ return colorhandler_ ? &colorhandler_->mapper_.setup_ : 0; }


void GeomIndexedShape::setDataSequence( const ColTab::Sequence& seq )
{
    if ( seq!=colorhandler_->sequence_ )
    {
	colorhandler_->sequence_ = seq;
	for ( int idx=0; idx<mNrMaterialSteps; idx++ )
	{
	    const float val = ( (float) idx )/( mNrMaterialSteps-1 );
	    const Color col = seq.color( val );
	    colorhandler_->material_->setColor( col, idx+1 );
	}

	colorhandler_->material_->setColor( seq.undefColor(), mUndefMaterial+1 );
    }

   if ( isColTabEnabled() )
	updateGeometryMaterial();
}


const ColTab::Sequence* GeomIndexedShape::getDataSequence() const
{ return colorhandler_ ? &colorhandler_->sequence_ : 0; }


void GeomIndexedShape::setDisplayTransformation( const mVisTrans* nt )
{
    if ( vtexshape_->getNormals() )
    {
        vtexshape_->getNormals()->setDisplayTransformation( nt );
	if ( !renderside_ )
	    vtexshape_->getNormals()->inverse();
    }
    vtexshape_->setDisplayTransformation( nt );
    vtexshape_->dirtyCoordinates();
    vtexshape_->turnOn( true );

}


const mVisTrans* GeomIndexedShape::getDisplayTransformation() const
{ return vtexshape_->getDisplayTransformation(); }


void GeomIndexedShape::setSurface( Geometry::IndexedShape* ns, TaskRunner* tr )
{
    shape_ = ns;
    touch( false, tr );
}


bool GeomIndexedShape::touch( bool forall, TaskRunner* tr )
{
    if( !shape_ )
	return false;

    if ( !shape_->needsUpdate() ) 
	return true;

    Coordinates* coords = Coordinates::create();
    Normals* normals = Normals::create();
    TextureCoords* texturecoords = TextureCoords::create();

    shape_->setCoordList( new CoordListAdapter(*coords),
	new NormalListAdapter( *normals ), 
	new TextureCoordListAdapter( *texturecoords ) );

    shape_->getGeometry().erase();

    if( !shape_->update( forall, tr ) || !coords->size() )
	return false;

    vtexshape_->removeAllPrimitiveSets();

    vtexshape_->setCoordinates( coords );
    
    if ( normals->nrNormals() )
	vtexshape_->setNormals( normals );
    
    if ( texturecoords->size() )
        vtexshape_->setTextureCoords( texturecoords );

    ObjectSet<Geometry::IndexedGeometry>& geoms=shape_->getGeometry();

    for ( int idx=0; idx<geoms.size(); idx++ )
    {
	Geometry::IndexedGeometry* idxgeom = geoms[idx];
	if( !idxgeom || idxgeom->getCoordsPrimitiveSet()->size() == 0 )
	    continue;

	vtexshape_->addPrimitiveSet( idxgeom->getCoordsPrimitiveSet() );

	if ( idxgeom->primitivetype_ == Geometry::IndexedGeometry::Lines )
	{
	    visBase::DrawStyle* ds = 
		vtexshape_->addNodeState( new visBase::DrawStyle );
	    ds->setLineStyle( LineStyle( LineStyle::Solid, 3 ) );
	}
    }

    return true;

}


void GeomIndexedShape::getAttribPositions( DataPointSet& set,
					   mVisTrans* toinlcrltrans,
					   TaskRunner*) const
{
    const DataColDef coordindex( sKeyCoordIndex() );
    if ( set.dataSet().findColDef(coordindex,PosVecDataSet::NameExact)==-1 )
	set.dataSet().add( new DataColDef(coordindex) );

    const int col =
	set.dataSet().findColDef(coordindex,PosVecDataSet::NameExact);

    Coordinates* vtxcoords = vtexshape_->getCoordinates();
    if ( !vtxcoords || !vtxcoords->size() )
	return;

    for ( int coordid = 0; coordid<vtxcoords->size(); coordid++ )
    {
	Coord3 pos = vtxcoords->getPos( coordid );
	if ( !pos.isDefined() )
	    continue;

	mVisTrans::transform( toinlcrltrans, pos );

	DataPointSet::Pos dpsetpos( BinID(mNINT32(pos.x),mNINT32(pos.y)), 
	    (float) pos.z );
	DataPointSet::DataRow datarow( dpsetpos, 1 );
	datarow.data_.setSize( set.nrCols(), mUdf(float) );
	datarow.data_[col-set.nrFixedCols()] =  coordid;
	set.addRow( datarow );
    }

    set.dataChanged();
}


void GeomIndexedShape::setAttribData( const DataPointSet& set,TaskRunner* tr)
{
    const DataColDef coordindex( sKeyCoordIndex() );
    const int col =
	set.dataSet().findColDef(coordindex,PosVecDataSet::NameExact);

    if ( col==-1 )
	return;

    const BinIDValueSet& vals = set.bivSet();
    if ( vals.nrVals()<col+1 )
	return;

    ArrayValueSeries<float,float>& cache = colorhandler_->attributecache_;
    cache.setSize( vals.totalSize() );
    cache.setAll( mUdf(float) );

    BinIDValueSet::Pos pos;
    while ( vals.next( pos ) )
    {
	const float* ptr = vals.getVals( pos );
	const int coordidx = mNINT32( ptr[col] );
	const float val = ptr[col+1];

	if ( coordidx>=cache.size() )
	{
	    int oldsz = cache.size();
	    cache.setSize( coordidx+1 );
	    if ( !cache.arr() )
		return;

	    const float udf = mUdf( float );
	    for ( int idx=oldsz; idx<=coordidx; idx++ )
		cache.setValue( idx, udf );
	}

	cache.setValue( coordidx, val );
    }

    if ( colorhandler_->mapper_.setup_.type_!=ColTab::MapperSetup::Fixed )
	reClip();

    updateGeometryMaterial();
}


void GeomIndexedShape::mapAttributeToColorTableMaterial()
{
    if ( !colorhandler_ || colorhandler_->attributecache_.size()<=0 )
	return;

    TypeSet<Color> colors;

    for ( int idx=0; idx<vtexshape_->getCoordinates()->size(); idx++ )
    {
	const int coloridx = ColTab::Mapper::snappedPosition(
	    &colorhandler_->mapper_,colorhandler_->attributecache_[idx], 
	    mNrMaterialSteps, mUndefMaterial )+1;
	colors.add( colorhandler_->material_->getColor(coloridx ) );
    }

    coltabmaterial_->setColors( colors, false );
    coltabmaterial_->setPropertiesFrom( *colorhandler_->material_ );
}


void GeomIndexedShape::reClip()
{
    colorhandler_->mapper_.setData( 
	&colorhandler_->attributecache_, colorhandler_->attributecache_.size() );
}


void GeomIndexedShape::set3DLineRadius( float radius, bool constantonscreen,
					float maxworldsize )
{
    if ( lineradius_ != radius ||
	 lineconstantonscreen_ != constantonscreen ||
	 linemaxsize_ != maxworldsize )
    {
	lineradius_ = radius;
	lineconstantonscreen_ = constantonscreen;
	linemaxsize_ = maxworldsize;
	touch( true );
    }
}

}; // namespace visBase
