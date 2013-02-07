    /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : D. Zheng
 * DATE     : Feb 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vishorizonsection.h"

#include "binidsurface.h"
#include "binidvalset.h"
#include "coltabmapper.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "mousecursor.h"
#include "posvecdataset.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "threadwork.h"
#include "viscoord.h"
#include "vismaterial.h"
#include "vistexturechannels.h"
#include "vistexturechannel2rgba.h"
#include "vistransform.h"
#include "zaxistransform.h"

#include <osgGeo/LayeredTexture>

#include <visosg.h>
#include <osg/Group>
#include <osg/Vec3>
#include <osg/Switch>
#include <osg/PrimitiveSet>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/LightModel>
#include <osg/LineWidth>
#include <osg/BoundingBox>
#include <osgGA/TrackballManipulator>

mCreateFactoryEntry( visBase::HorizonSection );

#define mMaxLineRadius		20
#define mLineRadius		1.5

#define mMaxNrTiles		15
#define mMaxNrResolutions	10

#define mNoTesselationNeeded	0
#define mShouldRetesselate      1
#define mMustRetesselate        2

#define m00 0
#define m01 1
#define m02 2
#define m10 3
#define m11 4
#define m12 5
#define m20 6
#define m21 7
#define m22 8

#define mNumberNodePerTileSide 65
#define mMaximumResolution 6


namespace visBase
{
    
class TileTesselator;


class HorizonSectionOsgCallBack : public osg::NodeCallback
{
public:
    HorizonSectionOsgCallBack( HorizonSection* section )
	: section_( section ) {}

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    { 
	if( nv->getVisitorType()==osg::NodeVisitor::CULL_VISITOR )
	{
	    osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);

	    if ( section_->getOsgTexture()->getSetupStateSet() )
		cv->pushStateSet(section_->getOsgTexture()->getSetupStateSet());
	    osg::CullStack* cs = dynamic_cast<osg::CullStack*>(nv);
	    section_->updateAutoResolution( cs );
	    section_->updatePrimitiveSets();
	    traverse(node,nv);
	    if ( section_->getOsgTexture()->getSetupStateSet() )
		cv->popStateSet();
	}
    }

protected:
    HorizonSection* section_;
};


class TileResolutionData
{
public:
			TileResolutionData( HorizonSectionTile* sectile, 
					 int resolution )
			    : vertices_( Coordinates::create() )
			    , osgswitch_( new osg::Switch )
			    , normals_( new osg::Vec3Array )
			    , needsretesselation_( mMustRetesselate )
			    , allnormalsinvalid_( true ) 
		            , sectile_( sectile )
			    , resolution_( resolution )
			    , nrdefinedpos_( 0 )
			    , linecolor_( new osg::Vec4Array )
			    , linewidth_( new osg::LineWidth )
			    , updateprimitiveset_( true )
			    , trianglespsud_ ( 0 )
			{
				buildGeometry();
				bbox_.init(); 
			};

    enum		PrimitiveSetType {Triangle = 0, Line, Point, WireFrame};

    void		setVertices( const TypeSet<Coord3>& );
    void		updateNormal( bool invalidUpdate = false);
    bool		tesselate( bool onlyifabsness );

    void		buildGeometry();
    osg::Node*		osgNode() { return osgswitch_; }
    void		setDisplayTransformation( const mVisTrans* t ); 

    osg::BoundingBox&	getBoundingBox() { return bbox_; }
    osg::BoundingBox&	updateBBox();

    void		setPos( int row, int col, const Coord3& pos );
    void		setTexture( unsigned int unit, osg::Vec2Array* arr,
				    osg::StateSet* stateset );
    void		updatePrimitiveSets();
    void		setDispItem( int dispitem );

    osg::ref_ptr<osg::Switch>	osgswitch_;
    osg::ref_ptr<osg::Geode>	geodes_[4];
    osg::ref_ptr<osg::Geometry>	geoms_[4];
    Coordinates*		vertices_;
    osg::ref_ptr<osg::Vec3Array>normals_;
    osg::ref_ptr<osg::Vec2Array>tcoords_;
    osg::ref_ptr<osg::Vec4Array>linecolor_;
    osg::ref_ptr<osg::LineWidth>linewidth_;
    bool			allnormalsinvalid_;
    TypeSet<int>		invalidnormals_;
    char			needsretesselation_;
    int				resolution_;
    int				nrdefinedpos_;

protected:
    friend class		HorizonSection;
    HorizonSectionTile*		sectile_;
    void			computeNormal( int nmidx, osg::Vec3& ) const;

    osg::ref_ptr<osg::DrawElementsUShort>	trianglesps_;
    osg::ref_ptr<osg::DrawElementsUShort>	linesps_;
    osg::ref_ptr<osg::DrawElementsUShort>	pointsps_;
    osg::ref_ptr<osg::DrawElementsUShort>	wireframesps_;

    osg::ref_ptr<osg::DrawElementsUShort>	trianglespsud_;
    osg::ref_ptr<osg::DrawElementsUShort>	linespsud_;
    osg::ref_ptr<osg::DrawElementsUShort>	pointspsud_;
    osg::ref_ptr<osg::DrawElementsUShort>	wireframespsud_;

    int				normalstartidx_;
    int				nrnodeperside_;
    bool			updateprimitiveset_;
    osg::BoundingBox		bbox_;

private:
    void			addIndex( osg::DrawElementsUShort*, int );
    void			addTriangleIndex( int, int, int);
    void			addLineIndex( int, int );
    void			addPointIndex( int );
    void			addWireframIndex( int, int );
    int				getCoordIndex( int row, int col );
    void			tesselateCell( int idx00 );
    void			setPrimitiveSet( PrimitiveSetType, 
						 osg::DrawElementsUShort* );
};


//A tile with 65x65 nodes.
class HorizonSectionTile : CallBacker
{
public:
				HorizonSectionTile(const HorizonSection&,
						   const RowCol& origin);
				~HorizonSectionTile();
    void			setNeighbor(char neighbor,HorizonSectionTile*);
    				//!<The neighbor is numbered from 0 to 8
    void			setRightHandSystem(bool yn);

    void			setResolution(int);
    				/*!<Resolution -1 means it is automatic. */
    int				getActualResolution() const;

    void			updateAutoResolution( osg::CullStack* );
				/*<Update only when the resolutionis -1. */

    void			setPos(int row,int col,const Coord3&);
    void			setPositions(const TypeSet<Coord3>&);

    void			setTexture( osg::Vec2f origin, 
					    osg::Vec2f opposite );
			        //!<Sets origin and opposite in global texture
    void			resetResolutionChangeFlag();

				//Calle by the end of each render
    				//Makes object ready for render
    void			updateNormals( char res);
    void			tesselateResolution(char,bool onlyifabsness);
    void			applyTesselation(char res);
    				//!<Should be called from rendering thread

    void			useWireframe(bool);
    void			turnOnWireframe(char res);
    
    const osg::BoundingBoxf&	getBBox() const; 
    osg::Node*			getRootNode() const	{ return root_; }

    bool			allNormalsInvalid(int res) const;
    void			setAllNormalsInvalid(int res,bool yn);
    void			emptyInvalidNormalsList(int res);

    bool			isDefined(int row,int col) const;
    				//!<Row/Col is local to this tile

   const HorizonSection&	getSection() { return section_; }
   const RowCol&		getOrigin()  { return origin_;	}

   const HorizonSectionTile*	getNeighbor( int idx ) {return neighbors_[idx];}
   osg::StateSet*		getOsgStateSet() { return stateset_; }

   bool				getUseWireframe() { return usewireframe_; }
   void				setDisplayItem( unsigned int dispitem );

			    // default 0(triangle),1(line),2(point),3(wire frame)
   unsigned int			getDisplayItem() { return dispitem_; }
   void				updatePrimitiveSets();
   void				updateGlue();
   void				tesselateGlue();
   int				getTextureUnit() { return txunit_ ;}
   ObjectSet<TileResolutionData>	tileresolutiondata_;

protected:

    void			buildGeometries();

    friend class		HorizonSectionTilePosSetup;
    friend class		TileTesselator;		
    friend class		HorizonSection;    
    void			setActualResolution( int );
    int				getAutoResolution( osg::CullStack* );
    void			updateBBox();
    void			setInvalidNormals(int row,int col);
    void			tesselateNeigborGlue( HorizonSectionTile*, 
						       bool right );

    bool			usewireframe_;

    HorizonSectionTile*		neighbors_[9];

    osg::BoundingBox		bbox_;
    bool			needsupdatebbox_;
    int				nrdefinedpos_;
    const RowCol		origin_;
    const HorizonSection&	section_;
    osg::ref_ptr<osg::Switch>	root_;

    int				desiredresolution_;
    bool			resolutionhaschanged_;
    Threads::Mutex		datalock_;

    int				tesselationqueueid_;
    char			glueneedsretesselation_;
    //!<0 - updated, 1 - needs update, 2 - dont disp

    osg::ref_ptr<osg::StateSet>	stateset_;
    int				txunit_;
    unsigned int		dispitem_;

    osg::ref_ptr<osg::Vec3Array>	    gluevtxcoords_;
    osg::ref_ptr<osg::Vec3Array>	    gluenormals_;
    osg::ref_ptr<osg::Vec2Array>	    gluetcoords_;
    osg::ref_ptr<osg::Geode>		    gluegeode_;
    osg::ref_ptr<osg::Geometry>		    gluegeom_;
    osg::ref_ptr<osg::DrawElementsUShort>   glueps_;


};


class HorizonTileRenderPreparer: public ParallelTask
{
public: 
    HorizonTileRenderPreparer( HorizonSection& section, 
	osg::CullStack* cs, int res )
	: section_( section )
	, cs_( cs )
	, tiles_( section.tiles_.getData() )
	, nrtiles_( section.tiles_.info().getTotalSz() )
	, nrcoltiles_( section.tiles_.info().getSize(1) )
	, resolution_( res )						
	, permutation_( 0 )    
    {}

