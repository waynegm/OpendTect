/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vismaterial.h"
#include "visosg.h"
#include "iopar.h"
#include "task.h"

#include <osg/Material>
#include <osg/Array>

namespace visBase
{


class OsgColorArrayUpdator: public ParallelTask			
{
public:
    OsgColorArrayUpdator(Material* p, const od_int64 size, 
		    const TypeSet<float>&, const TypeSet<float>&);
    od_int64	totalNr() const { return totalnrcolors_; }

protected:
    bool	doWork(od_int64 start, od_int64 stop, int);
    bool	doPrepare(int);
    od_int64	nrIterations() const { return totalnrcolors_; }

private:
    Material* material_;
    Threads::Atomic<od_int64>	totalnrcolors_;
    const TypeSet<float> diffintensity_;
    const TypeSet<float> transparency_;

};


OsgColorArrayUpdator::OsgColorArrayUpdator( Material* p, const od_int64 size,
				  const TypeSet<float>& diffintensity, 
				  const TypeSet<float>& transparency)
    : material_( p )
    , totalnrcolors_( size )
    , diffintensity_( diffintensity )
    , transparency_ ( transparency )
{
}

bool OsgColorArrayUpdator::doPrepare(int)
{
    if (mGetOsgVec4Arr(material_->osgcolorarray_) )
	mGetOsgVec4Arr(material_->osgcolorarray_)->resize( 
				material_->colors_.size() );
    
    if( diffintensity_.size() ==1 || 
    diffintensity_.size() < material_->colors_.size() ) 
    {
	while (material_->diffuseintensity_.size()<= material_->colors_.size())
	{
	    material_->diffuseintensity_+= 0.8;
	    material_->transparency_+= 0.0;
	}
    }

    return true;
}


bool OsgColorArrayUpdator::doWork(od_int64 start,od_int64 stop,int)
{
    for ( int idx = mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	if ( diffintensity_.size() ==1 || 
	    diffintensity_.size() < material_->colors_.size() )
	{
	    material_->setMinNrOfMaterials(idx);
	    material_->diffuseintensity_[idx] = diffintensity_[0]; 
	    material_->transparency_[idx] = transparency_[0];
	}
	material_->updateOsgColor( idx, false );
    }
    return true;
}



const char* Material::sKeyColor()		{ return "Color"; }
const char* Material::sKeyAmbience()		{ return "Ambient Intensity"; }
const char* Material::sKeyDiffIntensity()	{ return "Diffuse Intensity"; }
const char* Material::sKeySpectralIntensity()	{ return "Specular Intensity"; }
const char* Material::sKeyEmmissiveIntensity()	{ return "Emmissive Intensity";}
const char* Material::sKeyShininess()		{ return "Shininess"; }
const char* Material::sKeyTransparency()	{ return "Transparency"; }

Material::Material()
    : material_( addAttribute(new osg::Material) )
    , osgcolorarray_( 0 )
    , ambience_( 0.8 )
    , specularintensity_( 0 )
    , emmissiveintensity_( 0 )
    , shininess_( 0 )
    , change( this )
{
    material_->ref();
    setColorMode( Off );
    setMinNrOfMaterials(0);
    updateOsgColor(0);
}


Material::~Material()
{
    material_->unref();
    if ( osgcolorarray_ ) osgcolorarray_->unref();
}


#define mSetProp( prop ) prop = mat.prop
void Material::setFrom( const Material& mat )
{
    setColors( mat.colors_, false );
    setPropertiesFrom( mat );
}


void Material::setPropertiesFrom( const Material& mat )
{
    mSetProp( diffuseintensity_ );
    mSetProp( ambience_ );
    mSetProp( specularintensity_ );
    mSetProp( emmissiveintensity_ );
    mSetProp( shininess_ );
    mSetProp( transparency_ );

    synchronizingOsgColorArray();
}


void Material::setColors( const TypeSet<Color>& colors, bool synchronizing )
{
    colors_.erase();
    colors_ = colors;
    if ( synchronizing )
	synchronizingOsgColorArray();
}


void Material::synchronizingOsgColorArray()
{
    OsgColorArrayUpdator updator( 
	this, colors_.size(), diffuseintensity_, transparency_);
    TaskRunner tr;

    if( TaskRunner::execute( &tr,updator ) )
	change.trigger();

}


void Material::setColor( const Color& n, int idx )
{
    setMinNrOfMaterials(idx);
    if ( colors_[idx]==n )
	return;

    colors_[idx] = n;
    updateOsgColor( idx );
}


const Color& Material::getColor( int idx ) const
{
    if ( colors_.validIdx(idx) )
	return colors_[idx];

    return colors_[0];
}


void Material::removeColor( int idx )
{
     if ( colors_.validIdx(idx) && colors_.size()>1 )
     {
	 colors_.removeSingle( idx );
	 transparency_.removeSingle( idx );
	 diffuseintensity_.removeSingle( idx );

	 for ( int idy=idx; idy<colors_.size(); idy++ )
	     updateOsgColor( idy );
     }
     else
     {
	 pErrMsg("Removing invalid index or last color.");
     }
}


void Material::setDiffIntensity( float n, int idx )
{
    setMinNrOfMaterials(idx);
    diffuseintensity_[idx] = n;
    updateOsgColor( idx );
}


float Material::getDiffIntensity( int idx ) const
{
    if ( idx>=0 && idx<diffuseintensity_.size() )
	return diffuseintensity_[idx];
    return diffuseintensity_[0];
}


void Material::setTransparency( float n, int idx )
{
    setMinNrOfMaterials(idx);
    transparency_[idx] = n;
    updateOsgColor( idx );
}


float Material::getTransparency( int idx ) const
{
    if ( idx>=0 && idx<transparency_.size() )
	return transparency_[idx];
    return transparency_[0];
}


#define mSetGetProperty( Type, func, var ) \
void Material::set##func( Type n ) \
{ \
    var = n; \
    updateOsgColor( 0 ); \
} \
Type Material::get##func() const \
{ \
    return var; \
}


mSetGetProperty( float, Ambience, ambience_ );
mSetGetProperty( float, SpecIntensity, specularintensity_ );
mSetGetProperty( float, EmmIntensity, emmissiveintensity_ );
mSetGetProperty( float, Shininess, shininess_ );


#define mGetOsgCol( col, fac, transp ) \
    osg::Vec4( col.r()*fac/255, col.g()*fac/255, col.b()*fac/255, 1.0-transp )


void Material::updateOsgColor( int idx, bool trigger)
{
    const osg::Vec4 diffuse =
	mGetOsgCol( colors_[idx], diffuseintensity_[idx], transparency_[idx] );
    
    if ( !idx )
    {
	
	material_->setAmbient( osg::Material::FRONT_AND_BACK,
		mGetOsgCol(colors_[0],ambience_,transparency_[0]) );
	material_->setSpecular( osg::Material::FRONT_AND_BACK,
		mGetOsgCol(colors_[0],specularintensity_,transparency_[0]) );
	material_->setEmission( osg::Material::FRONT_AND_BACK,
		mGetOsgCol(colors_[0],emmissiveintensity_,transparency_[0]) );

	material_->setShininess(osg::Material::FRONT_AND_BACK, shininess_ );
	material_->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse );

	if ( !osgcolorarray_ )
	    createOsgColorArray();
	osg::Vec4Array& colarr = *mGetOsgVec4Arr(osgcolorarray_);
	if ( colarr.size() )
	    colarr[0] = diffuse;
	 else
	    colarr.push_back( diffuse );
    }
    else
    {
	if ( !osgcolorarray_ )
	{
	    createOsgColorArray();
	    mGetOsgVec4Arr(osgcolorarray_)->push_back( diffuse );
	}
	else
	{
	    osg::Vec4Array& colarr = *mGetOsgVec4Arr(osgcolorarray_);
	    
	    if ( colarr.size()<=idx )
		colarr.resize( idx+1 );
	    
	    colarr[idx] = diffuse;
	}
    }

