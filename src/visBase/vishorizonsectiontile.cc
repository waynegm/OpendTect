 /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Feb 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vishorizonsectiontile.h"
#include "vishorizonsection.h"
#include "vishortileresolutiondata.h"
#include "vishorizonsectiondef.h"


#include "threadwork.h"
#include "viscoord.h"
#include "vishorthreadworks.h"

#include <osgGeo/LayeredTexture>

#include <osg/Switch>
#include <osg/PrimitiveSet>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LightModel>
#include <osg/LineWidth>
#include <osg/Point>
#include <osg/PolygonOffset>
#include <osgUtil/CullVisitor>
#include <osg/UserDataContainer>

using namespace visBase;


HorizonSectionTile::HorizonSectionTile( const visBase::HorizonSection& section,
					const RowCol& origin )
    : osgswitchnode_( new osg::Switch )
    , desiredresolution_( cNoneResolution )
    , resolutionhaschanged_( false )
    , needsupdatebbox_( false )
    , usewireframe_( false )
    , nrdefinedvertices_( 0 )
    , origin_( origin )
    , hrsection_( section )
    , dispgeometrytype_( Triangle )
    , stateset_( new osg::StateSet )
    , gluevtxcoords_( new osg::Vec3Array )
    , gluetxcoords_( new osg::Vec2Array )
    , gluegeode_( new osg::Geode )
    , gluegeom_( new osg::Geometry )
    , gluenormals_ ( new osg::Vec3Array )
    , tesselationqueueid_( Threads::WorkManager::twm().addQueue(
    Threads::WorkManager::MultiThread, "Tessalation" ) )
{
    refOsgPtr( osgswitchnode_ );
    tileresolutiondata_.allowNull();
    buildOsgGeometries();
    bbox_.init();
}


void HorizonSectionTile::buildOsgGeometries()
{
    for ( int i = LEFTUPTILE; i < RIGHTBOTTOMTILE; i++ )
	neighbors_[i] = 0;

    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
    {
	tileresolutiondata_ += new TileResolutionData( this, res );
	osgswitchnode_->addChild( tileresolutiondata_[res]->osgswitch_ );
    }

    osgswitchnode_->addChild( gluegeode_ );
    gluegeode_->addDrawable( gluegeom_ );
    gluegeom_->setVertexArray( gluevtxcoords_ );
    gluegeom_->setNormalArray( gluenormals_ );
    gluegeom_->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
    gluegeom_->setDataVariance( osg::Object::DYNAMIC );
    osgswitchnode_->setAllChildrenOff();
}


HorizonSectionTile::~HorizonSectionTile()
{
    osgswitchnode_->removeChildren( 0, osgswitchnode_->getNumChildren() );
    unRefOsgPtr( osgswitchnode_ );
    unRefOsgPtr( stateset_ );

    Threads::WorkManager::twm().removeQueue( tesselationqueueid_, false );
    deepErase( tileresolutiondata_ );
}


void HorizonSectionTile::updateNormals( char res )
{
    if ( !tileresolutiondata_.validIdx( res ) ) 
	return;

    datalock_.lock();
    tileresolutiondata_[res]->calcNormals( 
	tileresolutiondata_[res]->allnormalsinvalid_  );
    datalock_.unLock();

    emptyInvalidNormalsList( res );

    tileresolutiondata_[res]->allnormalsinvalid_ = false; 
    tileresolutiondata_[res]->normals_->dirty();

}


void HorizonSectionTile::emptyInvalidNormalsList( char res )
{
    datalock_.lock();
    tileresolutiondata_[res]->invalidnormals_.erase();
    datalock_.unLock();
}


void HorizonSectionTile::setAllNormalsInvalid( char res, bool yn )
{ 
    if ( !tileresolutiondata_.validIdx( res ) ) 
	return;

    tileresolutiondata_[res]->allnormalsinvalid_ = yn; 

    if ( yn ) emptyInvalidNormalsList( res );
}


bool HorizonSectionTile::allNormalsInvalid( char res ) const
{ 
    if ( !tileresolutiondata_.validIdx( res ) ) 
	return false;

    return tileresolutiondata_[res]->allnormalsinvalid_; 
}


void HorizonSectionTile::setResolution( char res )
{ 
    desiredresolution_ = res; 
}