    ~HorizonTileRenderPreparer()
    { delete [] permutation_; }

od_int64 nrIterations() const { return nrtiles_; }
od_int64 totalNr() const { return nrtiles_ * 2; }
const char* message() const { return "Updating Horizon Display"; }
const char* nrDoneText() const { return "Parts completed"; }


bool doPrepare( int nrthreads )
{
    barrier_.setNrThreads( nrthreads );
    nrthreadsfinishedwithres_ = 0;

    delete [] permutation_;
    permutation_ = 0;
    mTryAlloc( permutation_, od_int64[nrtiles_] );
	
    for ( int idx=0; idx<nrtiles_; idx++ )
	permutation_[idx] = idx;

    std::random_shuffle( permutation_, permutation_+nrtiles_ );

    return true;
}


bool doWork( od_int64 start, od_int64 stop, int )
{
    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	if ( tiles_[realidx] ) 
	    tiles_[realidx]->updateAutoResolution( cs_ );
    }

    barrier_.waitForAll();

    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	HorizonSectionTile* tile = tiles_[realidx];
	if ( tile )
	{
	    int res = tile->getActualResolution();
	    if ( res==-1 ) res = resolution_;
	    tile->updateNormals( res );
	    const int actualres = tile->getActualResolution();
	    if ( actualres!=-1 )
		tile->tesselateResolution( actualres, true );
	}

	addToNrDone( 1 );
    }	
    
    barrier_.waitForAll();
    if ( !shouldContinue() )
	return false;


    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	const int realidx = permutation_[idx];
	if ( tiles_[realidx] ) 
	    tiles_[realidx]->updateGlue();

	addToNrDone( 1 );
    }

    return true;
}

    od_int64*			permutation_;
    HorizonSectionTile**	tiles_;
    HorizonSection&		section_;
    int				nrtiles_;
    int				nrcoltiles_;
    int				resolution_;
    int				nrthreads_;
    int				nrthreadsfinishedwithres_;
    Threads::Barrier		barrier_;
    osg::CullStack*		cs_;
};


class TileTesselator : public SequentialTask
{
public:
	TileTesselator( HorizonSectionTile* tile, char res )
	    : tile_( tile ), res_( res ) {}

    int	nextStep()
    {
	tile_->updateNormals( res_ );
	tile_->tesselateResolution( res_, false );
	return SequentialTask::Finished();
    }

    HorizonSectionTile*		tile_;
    unsigned char		res_;
};



class HorizonSectionTilePosSetup: public ParallelTask
{
public:    
    HorizonSectionTilePosSetup( ObjectSet<HorizonSectionTile> tiles, 
	    const Geometry::BinIDSurface& geo,
	    StepInterval<int> rrg, StepInterval<int> crg, ZAxisTransform* zat,
	    int ssz, unsigned char lowresidx )
	: tiles_( tiles )
	, geo_( geo )  
	, rrg_( rrg )
	, crg_( crg )			 	
        , zat_( zat )
	, nrcrdspertileside_( ssz )
	, lowestresidx_( lowresidx )	
    {
	if ( zat_ ) zat_->ref();
    }

    ~HorizonSectionTilePosSetup()
    {
	if ( zat_ ) zat_->unRef();
    }

    od_int64	nrIterations() const { return tiles_.size(); }

    const char*	message() const { return "Creating Horizon Display"; }
    const char*	nrDoneText() const { return "Parts completed"; }

protected:

    bool doWork( od_int64 start, od_int64 stop, int threadid )
    {
	for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
	{
	    const RowCol& origin = tiles_[idx]->origin_;
	    TypeSet<Coord3> positions;
	    positions.setCapacity( (nrcrdspertileside_)*(nrcrdspertileside_) );
	    for ( int rowidx=0; rowidx<nrcrdspertileside_ ; rowidx++ )
	    {
		const int row = origin.row + rowidx*rrg_.step;
		const bool rowok = rrg_.includes(row, false);
		const StepInterval<int> colrg( 
			mMAX(geo_.colRange(row).start, crg_.start),
		        mMIN(geo_.colRange(row).stop, crg_.stop), crg_.step );

		for ( int colidx=0; colidx<nrcrdspertileside_ ; colidx++ )
		{
		    const int col = origin.col + colidx*colrg.step;
		    Coord3 pos = rowok && colrg.includes(col, false)
			? geo_.getKnot(RowCol(row,col),false) 
			: Coord3::udf();
		    if ( zat_ ) pos.z = zat_->transform( pos );		
	    	    positions += pos;
		}
	    }

	    tiles_[idx]->setPositions( positions );
	    tiles_[idx]->updateNormals( lowestresidx_ );
	    tiles_[idx]->tesselateResolution( lowestresidx_, false );

	    addToNrDone(1);
	}

	return true;
    }

    int					nrcrdspertileside_;
    unsigned char			lowestresidx_;
    ObjectSet<HorizonSectionTile> 	tiles_;
    const Geometry::BinIDSurface&	geo_;
    StepInterval<int>			rrg_, crg_;
    ZAxisTransform*			zat_;
};



HorizonSection::HorizonSection() 
    : VisualObjectImpl( false )
    , transformation_( 0 )
    , zaxistransform_( 0 )
    , zaxistransformvoi_( -2 )			  
    , geometry_( 0 )
    , displayrrg_( -1, -1, 0 )
    , displaycrg_( -1, -1, 0 )
    , userchangedisplayrg_( false )			      
    , channels_( TextureChannels::create() )		   
    , channel2rgba_( ColTabTextureChannel2RGBA::create() ) 
    , tiles_( 0, 0 )					  
    , desiredresolution_( -1 )
    , ismoving_( false )
    , usewireframe_( false )
    , cosanglexinl_( cos(SI().computeAngleXInl()) )
    , sinanglexinl_( sin(SI().computeAngleXInl()) )		     
    , wireframematerial_( new visBase::Material )
    , tesselationlock_( false )
    , mnrcoordspertileside_( 0 )
    , mtotalnrcoordspertile_( 0 )
    , mtilesidesize_( 0 )
    , mtilelastidx_( 0 )
    , mtotalnormalsize_( 0 )
    , mlowestresidx_( 0 )
    , mhorsectnrres_( 0 )
    , spacing_( 0 )
    , nrcells_( 0 )
    , normalstartidx_( 0 )
    , normalsidesize_( 0 )
    , osghorizon_( new osg::Group )
{
    setLockable();
    cache_.allowNull( true );
    channel2rgba_->ref();
    channels_->ref();
    addChild( channels_->osgNode() );
    channels_->setChannels2RGBA( channel2rgba_ );
    if ( channels_->nrChannels()<1 )
	addChannel();
    else 
	cache_ += 0;

    channels_->getOsgTexture()->assignTextureUnits();
    channels_->getOsgTexture()->allowBorderedTextures();
    
    wireframematerial_->ref();
    osghorizon_->ref();

    osghorizon_->setCullCallback(new HorizonSectionOsgCallBack( this ));
    addChild( osghorizon_ );

}


HorizonSection::~HorizonSection()
{
    osghorizon_->unref();

    wireframematerial_->unRef();

    channel2rgba_->unRef();
    deepErase( cache_ );

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	if ( !tileptrs[idx] )
	    continue;

	writeLock();
	removeChild( tileptrs[idx]->getRootNode() );
	delete tileptrs[idx];
	writeUnLock();
    }
    
    if ( geometry_ )
    {
	CallBack cb =  mCB(this,HorizonSection,surfaceChangeCB);
	geometry_->movementnotifier.remove( cb );
	geometry_->nrpositionnotifier.remove( cb );
    }

    if ( transformation_ ) transformation_->unRef();
    removeZTransform();
   
    channels_->unRef();

    delete [] spacing_;
    delete [] nrcells_;
    delete [] normalstartidx_;
    delete [] normalsidesize_;
}


osgGeo::LayeredTexture*	HorizonSection::getOsgTexture() const 
{ 
    return channels_->getOsgTexture(); 
}

void HorizonSection::setRightHandSystem( bool yn )
{

}


void HorizonSection::setChannels2RGBA( TextureChannel2RGBA* t )
{
    channels_->setChannels2RGBA( t );
    if ( channel2rgba_ )
	channel2rgba_->unRef();

    channel2rgba_ = t;

    if ( channel2rgba_ )
	channel2rgba_->ref();
}

TextureChannel2RGBA* HorizonSection::getChannels2RGBA()
{ return channel2rgba_; }


const TextureChannel2RGBA* HorizonSection::getChannels2RGBA() const
{ return channel2rgba_; }


void HorizonSection::useChannel( bool yn )
{ channels_->turnOn( yn ); }


int HorizonSection::nrChannels() const
{ return channels_->nrChannels(); }


void HorizonSection::addChannel()
{ 
    channels_->addChannel();
    cache_ += 0;
    channel2rgba_->setEnabled( nrChannels()-1, true );
}


void HorizonSection::removeChannel( int channelidx )
{ 
    channels_->removeChannel( channelidx ); 
    delete cache_.removeSingle( channelidx );
}


void HorizonSection::swapChannels( int channel0, int channel1 )
{ 
    channels_->swapChannels( channel0, channel1 ); 
    cache_.swap( channel0, channel1 );
}


int HorizonSection::nrVersions( int channel ) const
{ return channels_->nrVersions( channel ); }


void HorizonSection::setNrVersions( int channel, int nrvers )
{ channels_->setNrVersions( channel, nrvers); }


int HorizonSection::activeVersion( int channel ) const
{ return channels_->currentVersion( channel ); }


void HorizonSection::selectActiveVersion( int channel, int version )
{ channels_->setCurrentVersion( channel, version ); }


void HorizonSection::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();
}


const mVisTrans* HorizonSection::getDisplayTransformation() const
{ return transformation_; }


