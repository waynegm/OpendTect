/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 1999
-*/


#include "raytrace1d.h"

#include "ailayer.h"
#include "arrayndimpl.h"
#include "iopar.h"
#include "genericnumer.h"
#include "mathfunc.h"


bool RayTracer1D::Setup::usePar( const IOPar& par )
{
    return par.getYN( sKeyPWave(), pdown_, pup_ ) &&
	   par.get( sKeySRDepth(), sourcedepth_, recieverdepth_ );
}


void RayTracer1D::Setup::fillPar( IOPar& par ) const 
{
    par.setYN( sKeyPWave(), pdown_, pup_ );
    par.set( sKeySRDepth(), sourcedepth_, recieverdepth_ );
}


RayTracer1D::RayTracer1D( const RayTracer1D::Setup& s )
    : setup_( s )
    , receiverlayer_( 0 )
    , sourcelayer_( 0 )
    , relsourcedepth_( 0 )
    , relreceiverdepth_( 0 )			
    , sini_( 0 )
{}					       	


RayTracer1D::~RayTracer1D()
{ delete sini_; }


void RayTracer1D::setModel( bool pwave, const TypeSet<AILayer>& lys )
{
    if ( pwave )
    {
	pmodel_ = &lys;
    }
    else 
    {
	smodel_ = &lys;
    }
}


void RayTracer1D::setOffsets( const TypeSet<float>& offsets )
{
    offsets_ = offsets;
}


od_int64 RayTracer1D::nrIterations() const
{
    return pmodel_ ? pmodel_->size() : smodel_->size();
}


bool RayTracer1D::doPrepare( int nrthreads )
{
    if ( pmodel_ && smodel_ && pmodel_->size()!=smodel_->size() )
    {
	pErrMsg("P and S model sizes must be identical" );
	return false;
    }
    const int layersize = nrIterations();

    float maxvel = 0;
    velmax_.erase();
    for ( int layer=0; layer<layersize; layer++ )
    {
	if ( !layer )
	{
	    velmax_ += 0;
	    continue;
	}

	if ( setup_.pdown_ || setup_.pup_ )
	{
	    if ( (*pmodel_)[layer-1].vel_ > maxvel )
		maxvel = (*pmodel_)[layer-1].vel_;
	}
	else if ( (*smodel_)[layer-1].vel_ > maxvel )
	    maxvel = (*smodel_)[layer-1].vel_;

	velmax_ += maxvel;
    }

    return true;
}


bool RayTracer1D::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    const int offsz = offsets_.size();
    const int firstreflection = 1 +
	(sourcelayer_ > receiverlayer_ ? sourcelayer_ : receiverlayer_);
    for ( int layer=start; layer<=stop; layer++ )
    {
	const int usedlayer = firstreflection > layer ? firstreflection : layer;
	float rayparam = 0;
	for ( int osidx=0; osidx<offsz; osidx++ )
	{
	    rayparam = findRayParam( usedlayer, offsets_[osidx], rayparam );
	    if ( mIsUdf(rayparam) ) 
		continue;

	    if ( !compute(layer, osidx, rayparam) )
		return false;
	}
    }

    return true;
}


class OffsetFromRayParam : public FloatMathFunction
{
public:
    			OffsetFromRayParam( const RayTracer1D& rt, int layer )
			    : raytracer_( rt ), layer_( layer ) {}  
    float 		getValue(float rayparam) const
			{ return raytracer_.getOffset( layer_, rayparam ); }

protected:

    const RayTracer1D&	raytracer_;
    int			layer_;
};


float RayTracer1D::findRayParam( int layer, float offset, float seed ) const
{
    OffsetFromRayParam function( *this, layer );

    if ( !findValue(function,0,0.9999999/velmax_[layer],seed,offset,1e-10) )
	return mUdf(float);

    return seed;
}


float RayTracer1D::getOffset( int layer, float rayparam) const
{
    const TypeSet<AILayer>& dlayers = setup_.pdown_ ? *pmodel_ : *smodel_;
    const TypeSet<AILayer>& ulayers = setup_.pup_ ? *pmodel_ : *smodel_; 
    
    float xtot = 0;
    int idx = sourcelayer_ < receiverlayer_ ? sourcelayer_ : receiverlayer_;
    while ( idx<layer )
    {
	if ( idx >= sourcelayer_ )
	{
	    const float sinidn = rayparam * dlayers[idx].vel_;
	    const float factor = sinidn/sqrt(1.0-sinidn*sinidn);
	    const float rd = idx==sourcelayer_ ? relsourcedepth_ : 0;
	    xtot += ( dlayers[idx].depth_ - rd ) * factor;
	}
	
	if ( idx >= receiverlayer_ )
	{
	    const float siniup = rayparam * ulayers[idx].vel_;
	    const float factor = siniup/sqrt(1-siniup*siniup);
	    const float rd = idx==receiverlayer_ ? relreceiverdepth_ : 0;
	    xtot += ( ulayers[idx].depth_ - rd ) * factor; 
	}

	idx++;
    }
    
    return xtot;
}


float RayTracer1D::getSinAngle( int layer, int offset ) const
{
    if ( !sini_ || layer<0 || layer>=sini_->info().getSize(0) || 
	 offset<0 || offset>=sini_->info().getSize(1) )
    return mUdf(float);
    
    return sini_->get( layer, offset );
}


//float* AngleRayTracer::getSinAngleData() const
//{ return sini_ ? sini_->getData() : 0; }


bool RayTracer1D::compute( int layer, int offsetidx, float rayparam )
{
    const TypeSet<AILayer>& dlayers = setup_.pdown_ ? *pmodel_ : *smodel_;
    const float sinival = layer ? dlayers[layer-1].vel_ * rayparam : 0;
    sini_->set( layer, offsetidx, sinival );

    return true;
}