char HorizonSectionTile::getActualResolution() const
{
    const int numchildren = osgswitchnode_->getNumChildren();
    for ( int i=0; i<numchildren; i++ )
    {
	if ( osgswitchnode_->getValue( i ) )
	    return  tileresolutiondata_[i]->resolution_;
    }

    return cNoneResolution;
}


void HorizonSectionTile::updateAutoResolution( const osg::CullStack* cs )
{
    char newres = desiredresolution_;
    if ( newres==cNoneResolution )
    {
	updateBBox();
	if ( !bbox_.valid() )
	    newres = cNoneResolution;
	else if ( desiredresolution_==cNoneResolution )
	{
	    const char wantedres = getAutoResolution( cs );

	    if ( !tileresolutiondata_.validIdx( wantedres ) ) 
		return;
	    newres = wantedres;
	    datalock_.lock();
	    for ( ; newres<hrsection_.nrhorsectnrres_-1; newres++ )
	    {
		if ( tileresolutiondata_[newres]->needsretesselation_ < 
		    cMustRetesselate )
		    break;
	    }
	    datalock_.unLock();

	    if ( !hrsection_.tesselationlock_ && ( wantedres!=newres || 
		tileresolutiondata_[newres]->needsretesselation_ ) )
	    {
		TileTesselator* tt = new TileTesselator( this, wantedres );
		Threads::WorkManager::twm().addWork(
		    Threads::Work( *tt, true ),
		    0, tesselationqueueid_, true );
	    }
	}
    }

    setActualResolution( newres );
}


#define mIsOsgDef( pos ) (pos[2]<9.9e29)


void HorizonSectionTile::updateBBox()
{
    if ( !needsupdatebbox_ ) return;

    bbox_.init();
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
	bbox_.expandBy( tileresolutiondata_[res]->updateBBox() );

    needsupdatebbox_ = false;
}


void HorizonSectionTile::setDisplayGeometryType( unsigned int dispgeometrytype )
{ 
    dispgeometrytype_ = dispgeometrytype; 
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
	tileresolutiondata_[res]->setDisplayGeometryType( dispgeometrytype_ );
}

void HorizonSectionTile::setLineColor( Color& color)
{
    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++ )
	tileresolutiondata_[res]->setLineColor( color );
}



void HorizonSectionTile::ensureGlueTesselated()
{
    bool needgluetesselation =
	glueneedsretesselation_ || resolutionhaschanged_ ||
	(neighbors_[RIGHTTILE] && 
	neighbors_[RIGHTTILE]->resolutionhaschanged_) ||
	(neighbors_[BOTTOMTILE] && 
	neighbors_[BOTTOMTILE]->resolutionhaschanged_);
    
    if ( !needgluetesselation )
	return;

    const char res = getActualResolution();
    if( res==cNoneResolution || 
	( !neighbors_[RIGHTTILE] && !neighbors_[BOTTOMTILE] ) ) 
	return;

    for ( int nb=RIGHTTILE; nb<RIGHTBOTTOMTILE; nb += 2 )
    {
	bool isright = nb == RIGHTTILE ? true : false;
	tesselateNeigborGlue( neighbors_[nb],isright );
    }
}


#define mAddGlue( glueidx, gluepsidx ) \
    mGetOsgVec3Arr( gluevtxcoords_ )->push_back( ( *arr )[glueidx] ) ; \
    mGetOsgVec2Arr( gluetxcoords_ )->push_back( ( *tcoords )[glueidx] );\
    mGetOsgVec3Arr( gluenormals_ )->push_back( ( *normals )[glueidx] );\
    glueps_->push_back( gluepsidx );\


#define mClearGlue\
    mGetOsgVec3Arr( gluevtxcoords_ )->clear();\
    mGetOsgVec3Arr( gluenormals_ )->clear();\
    mGetOsgVec2Arr( gluetxcoords_ )->clear();\
    glueps_->clear();\