void HorizonSection::setZAxisTransform( ZAxisTransform* zt, TaskRunner* )
{
    if ( zaxistransform_==zt ) 
	return;
    
    removeZTransform();
    if ( !zt ) return;

    zaxistransform_ = zt;
    zaxistransform_->ref();
    if ( geometry_ )
    {
	updateZAxisVOI();
	CBCapsule<const TypeSet<GeomPosID>*> caps( 0, geometry_ );
	surfaceChangeCB( &caps );
    }
}


void HorizonSection::removeZTransform()
{
    if ( !zaxistransform_ )
	return;

    if ( zaxistransformvoi_!=-2 )
	zaxistransform_->removeVolumeOfInterest( zaxistransformvoi_ );

    zaxistransformvoi_ = -2;
    
    zaxistransform_->unRef();
    zaxistransform_ = 0;
}


void HorizonSection::updateZAxisVOI()
{
    if ( !geometry_ || zaxistransformvoi_==-1 )	
	return;

    if ( !zaxistransform_ || !zaxistransform_->needsVolumeOfInterest() )
	return;

    CubeSampling cs;
    if ( userchangedisplayrg_ )
    	cs.hrg.set( displayrrg_, displaycrg_ );
    else
	cs.hrg.set( geometry_->rowRange(), geometry_->colRange() );

    HorSamplingIterator iter( cs.hrg );

    bool first = true;
    BinID curpos;
    while ( iter.next(curpos) )
    {
	const float depth = geometry_->getKnot(RowCol(curpos),false).z;
	if ( mIsUdf(depth) )
	    continue;

	if ( first )
	{
	    cs.zrg.start = cs.zrg.stop = depth;
	    first = false;
	}
	else
	    cs.zrg.include( depth );
    }

    if ( first ) return;

    if ( zaxistransformvoi_==-2 )
	zaxistransformvoi_ = zaxistransform_->addVolumeOfInterest( cs, false );
    else
	zaxistransform_->setVolumeOfInterest( zaxistransformvoi_, cs, false );
}


void HorizonSection::getDataPositions( DataPointSet& res, double zshift,
				       int sid, TaskRunner* tr ) const 
{
    if ( !geometry_ ) return;

    if ( zaxistransform_ && zaxistransformvoi_>=0 )
    {
	if ( !zaxistransform_->loadDataIfMissing(zaxistransformvoi_) )
		return;
    }

    const DataColDef sidcol( sKeySectionID() );
    if ( res.dataSet().findColDef(sidcol,PosVecDataSet::NameExact)==-1 )
	res.dataSet().add( new DataColDef(sidcol) );

    const int sidcolidx =  res.dataSet().findColDef( 
	    sidcol, PosVecDataSet::NameExact ) - res.nrFixedCols();
 
    BinIDValueSet& bivs = res.bivSet();
    mAllocVarLenArr( float, vals, bivs.nrVals() ); 
    for ( int idx=0; idx<bivs.nrVals(); idx++ )
	vals[idx] = mUdf(float);

    vals[sidcolidx+res.nrFixedCols()] = sid;

    const int nrknots = geometry_->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
    {
	const BinID bid = geometry_->getKnotRowCol(idx);
	if ( userchangedisplayrg_ &&
	     ( !displayrrg_.includes(bid.inl, false) || 
	       !displaycrg_.includes(bid.crl, false) ||
	       ((bid.inl-displayrrg_.start)%displayrrg_.step) ||
  	       ((bid.crl-displaycrg_.start)%displaycrg_.step) ) )
	    continue;

	const Coord3 pos = geometry_->getKnot(RowCol(bid),false);
	if ( !pos.isDefined() ) 
	    continue;

	float zval = pos.z;
	if ( zshift )
	{
	    if ( zaxistransform_ )
	    {
		zval = zaxistransform_->transform( BinIDValue(bid,zval) );
		if ( mIsUdf(zval) )
		    continue;

		zval += zshift;
		zval = zaxistransform_->transformBack( BinIDValue(bid,zval) );

		if ( mIsUdf(zval) )
		    continue;
	    }
	    else
	    {
		zval += zshift;
	    }
	}

	vals[0] = zval;
	bivs.add( bid, vals );
    }
}


void HorizonSection::setTextureData( int channel, const DataPointSet* dpset,
				     int sid, TaskRunner* tr )
{
    const BinIDValueSet* data = dpset ? &dpset->bivSet() : 0;
    if ( channel<0 || channel>=cache_.size() ) 
	return;

    if ( cache_[channel] )
    {
	if ( !data )
	{
	    delete cache_[channel];
	    cache_.replace( channel, 0 );
	}
	else
	{
	    (*cache_[channel]) = *data;
	}
    }
    else if ( data )
    {
	cache_.replace( channel, new BinIDValueSet(*data) );
    }

    updateTexture( channel, dpset, sid );
}


#define mDefineRCRange \
    const StepInterval<int> rrg = \
	userchangedisplayrg_ ? displayrrg_ : geometry_->rowRange(); \
    const StepInterval<int> crg = \
	userchangedisplayrg_ ? displaycrg_ : geometry_->colRange(); 


void HorizonSection::updateTexture( int channel, const DataPointSet* dpset, 
				    int sid )
{
    const BinIDValueSet* data = getCache( channel );
    if ( !geometry_ || !geometry_->getArray() || !dpset || !data )
	return;

    const int nrfixedcols = dpset->nrFixedCols();
    const DataColDef sidcoldef( sKeySectionID() );
    const int sidcol = 
	dpset->dataSet().findColDef(sidcoldef,PosVecDataSet::NameExact);
    const int shift = sidcol==-1 ?  nrfixedcols : nrfixedcols+1;

    const int nrversions = data->nrVals()-shift;
    setNrVersions( channel, nrversions );

    mDefineRCRange;
    const int nrrows = rrg.nrSteps()+1;
    const int nrcols = crg.nrSteps()+1;

    channels_->setSize( 1, nrrows, nrcols );
   
    ObjectSet<float> versiondata;
    versiondata.allowNull( true );
    const int nrcells = nrrows*nrcols;

    MemSetter<float> memsetter;
    memsetter.setSize( nrcells );
    memsetter.setValue( mUdf(float) );

    for ( int idx=0; idx<nrversions; idx++ )
    {
	float* vals = new float[nrcells];
	if ( !vals )
	{
	    deepEraseArr( versiondata );
	    return;
	}

	memsetter.setTarget( vals );
	memsetter.execute();

	versiondata += vals;
    }

    BinIDValueSet::Pos pos;
    const int startsourceidx = nrfixedcols + (nrfixedcols==sidcol ? 1 : 0);
    while ( data->next(pos,true) )
    {
	const float* ptr = data->getVals(pos);
	if ( sidcol!=-1 && sid!=mNINT32(ptr[sidcol]) )
	    continue;

	const BinID bid = data->getBinID( pos );
	if ( userchangedisplayrg_ )
	{
	    if ( !rrg.includes(bid.inl, false) ||!crg.includes(bid.crl,false) )
    		continue;

	    if ( (bid.inl-rrg.start) % rrg.step || 
		 (bid.crl-crg.start) % crg.step )
		continue;
	}

	const int inlidx = rrg.nearestIndex(bid.inl);
	const int crlidx = crg.nearestIndex(bid.crl);

	const int offset = inlidx*nrcols + crlidx;
	if ( offset>=nrcells )
	    continue;

	for ( int idx=0; idx<nrversions; idx++ )
	    versiondata[idx][offset] = ptr[idx+startsourceidx];
    }

    for ( int idx=0; idx<nrversions; idx++ )
	channels_->setUnMappedData( channel, idx, versiondata[idx],
				    OD::TakeOverPtr, 0 );

    updateTileTextureOrigin( RowCol( rrg.start - origin_.row,
				     crg.start - origin_.col ) );
}


void HorizonSection::setWireframeColor( Color col )
{ wireframematerial_->setColor( col ); }


void HorizonSection::setColTabSequence(int channel, const ColTab::Sequence& se)
{
    if ( channel>=0 ) channel2rgba_->setSequence( channel, se );
}


const ColTab::Sequence* HorizonSection::getColTabSequence( int channel ) const
{
    return channel<0 ? 0 : channel2rgba_->getSequence( channel );
}


void HorizonSection::setColTabMapperSetup( int channel, 
		       const ColTab::MapperSetup& mapper, TaskRunner* tr )
{
    if ( channel>=0 )
    {
	const bool needsclip =
	    channels_->getColTabMapperSetup( channel, 0 ).needsReClip( mapper );
	channels_->setColTabMapperSetup( channel, mapper );
	channels_->reMapData( channel, !needsclip, tr );
    }
}


const ColTab::MapperSetup* HorizonSection::getColTabMapperSetup( int ch ) const
{
    return ch<0 ? 0 : &channels_->getColTabMapperSetup( ch,activeVersion(ch) );
}


const TypeSet<float>* HorizonSection::getHistogram( int ch ) const
{ return channels_->getHistogram( ch ); }


void HorizonSection::setTransparency( int ch, unsigned char yn )
{ 
    mDynamicCastGet( ColTabTextureChannel2RGBA*, ct, channel2rgba_ );
    if ( ct && ch>=0 ) ct->setTransparency( ch, yn );
}


unsigned char HorizonSection::getTransparency( int ch ) const
{ 
    mDynamicCastGet( ColTabTextureChannel2RGBA*, ct, channel2rgba_ );
    if ( !ct )
	return 0;
    
    return ct->getTransparency( ch ); 
}


void HorizonSection::inValidateCache( int channel )
{
    if ( channel==-1 )
    {
	for ( int idx=0; idx<cache_.size(); idx++ )
	    inValidateCache( idx );
    }
    else
    {
    	delete cache_[channel];
    	cache_.replace( channel, 0 );
    }
}


const BinIDValueSet* HorizonSection::getCache( int channel ) const
{ 
    return cache_.validIdx(channel) ? cache_[channel] : 0; 
}