    if( trigger )
	change.trigger();
}


int Material::nrOfMaterial() const
{ return colors_.size(); }


void Material::setMinNrOfMaterials(int minnr)
{
    while ( colors_.size()<=minnr )
    {
	colors_ += Color(179,179,179);
	diffuseintensity_ += 0.8;
	transparency_ += 0.0;
    }
}


int Material::usePar( const IOPar& iopar )
{
    int r,g,b;

    if ( iopar.get( sKeyColor(), r, g, b ) )
	setColor( Color( r,g,b ));

    float val;
    if ( iopar.get( sKeyAmbience(), val ))
	setAmbience( val );

    if ( iopar.get( sKeyDiffIntensity(), val ))
	setDiffIntensity( val );

    if ( iopar.get( sKeySpectralIntensity(), val ))
	setSpecIntensity( val );

    if ( iopar.get( sKeyEmmissiveIntensity(), val ))
	setEmmIntensity( val );

    if ( iopar.get( sKeyShininess(), val ))
	setShininess( val );

    if ( iopar.get( sKeyTransparency(), val ))
	setTransparency( val );

    return 1;
}


void Material::fillPar( IOPar& iopar ) const
{
    Color tmpcolor = getColor();
    iopar.set( sKeyColor(), tmpcolor.r(), tmpcolor.g(), tmpcolor.b() ) ;
    iopar.set( sKeyAmbience(), getAmbience() );
    iopar.set( sKeyDiffIntensity(), getDiffIntensity() );
    iopar.set( sKeySpectralIntensity(), getSpecIntensity() );
    iopar.set( sKeyEmmissiveIntensity(), getEmmIntensity() );
    iopar.set( sKeyShininess(), getShininess() );
    iopar.set( sKeyTransparency(), getTransparency() );
}
    
    
const osg::Array* Material::getColorArray() const
{
    return osgcolorarray_;
}
    