void HorizonSectionTile::tesselateNeigborGlue( HorizonSectionTile* neighbor,
					       bool rightneighbor )
{
    if ( !neighbor ) return;

    const static int cTriangleEndIdx = 3;
    const static int cTriangleBeginIdx = 1;

    const char thisres = getActualResolution();
    const char neighborres = neighbor->getActualResolution();
    if( thisres==neighborres ) return;

    HorizonSectionTile* gluetile = thisres < neighborres ? this : neighbor ;
    const char highestres = thisres < neighborres ? thisres : neighborres;

    if( highestres == cNoneResolution ) return;

    osg::StateSet* stateset = gluetile->stateset_;

    const HorizonSection& hrsection = gluetile->hrsection_;
    const int spacing = hrsection.spacing_[highestres];
    const int nrblocks = spacing == 1 ? (int)hrsection.nrcoordspertileside_
	/spacing : (int)hrsection.nrcoordspertileside_/spacing +1 ;
    const Coordinates* coords = 
	gluetile->tileresolutiondata_[highestres]->vertices_;

    const osg::Vec3Array* arr = dynamic_cast<osg::Vec3Array*>
	( gluetile->tileresolutiondata_[highestres]->vertices_->osgArray() );
    const osg::Vec3Array* normals = mGetOsgVec3Arr(
	gluetile->tileresolutiondata_[highestres]->normals_ );
    const osg::Vec2Array* tcoords = mGetOsgVec2Arr(
	gluetile->tileresolutiondata_[highestres]->txcoords_ );


    int coordidx = 0;
    int gluepsidx = 0;
    int triangleidx = 0;
    int precoordidx = 0;
    int pregluepsidx = 0;

    glueps_ = new osg::DrawElementsUShort( osg::PrimitiveSet::TRIANGLES, 0 );
    mClearGlue;

    for ( int idx=0; idx<nrblocks; idx++ )
    {
	if ( rightneighbor && gluetile==this )
	{
	    coordidx = ( idx+1 )*nrblocks;
	}
	else if ( rightneighbor && gluetile==neighbor )
	{
	    coordidx = idx == 0 ? 0 : idx*nrblocks + 1;
	}
	else if ( !rightneighbor && gluetile==this )
	{
	    const int baseidx = (nrblocks - 1)*nrblocks;
	    coordidx = baseidx + idx ;
	}
	else if ( !rightneighbor && gluetile==neighbor )
	{
	    coordidx = idx;
	}

	if( coords->isDefined( coordidx ) )
	{
	    if( triangleidx==cTriangleEndIdx )
	    {
		mAddGlue( precoordidx, pregluepsidx ) ;
		triangleidx = cTriangleBeginIdx;
	    }
	    mAddGlue( coordidx, gluepsidx );
	    precoordidx = coordidx;
	    pregluepsidx = gluepsidx;
	    gluepsidx++;
	    triangleidx ++;
	}
    }

    if ( gluegeode_ && gluegeom_ )
    {
	gluegeom_->removePrimitiveSet( 0,gluegeom_->getNumPrimitiveSets() );
	gluegeom_->addPrimitiveSet( glueps_ ) ;
	gluegeom_->setTexCoordArray( gluetile->txunit_, gluetxcoords_ );
	gluegeode_->setStateSet( stateset );
    }

}


char HorizonSectionTile::getAutoResolution( const osg::CullStack* cs ) 
{
    const static int cIdealNrPixelsPerCell = 32;

    updateBBox();
    if ( !bbox_.valid() || !cs) return cNoneResolution;

    const float screensize = cs->clampedPixelSize( osg::BoundingSphere(bbox_));
    const float nrpixels = screensize*screensize;
    const int wantednumcells = (int)( nrpixels / cIdealNrPixelsPerCell);

    if ( !wantednumcells )
	return hrsection_.lowestresidx_;

    if ( nrdefinedvertices_<=wantednumcells )
	return 0;

    char maxres = nrdefinedvertices_/wantednumcells;
    if ( nrdefinedvertices_%wantednumcells ) maxres++;

    for ( int desiredres=hrsection_.lowestresidx_; desiredres>=0; desiredres-- )
    {
	if ( hrsection_.nrcells_[desiredres]>=wantednumcells )
	    return mMIN(desiredres,maxres);
    }

    return 0;
}


void HorizonSectionTile::setActualResolution( char resolution )
{
    if ( resolution != getActualResolution() )
    {
	if ( resolution == cNoneResolution ) 
	    resolution = hrsection_.lowestresidx_;
	osgswitchnode_->setAllChildrenOff();
	osgswitchnode_->setValue( resolution, true );
	tileresolutiondata_[resolution]->setDisplayGeometryType( 
	    ( GeometryType ) dispgeometrytype_ );
	resolutionhaschanged_ = true;
    }
}


void HorizonSectionTile::tesselateResolution( char res, bool onlyifabsness )
{
    if ( !tileresolutiondata_.validIdx( res ) )  
	return;

    datalock_.lock();
    tileresolutiondata_[res]->tesselateResolution( onlyifabsness );
    datalock_.unLock();
}