void HorizonSection::setSizeParameters()
{
    mDefineRCRange; 
    const int maxsz = mMAX( rrg.nrSteps()+1, crg.nrSteps()+1 ); 

    mnrcoordspertileside_ = mNumberNodePerTileSide;
    mhorsectnrres_ = mMaximumResolution;
    while ( maxsz > (mnrcoordspertileside_ * mMaxNrTiles) &&
	    mhorsectnrres_ < mMaxNrResolutions )
    {
	mnrcoordspertileside_ *= 2;
	mhorsectnrres_++;
    }

    mtotalnrcoordspertile_ = mnrcoordspertileside_ * mnrcoordspertileside_;
    mtilesidesize_ = mnrcoordspertileside_ - 1;
    mtilelastidx_ = mnrcoordspertileside_ - 1;
    mlowestresidx_ = mhorsectnrres_-1;

    delete [] spacing_;
    spacing_ = new int[mhorsectnrres_];

    delete [] nrcells_;
    nrcells_ = new int[mhorsectnrres_];

    delete [] normalstartidx_;
    normalstartidx_ = new int[mhorsectnrres_];

    delete [] normalsidesize_;
    normalsidesize_ = new int[mhorsectnrres_];

    mtotalnormalsize_ = 0;

    for ( int idx=0; idx<mhorsectnrres_; idx++ )
    {
	spacing_[idx] = !idx ? 1 : 2 * spacing_[idx-1];
	normalsidesize_[idx] = mtilesidesize_ / spacing_[idx] + 1;
	nrcells_[idx] = ( normalsidesize_[idx] - (idx ? 1 : 0) ) * 
	    		( normalsidesize_[idx] - (idx ? 1 : 0) );
	normalstartidx_[idx] = mtotalnormalsize_;
	mtotalnormalsize_ += normalsidesize_[idx] * normalsidesize_[idx];
    }

}


void HorizonSection::setSurface( Geometry::BinIDSurface* surf, bool connect,
       				 TaskRunner* tr )
{
    if ( !surf ) return;

    if ( connect )
    {
	geometry_ = surf;
	CallBack cb =  mCB( this, HorizonSection, surfaceChangeCB );
	geometry_->movementnotifier.notify( cb );
	geometry_->nrpositionnotifier.notify( cb );
    }

    setSizeParameters();
    rowdistance_ = geometry_->rowRange().step*SI().inlDistance();
    coldistance_ = geometry_->colRange().step*SI().crlDistance();
    surfaceChange( 0, tr );
}


void HorizonSection::setDisplayRange( const StepInterval<int>& rrg,
				      const StepInterval<int>& crg )
{
    if ( rrg.isUdf() || crg.isUdf() || (displayrrg_==rrg && displaycrg_==crg) )
	return;

    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tiles_.info().getTotalSz(); idx++ )
    {
	if ( tileptrs[idx] )
	{
	    writeLock();
    	    removeChild( tileptrs[idx]->getRootNode() );
    	    delete tileptrs[idx];
	    tileptrs[idx] = 0;
	    writeUnLock();
	}
    }

    userchangedisplayrg_ = true;
    displayrrg_ = rrg;
    displaycrg_ = crg;
    origin_ = RowCol( displayrrg_.start, displaycrg_.start );
    rowdistance_ = displayrrg_.step*SI().inlDistance();
    coldistance_ = displaycrg_.step*SI().crlDistance();

    setSizeParameters();
    surfaceChange( 0, 0 );
    setResolution( desiredresolution_, 0 );
}


StepInterval<int> HorizonSection::displayedRowRange() const
{
   if ( userchangedisplayrg_ ) 
       return displayrrg_; 

   if ( geometry_ )
       return geometry_->rowRange();
   else
       return StepInterval<int>(0, 0, 0);
}


StepInterval<int> HorizonSection::displayedColRange() const
{ 
   if ( userchangedisplayrg_ ) 
       return displaycrg_; 

   if ( geometry_ )
       return geometry_->colRange();
   else
       return StepInterval<int>(0, 0, 0);
}


void HorizonSection::surfaceChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpack( const TypeSet<GeomPosID>*, gpids, cb );

    updatelock_.lock();
    surfaceChange( gpids, 0 );
    updatelock_.unLock();
}


void HorizonSection::surfaceChange( const TypeSet<GeomPosID>* gpids,
				    TaskRunner* tr )
{
   
    if ( !geometry_ || !geometry_->getArray() )
	return;

    if ( zaxistransform_ && zaxistransformvoi_!=-1 )
    {
	updateZAxisVOI();
	if ( !zaxistransform_->loadDataIfMissing(zaxistransformvoi_,tr) )
	    return;
    }

    if ( !gpids || !tiles_.info().getSize(0) || !tiles_.info().getSize(1) )
	resetAllTiles( tr );
    else
	updateNewPoints( gpids, tr );

}


void HorizonSection::updateNewPoints( const TypeSet<GeomPosID>* gpids,
				      TaskRunner* tr )
{
    mDefineRCRange;
    if ( rrg.width(false)<0 || crg.width(false)<0 )
	return;
    
    tesselationlock_ = true;
    updateTileArray();
    
    const int nrrowsz = tiles_.info().getSize(0);
    const int nrcolsz = tiles_.info().getSize(1);
    
    ObjectSet<HorizonSectionTile> fullupdatetiles;
    ObjectSet<HorizonSectionTile> oldupdatetiles;

    for ( int idx=(*gpids).size()-1; idx>=0; idx-- )
    {
	const RowCol absrc = RowCol::fromInt64( (*gpids)[idx] );
	RowCol rc = absrc - origin_; 
	rc.row /= rrg.step; rc.col /= crg.step;

	int tilerowidx = rc.row/mtilesidesize_;
	int tilerow = rc.row%mtilesidesize_;
	if ( tilerowidx==nrrowsz && !tilerow )
	{
	    tilerowidx--;
	    tilerow = mtilelastidx_;
	}

	int tilecolidx = rc.col/mtilesidesize_;
	int tilecol = rc.col%mtilesidesize_;
	if ( tilecolidx==nrcolsz && !tilecol )
	{
	    tilecolidx--;
	    tilecol = mtilelastidx_;
	}

	/*If we already set work area and the position is out of the area,
	  we will skip the position. */
	if ( tilerowidx>=nrrowsz || tilecolidx>=nrcolsz )
	    continue;

	const Coord3 pos = geometry_->getKnot(absrc,false);
	
	bool addoldtile = false;
	HorizonSectionTile* tile = tiles_.get( tilerowidx, tilecolidx );
	if ( !tile ) 
	{
	    tile = createTile( tilerowidx, tilecolidx );
	    fullupdatetiles += tile;
	}
	else if ( fullupdatetiles.indexOf(tile)==-1 )
	{
	    for ( int res=0; res<=mlowestresidx_; res++ )
		tile->setAllNormalsInvalid( res, false );
	
	    tile->setPos( tilerow, tilecol, pos );
	    if ( desiredresolution_!=-1 )
	    {
		addoldtile = true;
		if ( oldupdatetiles.indexOf(tile)==-1 )
		    oldupdatetiles += tile;		    
	    }
	}

	for ( int rowidx=-1; rowidx<=1; rowidx++ ) //Update neighbors
	{
	    const int nbrow = tilerowidx+rowidx;
	    if ( nbrow<0 || nbrow>=nrrowsz ) continue;

	    for ( int colidx=-1; colidx<=1; colidx++ )
	    {
		const int nbcol = tilecolidx+colidx;
		if ( (!rowidx && !colidx) || nbcol<0 || nbcol>=nrcolsz )
		    continue;

		HorizonSectionTile* nbtile = tiles_.get( nbrow, nbcol );
		if ( !nbtile || fullupdatetiles.indexOf(nbtile)!=-1)
		    continue;

		nbtile->setPos( tilerow-rowidx*mtilesidesize_,
				tilecol-colidx*mtilesidesize_, pos );
		if ( !addoldtile || rowidx+colidx>=0 || desiredresolution_==-1 
				 || oldupdatetiles.indexOf(nbtile)!=-1 )
		    continue;
	    
		if ( (!tilecol && !rowidx && colidx==-1) || 
			(!tilerow && rowidx==-1 && 
			 ((!tilecol && colidx==-1) || !colidx)) )
		    oldupdatetiles += nbtile;
	    }
	}
    }

    HorizonSectionTilePosSetup task( fullupdatetiles, *geometry_, rrg, crg, 
	    zaxistransform_, mnrcoordspertileside_, mlowestresidx_ );
    TaskRunner::execute( tr, task );

    //Only for fixed resolutions, which won't be tesselated at render.
    if ( oldupdatetiles.size() )
    {
	TypeSet<Threads::Work> work;
	for ( int idx=0; idx<oldupdatetiles.size(); idx++ )
	{
	    TileTesselator* tt =
		new TileTesselator( oldupdatetiles[idx], desiredresolution_ );
	    work += Threads::Work( *tt, true );
	}

	Threads::WorkManager::twm().addWork( work,
	       Threads::WorkManager::cDefaultQueueID() );
    }
  
    tesselationlock_ = false;
}


void HorizonSection::resetAllTiles( TaskRunner* tr )
{
    mDefineRCRange;
    if ( rrg.width(false)<0 || crg.width(false)<0 )
	return;
    
    tesselationlock_ = true;
    origin_ = RowCol( rrg.start, crg.start );
    const int nrrows = nrBlocks( rrg.nrSteps()+1, mnrcoordspertileside_, 1 );
    const int nrcols = nrBlocks( crg.nrSteps()+1, mnrcoordspertileside_, 1 );

    writeLock();
    if ( !tiles_.setSize( nrrows, nrcols ) )
    {
	tesselationlock_ = false;
	writeUnLock();
	return;
    }

    tiles_.setAll( 0 );
    writeUnLock();

    ObjectSet<HorizonSectionTile> fullupdatetiles;
    for ( int tilerowidx=0; tilerowidx<nrrows; tilerowidx++ )
    {
	for ( int tilecolidx=0; tilecolidx<nrcols; tilecolidx++ )
	{
	    fullupdatetiles += createTile(tilerowidx, tilecolidx);
	}
    }

    updateTileTextureOrigin( RowCol(0,0) );
    
    HorizonSectionTilePosSetup task( fullupdatetiles, *geometry_, rrg, crg, 
	    zaxistransform_, mnrcoordspertileside_, mlowestresidx_ );
    TaskRunner::execute( tr, task );
    
    tesselationlock_ = false;
    
}