osg::Array* Material::getColorArray()
{
    if ( !osgcolorarray_ )
	createOsgColorArray();

    return osgcolorarray_;
}


void Material::createOsgColorArray()
{
    if ( osgcolorarray_ )
	return;
    
    osgcolorarray_ = new osg::Vec4Array;
    osgcolorarray_->ref();
}


void Material::setColorMode( ColorMode mode )
{
    if ( mode == Ambient ) 
	material_->setColorMode( osg::Material::AMBIENT );
    else if ( mode == Diffuse )
	material_->setColorMode( osg::Material::DIFFUSE );
    else if ( mode == Specular )
	material_->setColorMode( osg::Material::SPECULAR );
    else if ( mode == Emission )
	material_->setColorMode( osg::Material::EMISSION );
    else if ( mode == AmbientAndDiffuse )
	material_->setColorMode( osg::Material::AMBIENT_AND_DIFFUSE );
    else
	material_->setColorMode( osg::Material::OFF );
}


Material::ColorMode Material::getColorMode() const
{ 
    if ( material_->getColorMode() == osg::Material::AMBIENT )
	return Ambient;
    if ( material_->getColorMode() == osg::Material::DIFFUSE )
	return Diffuse;
    if ( material_->getColorMode() == osg::Material::SPECULAR )
	return Specular;
    if ( material_->getColorMode() == osg::Material::EMISSION )
	return Emission;
    if ( material_->getColorMode() == osg::Material::AMBIENT_AND_DIFFUSE )
	return AmbientAndDiffuse;

    return Off;
}


void	Material::clear()
{
    if ( colors_.size() )
        colors_.erase();
    diffuseintensity_.erase();
    transparency_.erase();
    if ( osgcolorarray_ )
        mGetOsgVec4Arr( osgcolorarray_ )->clear();
}


}; // namespace visBase