void HorizonSectionTile::applyTesselation( char res )
{
    if ( !tileresolutiondata_.validIdx( res ) ) 
	return;
    osgswitchnode_->setChildValue( osgswitchnode_->getChild( res ), true );
    resolutionhaschanged_ = false;
}


void HorizonSectionTile::useWireframe( bool yn )
{ usewireframe_=yn; }


void HorizonSectionTile::setPositions( const TypeSet<Coord3>& pos )
{
    const RefMan<const Transformation> trans = hrsection_.transformation_;
    nrdefinedvertices_ = 0;
    bbox_.init();

    datalock_.lock();
    for ( char res=0; res< hrsection_.nrhorsectnrres_; res++)
    {
	tileresolutiondata_[res]->initVertices();
	tileresolutiondata_[res]->setDisplayTransformation( trans );
	tileresolutiondata_[res]->setAllVertices( pos );
	bbox_.expandBy( tileresolutiondata_[res]->bbox_ );
	tileresolutiondata_[res]->needsretesselation_ = cMustRetesselate;
	tileresolutiondata_[res]->allnormalsinvalid_ = true;
	tileresolutiondata_[res]->invalidnormals_.erase();
    }
    nrdefinedvertices_ = tileresolutiondata_[Triangle]->nrdefinedvertices_;
    datalock_.unLock();

    needsupdatebbox_ = false;

}


void HorizonSectionTile::updatePrimitiveSets()
{
    const char res = getActualResolution();
    if ( !tileresolutiondata_.validIdx( res ) )
	return;
    tileresolutiondata_[res]->updatePrimitiveSets();

}


void HorizonSectionTile::setNeighbor( int nbidx, HorizonSectionTile* nb )
{ 
    neighbors_[nbidx] = nb;
    glueneedsretesselation_ = true;
}


void HorizonSectionTile::setPos( int row, int col, const Coord3& pos )
    // not implemented yet due to polygonselection
{
    for ( char res = 0; res < hrsection_.nrhorsectnrres_; )
	tileresolutiondata_[res]->setSingleVertex( row, col, pos );
    glueneedsretesselation_ = true;
}


void HorizonSectionTile::setInvalidNormals( int row, int col )
    // not implemented yet due to polygonselection 
{ 

}


void HorizonSectionTile::setTexture( const Coord& origincrd, 
    const Coord& oppositecrd )
{
    osg::Vec2f origin = Conv::to<osg::Vec2f>( origincrd );  
    osg::Vec2f opposite = Conv::to<osg::Vec2f>( oppositecrd ); 

    std::vector<osgGeo::LayeredTexture::TextureCoordData> tcdata;
    const osgGeo::LayeredTexture* texture = hrsection_.getOsgTexture();

    unRefOsgPtr( stateset_ );
	stateset_  = texture->createCutoutStateSet( origin,opposite, tcdata );
    refOsgPtr( stateset_ );

    const std::vector<osgGeo::LayeredTexture::TextureCoordData>::iterator tcit= 
	tcdata.begin();

    if ( !tcdata.size() ) return;    

    const osg::Vec2 texturetilestep = tcit->_tc11 - tcit->_tc00;

    for ( char res=0; res<hrsection_.nrhorsectnrres_; res++)
    {
	const int spacing = hrsection_.spacing_[res];
	const int size = spacing == 1 ? 
	    (int)hrsection_.nrcoordspertileside_/spacing :
	    (int)hrsection_.nrcoordspertileside_/spacing +1 ;

	const osg::Vec2f diff = ( opposite-origin )/spacing;
	osg::Vec2Array* txcoords = 
	    mGetOsgVec2Arr( tileresolutiondata_[res]->txcoords_ );

	for ( int r=0; r<size; r++ )
	{
	    for ( int c=0; c<size; c++)
	    {
		(*txcoords)[r*size+c][0] = 
		    tcit->_tc00[0] + ( float(c)/diff.x() )*texturetilestep.x();
		(*txcoords)[r*size+c][1] = 
		    tcit->_tc00[1] + ( float(r)/diff.y() )*texturetilestep.y();
	    }
	}

	tileresolutiondata_[res]->setTexture( tcit->_textureUnit,
	    txcoords,stateset_ );
    }

    txunit_ = tcit->_textureUnit ;
}