void HorizonSection::turnOsgOn( bool yn )
{  turnOn( yn ); }


void HorizonSection::updateTileArray()
{
    mDefineRCRange;
    const int rowsteps =  mtilesidesize_ * rrg.step;
    const int colsteps = mtilesidesize_ * crg.step;
    const int oldrowsize = tiles_.info().getSize(0);
    const int oldcolsize = tiles_.info().getSize(1);
    int newrowsize = oldrowsize;
    int newcolsize = oldcolsize;
    int nrnewrowsbefore = 0;
    int nrnewcolsbefore = 0;
	
    int diff = origin_.row - rrg.start;
    if ( diff>0 ) 
    {
	nrnewrowsbefore = diff/rowsteps + (diff%rowsteps ? 1 : 0);
    	newrowsize += nrnewrowsbefore;
    }

    diff = rrg.stop - (origin_.row+oldrowsize*rowsteps);
    if ( diff>0 ) newrowsize += diff/rowsteps + (diff%rowsteps ? 1 : 0);
    
    diff = origin_.col - crg.start;
    if ( diff>0 ) 
    {
	nrnewcolsbefore = diff/colsteps + (diff%colsteps ? 1 : 0);
    	newcolsize += nrnewcolsbefore;
    }

    diff = crg.stop - (origin_.col+oldcolsize*colsteps);
    if ( diff>0 ) newcolsize += diff/colsteps + (diff%colsteps ? 1 : 0);

    if ( newrowsize==oldrowsize && newcolsize==oldcolsize )
	return;

    Array2DImpl<HorizonSectionTile*> newtiles( newrowsize, newcolsize );
    newtiles.setAll( 0 );

    for ( int rowidx=0; rowidx<oldrowsize; rowidx++ )
    {
	const int targetrow = rowidx+nrnewrowsbefore;
	for ( int colidx=0; colidx<oldcolsize; colidx++ )
	{
	    const int targetcol = colidx+nrnewcolsbefore;
	    newtiles.set( targetrow, targetcol, tiles_.get(rowidx,colidx) );
	}
    }

    writeLock();
    tiles_.copyFrom( newtiles );
    origin_.row -= nrnewrowsbefore*rowsteps;
    origin_.col -= nrnewcolsbefore*colsteps;
    writeUnLock();
}


void HorizonSection::updateTileTextureOrigin( const RowCol& textureorigin )
{
    const int nrrows = tiles_.info().getSize(0);
    const int nrcols = tiles_.info().getSize(1);
    mDefineRCRange;
 
    std::vector<float> sOrigins, tOrigins;
    const osgGeo::LayeredTexture* texture = getOsgTexture();

    const osg::Vec2f imgsize = texture->imageEnvelopeSize();
    if ( !imgsize.length())
	return ;

    texture->planTiling( mnrcoordspertileside_ - 1, sOrigins, tOrigins );

    const int nrs = sOrigins.size()-1;
    const int nrt = tOrigins.size()-1;

    for ( int ids=0; ids<nrs; ids++ )
    {
	for ( int idt=0; idt<nrt; idt++ )
	{
	    osg::Vec2f origin( sOrigins[ids], tOrigins[idt] );
	    osg::Vec2f opposite( sOrigins[ids+1], tOrigins[idt+1] );
	    HorizonSectionTile* tile = tiles_.get( idt, ids );
	    if ( !tile )
		continue;
	    tile->setTexture( origin, opposite );
	}
    }
}


HorizonSectionTile* HorizonSection::createTile( int tilerowidx, int tilecolidx )
{
    mDefineRCRange;
    const RowCol step( rrg.step, crg.step );
    const RowCol tileorigin( origin_.row+tilerowidx*mtilesidesize_*step.row,
			     origin_.col+tilecolidx*mtilesidesize_*step.col );
    HorizonSectionTile* tile = new HorizonSectionTile( *this, tileorigin );

    tile->setResolution( desiredresolution_ );
    tile->useWireframe( usewireframe_ );
    tile->setRightHandSystem( righthandsystem_ );

    writeLock();
    tiles_.set( tilerowidx, tilecolidx, tile );
    writeUnLock();
    
    for ( int rowidx=-1; rowidx<=1; rowidx++ )
    {
	const int neighborrow = tilerowidx+rowidx;
	if ( neighborrow<0 || neighborrow>=tiles_.info().getSize(0) )
	    continue;

	for ( int colidx=-1; colidx<=1; colidx++ )
	{
	    if ( !colidx && !rowidx )
		continue;

	    const int neighborcol = tilecolidx+colidx;
	    if ( neighborcol<0 || neighborcol>=tiles_.info().getSize(1) )
		continue;

	    HorizonSectionTile* neighbor = tiles_.get(neighborrow,neighborcol);

	    if ( !neighbor ) 
		continue;

	    char pos;
	    if ( colidx==-1 ) 
		pos = rowidx==-1 ? m00 : (!rowidx ? m10 : m20);
	    else if ( colidx==0 ) 
		pos = rowidx==-1 ? m01 : (!rowidx ? m11 : m21);
	    else 
		pos = rowidx==-1 ? m02 : (!rowidx ? m12 : m22);

	    tile->setNeighbor( pos, neighbor );

	    if ( colidx==1 ) 
		pos = rowidx==1 ? m00 : (!rowidx ? m10 : m20);
	    else if ( colidx==0 ) 
		pos = rowidx==1 ? m01 : (!rowidx ? m11 : m21);
	    else 
		pos = rowidx==1 ? m02 : (!rowidx ? m12 : m22);

	    neighbor->setNeighbor( pos, tile );
	}
    }

    writeLock();
    osghorizon_->addChild( tile->getRootNode() );
    writeUnLock();

    return tile;
}


void HorizonSection::updateAutoResolution( osg::CullStack* cs )
{
    const int tilesz = tiles_.info().getTotalSz();
    if ( !tilesz ) return;

    HorizonSectionTile** tileptrs = tiles_.getData();

    HorizonTileRenderPreparer task( *this, cs, desiredresolution_ );
    task.execute();

    Threads::SpinLockLocker lock ( lock_ );
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( tileptrs[idx] )
	{
	    tileptrs[idx]->applyTesselation( 
		tileptrs[idx]->getActualResolution() );
	    tileptrs[idx]->resetResolutionChangeFlag();
	}
    }
}


void HorizonSection::updatePrimitiveSets()
{
    const int tilesz = tiles_.info().getTotalSz();
    if ( !tilesz ) return;

    HorizonSectionTile** tileptrs = tiles_.getData();

    Threads::SpinLockLocker lock( lock_ );
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( tileptrs[idx] )
	    tileptrs[idx]->updatePrimitiveSets();
    }
}


char HorizonSection::currentResolution() const
{ return desiredresolution_; }


char HorizonSection::nrResolutions() const
{ return mhorsectnrres_; }


void HorizonSection::setResolution( int res, TaskRunner* tr )
{
    desiredresolution_ = res;
    const int tilesz = tiles_.info().getTotalSz();
    if ( !tilesz ) return;
    
    HorizonSectionTile** tileptrs = tiles_.getData();
    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->setResolution( res );

    if ( res==-1 )
	return;

    TypeSet<Threads::Work> work;
    for ( int idx=0; idx<tilesz; idx++ )
    {
	if ( !tileptrs[idx] )
	    continue;
	
	tileptrs[idx]->setActualResolution( res );
	work += Threads::Work(
		*new TileTesselator( tileptrs[idx], res ), true );
    }
    
    Threads::WorkManager::twm().addWork( work,
	       Threads::WorkManager::cDefaultQueueID() );
}


void HorizonSection::useWireframe( bool yn )
{
    if ( usewireframe_==yn )
	return;

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    usewireframe_ = yn;

    HorizonSectionTile** tileptrs = tiles_.getData();
    const int tilesz = tiles_.info().getTotalSz();
    
    for ( int idx=0; idx<tilesz; idx++ )
	if ( tileptrs[idx] ) tileptrs[idx]->useWireframe( yn );
}


bool HorizonSection::usesWireframe() const
{ return usewireframe_; }


HorizonSectionTile::HorizonSectionTile( const HorizonSection& section,
					const RowCol& origin )
    : root_( new osg::Switch )
    , desiredresolution_( -1 )
    , resolutionhaschanged_( false )
    , needsupdatebbox_( false )
    , usewireframe_( false )
    , nrdefinedpos_( 0 )
    , origin_( origin )
    , section_( section )
    , dispitem_( 0 )
    , stateset_( new osg::StateSet )
    , gluevtxcoords_( new osg::Vec3Array )
    , gluetcoords_( new osg::Vec2Array )
    , gluegeode_( new osg::Geode )
    , gluegeom_( new osg::Geometry )
    , gluenormals_ ( new osg::Vec3Array )
    , tesselationqueueid_( Threads::WorkManager::twm().addQueue(
			    Threads::WorkManager::MultiThread ) )
{

    tileresolutiondata_.allowNull();
    buildGeometries();
    bbox_.init();
}


void HorizonSectionTile::buildGeometries()
{
    
    for ( int i = 0; i < 9; i++ )
	neighbors_[i] = 0;
    
    glueps_ = new osg::DrawElementsUShort( osg::PrimitiveSet::TRIANGLES, 0 );
    
    for ( int res=0; res<section_.mhorsectnrres_; res++ )
    {
	tileresolutiondata_ += new TileResolutionData( this, res );
	root_->addChild( tileresolutiondata_[res]->osgNode() );
    }

    root_->addChild( gluegeode_ );
    gluegeode_->addDrawable( gluegeom_ );
    gluegeom_->setVertexArray( gluevtxcoords_ );
    gluegeom_->setNormalArray( gluenormals_ );
    gluegeom_->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
    gluegeom_->setDataVariance( osg::Object::DYNAMIC );
    root_->setAllChildrenOff();

}


HorizonSectionTile::~HorizonSectionTile()
{
    root_->removeChildren( 0, root_->getNumChildren() );

    Threads::WorkManager::twm().removeQueue( tesselationqueueid_, false );
    deepErase( tileresolutiondata_ );
}


void HorizonSectionTile::setRightHandSystem( bool yn )
{
 
}


void HorizonSectionTile::updateNormals( char res )
{
    if ( res<0 ) return;

    bool change = false;
    if ( tileresolutiondata_[res]->allnormalsinvalid_ )
    {
	datalock_.lock();
	tileresolutiondata_[res]->updateNormal();
	datalock_.unLock();
	change = true;
    }
    else
    {
	datalock_.lock();
	tileresolutiondata_[res]->updateNormal( true );
	datalock_.unLock();
	change = true;
    }

    emptyInvalidNormalsList( res );
    tileresolutiondata_[res]->allnormalsinvalid_ = false; 
    tileresolutiondata_[res]->normals_->dirty();
 
}


#define mGetGradient( rc, arrpos ) \
    beforefound = false; afterfound = false; \
    for ( int idx=section.spacing_[res]; idx>=0; idx-- ) \
    { \
        if ( !beforefound ) \
        { \
            const int cur##rc = rc-idx*step.rc; \
            if ( cur##rc>=rc##range.start ) \
            { \
                const Coord3 pos =  \
                    section.geometry_->getKnot(arrpos,false); \
                if ( pos.isDefined() ) \
                { \
		    Coord3 crd; \
		    mVisTrans::transform( section.transformation_, pos, crd ); \
                    beforepos.x = -idx*section.rc##distance_; \
                    beforepos.y = crd.z; \
                    beforefound = true; \
                } \
            } \
        } \
 \
        if ( idx && !afterfound ) \
        { \
            const int cur##rc = rc+idx*step.rc; \
            if ( cur##rc<=rc##range.stop ) \
            { \
                const Coord3 pos =  \
                    section.geometry_->getKnot(arrpos,false); \
                if ( pos.isDefined() ) \
                { \
		    Coord3 crd; \
		    mVisTrans::transform( section.transformation_, pos, crd ); \
                    afterpos.x = idx*section.rc##distance_; \
                    afterpos.y = crd.z; \
                    afterfound = true; \
                } \
            } \
        } \
 \
        if ( afterfound && beforefound ) \
            break; \
    } \
 \
    const double d##rc = afterfound && beforefound \
        ? (afterpos.y-beforepos.y)/(afterpos.x-beforepos.x) \
        : 0;


void HorizonSectionTile::resetResolutionChangeFlag()
{ resolutionhaschanged_= false; }


void HorizonSectionTile::emptyInvalidNormalsList( int res )
{
    datalock_.lock();
    tileresolutiondata_[res]->invalidnormals_.erase();
    datalock_.unLock();
}


void HorizonSectionTile::setAllNormalsInvalid( int res, bool yn )
{ 
    tileresolutiondata_[res]->allnormalsinvalid_ = yn; 
    
    if ( yn ) emptyInvalidNormalsList( res );
}


bool HorizonSectionTile::allNormalsInvalid( int res ) const
{ return tileresolutiondata_[res]->allnormalsinvalid_; }


void HorizonSectionTile::setResolution( int res )
{ 
    desiredresolution_ = res; 
}


int HorizonSectionTile::getActualResolution() const
{
    int numchildren = root_->getNumChildren();

    for ( int i=0; i<numchildren; i++ )
    {
	if ( root_->getValue( i ) )
	  return  tileresolutiondata_[i]->resolution_;
    }
    
    return -1;
}


void HorizonSectionTile::updateAutoResolution( osg::CullStack* cs )
{
    int newres = desiredresolution_;
    if ( newres==-1 )
    {
	updateBBox();
	if ( !bbox_.valid() )
	    newres = -1;
	else if ( desiredresolution_==-1 )
	{
	    const int wantedres = getAutoResolution( cs );
	    newres = wantedres;
	    datalock_.lock();
	    for ( ; newres<section_.mhorsectnrres_-1; newres++ )
	    {
		if ( tileresolutiondata_[newres]->needsretesselation_ < 
						    mMustRetesselate )
		    break;
	    }
	    datalock_.unLock();

	    if ( !section_.tesselationlock_ &&
		(wantedres!=newres || 
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


const osg::BoundingBoxf& HorizonSectionTile::getBBox() const
{ return bbox_; }


#define mIsDef( pos ) (pos[2]<9.9e29)


void HorizonSectionTile::updateBBox()
{
    if ( !needsupdatebbox_ ) return;

    bbox_.init();

    for ( int idx=0; idx<section_.mhorsectnrres_; idx++ )
    {
	bbox_.expandBy( tileresolutiondata_[idx]->updateBBox() );
    }

    needsupdatebbox_ = false;
}


void HorizonSectionTile::setDisplayItem( unsigned int dispitem )
{ 
    dispitem_ = dispitem; 
}


void HorizonSectionTile::updateGlue()
{
    if ( glueneedsretesselation_ || resolutionhaschanged_ ||
	(neighbors_[5] && neighbors_[5]->resolutionhaschanged_) ||
	(neighbors_[7] && neighbors_[7]->resolutionhaschanged_) ||
	(neighbors_[8] && neighbors_[8]->resolutionhaschanged_) )
    {
	tesselateGlue();
    }
}


#define mGlueRight 5
#define mGlueBottom 7


void HorizonSectionTile::tesselateGlue()
{
    const int res = getActualResolution();
    const short nrcoordspertile = section_.mnrcoordspertileside_;

    if( res==-1 || ( !neighbors_[mGlueRight] && !neighbors_[mGlueBottom] ) ) 
	return;

    for ( int nb=5; nb<8; nb += 2 )
    {
	bool right = nb == mGlueRight ? true : false;
	tesselateNeigborGlue( neighbors_[nb],right );
    }
    
}


#define mAddGlue( glueidx, gluepsidx ) \
    gluevtxcoords_->push_back( ( *arr )[glueidx] ) ; \
    gluetcoords_->push_back( ( *tcoords )[glueidx] );\
    gluenormals_->push_back( ( *normals )[glueidx] );\
    glueps_->push_back( gluepsidx );\


#define mClearGlue()\
    gluevtxcoords_->clear();\
    gluenormals_->clear();\
    gluetcoords_->clear();\
    glueps_->clear();\

#define mTriangleEndIdx 3
#define mTriangleBeginIdx 1

void HorizonSectionTile::tesselateNeigborGlue( HorizonSectionTile* neighbor,
						bool right )
{
    if ( !neighbor ) 
	return;
    const int res = getActualResolution();
    const int nbres = neighbor->getActualResolution();

    if( res==nbres ) 
	return;

    HorizonSectionTile* gluetile = res < nbres ? this : neighbor ;
    const int hgres = res < nbres ? res : nbres;
    osg::StateSet* stateset = 
			res < nbres ? stateset_ : neighbor->getOsgStateSet();

    const HorizonSection& section = gluetile->getSection();
    const int spacing = section.spacing_[hgres];
    const int nrblock = spacing == 1 ? (int)section.mnrcoordspertileside_
		   /spacing : (int)section.mnrcoordspertileside_/spacing +1 ;

    Coordinates* coords = gluetile->tileresolutiondata_[hgres]->vertices_;

    osg::Vec3Array* arr = dynamic_cast<osg::Vec3Array*>
	( gluetile->tileresolutiondata_[hgres]->vertices_->osgArray() );
    osg::Vec3Array* normals = gluetile->tileresolutiondata_[hgres]->normals_;
    osg::Vec2Array* tcoords = gluetile->tileresolutiondata_[hgres]->tcoords_;

    mClearGlue();

    int glueidx = 0;
    int gluepsidx = 0;
    int triangleidx = 0;
    int oldidx = 0;
    for ( int idx=0; idx<nrblock; idx++ )
    {
	if ( right && gluetile==this )
	{
	      glueidx = ( idx+1 )*nrblock;
	}
	else if ( right && gluetile==neighbor )
	{
	    glueidx = idx == 0 ? 0 : idx*nrblock + 1;
	}
	else if ( !right && gluetile==this )
	{
	    int baseidx = (nrblock - 1)*nrblock;
	    glueidx = baseidx + idx ;
	}
	else if ( !right && gluetile==neighbor )
	{
	    glueidx = idx;
	}
	if( coords->isDefined( glueidx ) )
	{
	    if( triangleidx==mTriangleEndIdx )
	    {
		mAddGlue( oldidx, gluepsidx ) ;
		gluepsidx++;
		triangleidx = mTriangleBeginIdx;
	    }
	    mAddGlue( glueidx, gluepsidx );
	    oldidx = glueidx;
	    gluepsidx++;
	    triangleidx ++;
	}
    }

    if ( gluegeode_ && gluegeom_ )
    {
	int unit = gluetile->getTextureUnit();
	gluegeom_->removePrimitiveSet( 0,gluegeom_->getNumPrimitiveSets() );
	gluegeom_->addPrimitiveSet( new osg::DrawArrays(
			osg::PrimitiveSet::TRIANGLES, 0, glueps_->size()) ) ;
	gluegeom_->setTexCoordArray( unit, gluetcoords_ );
	gluegeode_->setStateSet( stateset );
    }

}


#define mIdealNrPixelsPerCell 32

int HorizonSectionTile::getAutoResolution( osg::CullStack* cs ) 
{
    updateBBox();
    if ( !bbox_.valid() || !cs)
	return -1;

    const float screensize = cs->clampedPixelSize(osg::BoundingSphere(bbox_) );
    const float nrpixels = screensize*screensize;
    const int wantednumcells = (int)( nrpixels / mIdealNrPixelsPerCell);
    if ( !wantednumcells )
	return section_.mlowestresidx_;

    if ( nrdefinedpos_<=wantednumcells )
	return 0;

    int maxres = nrdefinedpos_/wantednumcells;
    if ( nrdefinedpos_%wantednumcells ) maxres++;

    for ( int desiredres=section_.mlowestresidx_; desiredres>=0; desiredres-- )
    {
	if ( section_.nrcells_[desiredres]>=wantednumcells )
	    return mMIN(desiredres,maxres);
    }

    return 0;
}


void HorizonSectionTile::setActualResolution( int resolution )
{
    if ( resolution != getActualResolution() )
    {
	root_->setAllChildrenOff();
	root_->setValue( resolution, true );
	tileresolutiondata_[resolution]->setDispItem( dispitem_ );
	if ( !dispitem_ )
	   root_->setValue( section_.mhorsectnrres_, true );
	resolutionhaschanged_ = true;
    }
}


void HorizonSectionTile::tesselateResolution( char res, bool onlyifabsness )
{
    datalock_.lock();
    tileresolutiondata_[res]->tesselate( onlyifabsness );
    datalock_.unLock();
}


void HorizonSectionTile::applyTesselation( char res )
{
    if ( !tileresolutiondata_.validIdx( res ) )
	return;
    root_->setChildValue( root_->getChild( res ), true );
}


void HorizonSectionTile::useWireframe( bool yn )
{
    if ( usewireframe_==yn )
	return;

    usewireframe_ = yn;
    for ( int idx=0; idx<section_.mhorsectnrres_; idx++ )
	tileresolutiondata_[idx]->needsretesselation_ = 1; //TODO
}


void HorizonSectionTile::setPositions( const TypeSet<Coord3>& pos )
{
    RefMan<const Transformation> trans = section_.transformation_;
    nrdefinedpos_ = 0;
    bbox_.init();

    datalock_.lock();
    for ( int res=0; res< section_.mhorsectnrres_; res++)
    {
	tileresolutiondata_[res]->setDisplayTransformation( trans );
	tileresolutiondata_[res]->setVertices( pos );
	bbox_.expandBy( tileresolutiondata_[res]->getBoundingBox() );
	tileresolutiondata_[res]->needsretesselation_ = mMustRetesselate;
	tileresolutiondata_[res]->allnormalsinvalid_ = true;
	tileresolutiondata_[res]->invalidnormals_.erase();
	if ( tileresolutiondata_[res]->nrdefinedpos_ > 0 )
	    nrdefinedpos_ = tileresolutiondata_[res]->nrdefinedpos_;
    }
    //Prevent anything to be sent in this shape to Coin
    needsupdatebbox_ = false;
    datalock_.unLock();

}
 

void HorizonSectionTile::updatePrimitiveSets()
{
    for ( int res=0; res< section_.mhorsectnrres_; res++)
	tileresolutiondata_[res]->updatePrimitiveSets();

}

 
void HorizonSectionTile::setNeighbor( char nbidx, HorizonSectionTile* nb )
{  neighbors_[nbidx] = nb; }


void HorizonSectionTile::setPos( int row, int col, const Coord3& pos )
{
   
    for ( int res = 0; res < section_.mhorsectnrres_; )
	tileresolutiondata_[res]->setPos( row, col, pos );
    
}


void HorizonSectionTile::setInvalidNormals( int row, int col )
{ 
 
}


void HorizonSectionTile::setTexture( osg::Vec2f origin, osg::Vec2f opposite )
{
    std::vector<osgGeo::LayeredTexture::TextureCoordData> tcdata;
    const osgGeo::LayeredTexture* texture = section_.getOsgTexture();
  
    stateset_ = texture->createCutoutStateSet( origin,opposite, tcdata );
    std::vector<osgGeo::LayeredTexture::TextureCoordData>::iterator tcit = 
	tcdata.begin();

    if ( !tcdata.size() )
	return;    

    osg::Vec2 texturetilestep = tcit->_tc11 - tcit->_tc00;
    const HorizonSection& section = getSection();

    for ( int res=0; res<section.mhorsectnrres_; res++)
    {
	const int spacing = section_.spacing_[res];
	int size = spacing == 1 ? (int)section.mnrcoordspertileside_/spacing :
	    (int)section.mnrcoordspertileside_/spacing +1 ;

	osg::Vec2f diff = ( opposite-origin )/spacing;

	for ( int r=0; r<size; r++ )
	{
	    for ( int c=0; c<size; c++)
	    {
		(*tileresolutiondata_[res]->tcoords_)[r*size+c][0] = 
		    tcit->_tc00[0] + ( float(c)/diff.x() )*texturetilestep.x();
		(*tileresolutiondata_[res]->tcoords_)[r*size+c][1] = 
		    tcit->_tc00[1] + ( float(r)/diff.y() )*texturetilestep.y();
	    }
	}
	tileresolutiondata_[res]->setTexture( tcit->_textureUnit,
	    tileresolutiondata_[res]->tcoords_.get(),stateset_ );
    }

    txunit_ = tcit->_textureUnit ;

}


void TileResolutionData::setTexture( unsigned int unit, osg::Vec2Array* tcarr,
				  osg::StateSet* stateset )
{
   unsigned int dispitem = sectile_->getDisplayItem(); 
   osg::Geode* geode = geodes_[dispitem];
   osg::Geometry* geom = geoms_[dispitem];

   if ( geode && geom )
   {
	geom->setTexCoordArray( unit, tcarr );
	geode->setStateSet( stateset );
   }
}


void TileResolutionData::setDispItem( int dispitem )
{
    osgswitch_->setAllChildrenOff();
    osgswitch_->setValue( dispitem, true );
}


void TileResolutionData::setVertices( const TypeSet<Coord3>& pos )
{
    const HorizonSection& section = sectile_->getSection();
    const int spacing = section.spacing_[resolution_];
    int sidesize = section.mnrcoordspertileside_;
    int cidx = 0;
    bbox_.init();

    for ( int row=0; row<sidesize; row+=spacing )
    {
	for ( int col=0; col<sidesize; col+=spacing )
	{
	    int crdIdx = col + row*sidesize;
	    Coord3 vertex = pos[crdIdx];
	    if ( crdIdx >= pos.size() || !vertex.isDefined() )
	    {
		vertex[2] = mUdf(float);
	    }
	    else
		nrdefinedpos_ = spacing == 1 ? nrdefinedpos_+1 : nrdefinedpos_;
		vertices_->setPos( cidx, vertex );
		osg::Vec3Array* arr = 
		    dynamic_cast<osg::Vec3Array*>(vertices_->osgArray());
		osg::Vec3f coord = arr->at( cidx );
		if( vertex[2] != mUdf(float) )
		    bbox_.expandBy( coord );
	    cidx++;
	}
    }
}


void TileResolutionData::updateNormal( bool invalidUpdate )
{
    const HorizonSection& section = sectile_->getSection();
    const int res = resolution_;
    
    int valididx = 0;

    if ( !invalidUpdate )
    {
	const int normalstop = res < section.mlowestresidx_ ? 
	    section.normalstartidx_[res+1]-1 : section.mtotalnormalsize_-1;
	for ( int idx=section.normalstartidx_[res]; idx<=normalstop; idx++ )
	{
	    valididx = idx - section.normalstartidx_[res];
	    computeNormal( idx, (*normals_)[valididx] );
	}
    }
    else
    {
	const int sz = invalidnormals_.size();
	for ( int idx=0; idx<sz; idx++ )
	{
	    computeNormal( invalidnormals_[idx],
		(*normals_)[invalidnormals_[idx]]);
	}
    }

}


void TileResolutionData::setDisplayTransformation( const mVisTrans* t )
{ vertices_->setDisplayTransformation( t ); }


osg::BoundingBox& TileResolutionData::updateBBox()
{
    bbox_.init();
    osg::Vec3Array* arr = 
	dynamic_cast<osg::Vec3Array*>(vertices_->osgArray());
    for ( int idx=0; idx<arr->size(); idx++ )
    {
	const osg::Vec3d& pos = (*arr)[idx];
	if ( mIsDef(pos) )
	    bbox_.expandBy( pos) ;
    }
    return bbox_;
}


void TileResolutionData::setPos( int row, int col, const Coord3& pos )
{
    const HorizonSection& section = sectile_->getSection();
    const int spacing = section.spacing_[resolution_];

    int crdidx = getCoordIndex( row, col );
    
    osg::Vec3Array* arr = dynamic_cast<osg::Vec3Array*>(vertices_->osgArray());

    const bool olddefined = mIsDef((*arr)[crdidx]);
    const bool newdefined = pos.isDefined();

    if ( !olddefined && !newdefined )
	 return;

    if ( !newdefined )
	(*arr)[crdidx][2] =  mUdf(float);
    else
    {
	Coord3 crd;
	mVisTrans::transform( section.transformation_, pos, crd );

	(*arr)[crdidx] = osg::Vec3d( crd[0], crd[1], crd[2] );
	    bbox_.expandBy((*arr)[crdidx]);
    }

    char newstatus = mShouldRetesselate;
    if ( olddefined && !newdefined )
    {
	newstatus = mMustRetesselate;
    }

    if ( newdefined && !olddefined )
	nrdefinedpos_ ++;
    else if ( !newdefined && olddefined )
	nrdefinedpos_--;

    if ( newstatus>needsretesselation_ &&
	!(row%spacing) && !(col%spacing))
	needsretesselation_ = newstatus;
}


int TileResolutionData::getCoordIndex( int row, int col )
{ return row*nrnodeperside_ + col; }


bool TileResolutionData::tesselate( bool onlyifabsness )
{
    if ( resolution_<0 || needsretesselation_==mNoTesselationNeeded ||
	(needsretesselation_==mShouldRetesselate && onlyifabsness) )
	return false;

    trianglesps_->clear();
    wireframesps_->clear();
    linesps_->clear();
    pointsps_->clear();
    
   for ( int idx00=0; idx00<vertices_->size(); idx00++)
	tesselateCell( idx00 );

    updateprimitiveset_ = true;
    needsretesselation_ = mNoTesselationNeeded;

    return true;
}


void TileResolutionData::updatePrimitiveSets()
{
    if ( !updateprimitiveset_ )
	return;

    if( trianglesps_->size() )
    {
	trianglespsud_ = new osg::DrawElementsUShort( *trianglesps_ );
	setPrimitiveSet( Triangle, trianglespsud_ );
    }

    if( linesps_->size() )
    {
	linespsud_ = new osg::DrawElementsUShort( *linesps_ );
	setPrimitiveSet( Line, linespsud_ );
    }

    if( pointsps_->size() )
    {
	pointspsud_ = new osg::DrawElementsUShort( *pointsps_ );
	setPrimitiveSet( Point, pointspsud_ );
    }

    if( wireframesps_->size() && sectile_->getUseWireframe() )
    {
	wireframespsud_ = new osg::DrawElementsUShort( *wireframesps_ );
	setPrimitiveSet( WireFrame, wireframespsud_ );
    }

    updateprimitiveset_ = false;
}


void TileResolutionData::setPrimitiveSet( PrimitiveSetType type, 
				       osg::DrawElementsUShort* drawelem )
{
    if( !drawelem )
	return;

    osg::Geometry* geom = geoms_[type];

    if ( drawelem->size() )
    {
	if ( geom->getPrimitiveSetIndex( drawelem ) == 
	    geom->getNumPrimitiveSets() )
	    geom->addPrimitiveSet( drawelem );
    }
    else
    {
	const int idx = geom->getPrimitiveSetIndex( drawelem );
	if ( idx!=geom->getNumPrimitiveSets() )
	    geom->removePrimitiveSet(idx,1);
    }
}


void TileResolutionData::tesselateCell( int idx00 )
{
    const HorizonSection& section = sectile_->getSection();
    const int spacing = section.spacing_[resolution_];
    int size = section.mnrcoordspertileside_/spacing;
    int nmrowcol = resolution_ == 0 ? size : size +1 ;
    bool usewireframe = sectile_->getUseWireframe();
   
    int idx01 = idx00 + 1;
    int idx10 = idx00 + nmrowcol;
    int idx11 = idx00 + nmrowcol + 1;

    char c00 = vertices_->isDefined( idx00 );
    char c01 = vertices_->isDefined( idx01 ) << 1;
    char c10 = vertices_->isDefined( idx10 ) << 2;
    char c11 = vertices_->isDefined( idx11 ) << 3;

    if( !( (idx00+1)%nmrowcol ) )
    {
	c01 = 0;
	c11 = 0;
    }

    const char setup = c00 + c01 + c10 + c11;

    switch ( setup )
    {
    case 1:
	addPointIndex( idx00 );
	return;
    case 3:
	addLineIndex( idx00, idx01 );
	if ( usewireframe )
	    addWireframIndex( idx00, idx01 );
	break;
    case 5:
	addLineIndex( idx00, idx10 );
	if ( usewireframe )
	addWireframIndex( idx00, idx10 );
	break;
    case 7:
	addTriangleIndex( idx00, idx01, idx10 );
	if ( usewireframe )
	{
	    addWireframIndex( idx00, idx01 );
	    addWireframIndex( idx00, idx10 );
	}
	addLineIndex( idx00, idx01 );
	addLineIndex( idx00, idx10 );
	break;
    case 9:
	addPointIndex( idx00 );
	break;
    case 11:
	addTriangleIndex( idx00, idx01, idx11 );
	break;
    case 13:
	addTriangleIndex( idx00, idx10, idx11);
	addLineIndex( idx00, idx10 );
	if ( usewireframe )
	    addWireframIndex( idx00, idx10 );
	break;
    case 14:
	addTriangleIndex( idx01, idx10, idx11 );
	break;
    case 15:
	addTriangleIndex (idx00, idx01, idx10 );
	addTriangleIndex (idx01, idx10, idx11 );
	addLineIndex( idx00, idx01 );
	addLineIndex( idx00, idx10 );
	if ( usewireframe )
	{
	    addWireframIndex( idx00, idx01 );
	    addWireframIndex( idx00, idx10 );
	}
	break;
    }

}


void TileResolutionData::addLineIndex( int idx1, int idx2 )
{
    addIndex( linesps_, idx1 );
    addIndex( linesps_, idx2 );
}


void TileResolutionData::addPointIndex( int idx )
{
    addIndex( pointsps_, idx );
}


void TileResolutionData::addWireframIndex( int idx1, int idx2 )
{
   addIndex( wireframesps_, idx1 );
   addIndex( wireframesps_, idx2 );
}


void TileResolutionData::addTriangleIndex( int idx0, int idx1, int idx2 )
{
    const int pssize = trianglesps_->getNumIndices();
    if ( pssize > 2 )
    {
	const unsigned int lastidx = trianglesps_->index(pssize-1);
	if ( lastidx==idx1 && trianglesps_->index(pssize-2)==idx0 )
	{
	     trianglesps_->push_back( idx2 );
	     return;
	}
	trianglesps_->push_back( lastidx );
    }

    if ( pssize>2 )
	trianglesps_->push_back( idx0 );
    
    trianglesps_->push_back( idx0 );
    trianglesps_->push_back( idx1 );
    trianglesps_->push_back( idx2 );
}


void TileResolutionData::addIndex( osg::DrawElementsUShort* drwelem, int idx )
{ drwelem->push_back( idx ); }


void TileResolutionData::computeNormal( int nmidx,osg::Vec3& normal ) const
{
    const HorizonSection& section = sectile_->getSection();
    RowCol origin_ = sectile_->getOrigin();

    const int res = resolution_;

    if ( !section.geometry_ )
	return;

    RowCol step;
    if ( section.userchangedisplayrg_ )
	step = RowCol( section.displayrrg_.step, section.displaycrg_.step );
    else
	step = RowCol( section.geometry_->rowRange().step,
	section.geometry_->colRange().step );

    const int normalrow = 
	(nmidx-section.normalstartidx_[res])/section.normalsidesize_[res];
    const int normalcol = 
	(nmidx-section.normalstartidx_[res])%section.normalsidesize_[res];

    const int row = origin_.row + step.row * normalrow*section.spacing_[res];
    const int col = origin_.col + step.col * normalcol*section.spacing_[res];

    const StepInterval<int> rowrange = section.geometry_->rowRange();
    const StepInterval<int> colrange = section.geometry_->colRange();
    bool beforefound, afterfound;
    Coord beforepos, afterpos;

    mGetGradient( row, RowCol(currow,col) );
    mGetGradient( col, RowCol(row,curcol) );

    normal[0] = drow*section.cosanglexinl_+dcol*section.sinanglexinl_;
    normal[1] = dcol*section.cosanglexinl_-drow*section.sinanglexinl_;
    normal[2] = -1; 
}


void TileResolutionData::buildGeometry()
{
    const HorizonSection& section = sectile_->getSection();
    const int spacing = section.spacing_[resolution_];

    if ( spacing<=0 )
	return;

    trianglesps_  =  new osg::DrawElementsUShort( GL_TRIANGLE_STRIP,0 );
    linesps_	  =  new osg::DrawElementsUShort( GL_LINES, 0 );
    pointsps_	  =  new osg::DrawElementsUShort( GL_POINTS ,0 );
    wireframesps_ =  new osg::DrawElementsUShort( GL_LINES ,0 );
    tcoords_	  =  new osg::Vec2Array;

    for ( int idx =0; idx < 4; idx++)
    {
        geodes_[idx] = new osg::Geode;
	geoms_[idx] = new osg::Geometry;
	osgswitch_->addChild( geodes_[idx] );
	geoms_[idx]->setVertexArray( vertices_->osgArray() );
	geoms_[idx]->setNormalArray( normals_ );
	geoms_[idx]->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );
	geoms_[idx]->setDataVariance( osg::Object::DYNAMIC );
	geodes_[idx]->addDrawable( geoms_[idx] );

	osg::ref_ptr<osg::LightModel> lightmodel = new osg::LightModel;
	lightmodel->setTwoSided( true );
	geoms_[idx]->getOrCreateStateSet()->setAttributeAndModes(lightmodel.get());

    }
    
    linecolor_->push_back( osg::Vec4d( 1, 0, 0, 0 ) );
    geoms_[Line]->setColorArray( linecolor_ );
    geoms_[Line]->setColorBinding( osg::Geometry::BIND_OVERALL );
    osg::ref_ptr<osg::Vec3Array> linenormal = new osg::Vec3Array;
    linenormal->push_back( osg::Vec3 ( 0.0f,-1.0f,0.0f ) );
    geoms_[Line]->setNormalArray( linenormal.get() );
    geoms_[Line]->setNormalBinding( osg::Geometry::BIND_OVERALL );

    linewidth_->setWidth(1.0);

    geoms_[Line]->getStateSet()->setAttributeAndModes( linewidth_ );
    geoms_[Line]->getStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    nrnodeperside_ = spacing == 1 ? (int)section.mnrcoordspertileside_/spacing :
			    (int)section.mnrcoordspertileside_/spacing +1 ;
    int coordsize = nrnodeperside_*nrnodeperside_;
    normals_->resize( coordsize );
    std::fill( normals_->begin(), normals_->end(), osg::Vec3f( 0, 0, -1 ) );
    vertices_->setAllPositions( 
	Coord3(  mUdf(float), mUdf(float), mUdf(float) ), coordsize, 0 );
    tcoords_->resize( coordsize );
}

}; // namespace visBase

