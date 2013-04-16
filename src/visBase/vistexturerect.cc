/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vistexturerect.h"

#include "cubesampling.h"
#include "vistexturechannels.h"


#include <osgGeo/TexturePlane>

mCreateFactoryEntry( visBase::TextureRectangle );

using namespace visBase;


TextureRectangle::TextureRectangle()
    : VisualObjectImpl( false )
    , textureplane_( new osgGeo::TexturePlaneNode )
{
    textureplane_->ref();
    addChild( textureplane_ );
}


TextureRectangle::~TextureRectangle()
{
    textureplane_->unref();
}


void TextureRectangle::setTextureChannels( visBase::TextureChannels* channels )
{
    channels_ = channels;
    textureplane_->setLayeredTexture( channels_->getOsgTexture() );
}


void TextureRectangle::setCenter( const Coord3& center )
{
    osg::Vec3 osgcenter;
    mVisTrans::transform( displaytrans_, center, osgcenter );
    textureplane_->setCenter( osgcenter );
}


Coord3 TextureRectangle::getCenter() const
{
    Coord3 res;
    mVisTrans::transformBack( displaytrans_, textureplane_->getCenter(), res );
    return res;
}


void TextureRectangle::setWidth( const Coord3& width )
{
    osg::Vec3f osgwidth;
    mVisTrans::transformDir( displaytrans_, width, osgwidth );
    textureplane_->setWidth( osgwidth );
}


Coord3 TextureRectangle::getWidth() const
{
    Coord3 res;
    mVisTrans::transformBackDir( displaytrans_, textureplane_->getWidth(), res);
    return res;
}


void TextureRectangle::swapTextureAxes( bool yn )
{
    textureplane_->swapTextureAxes( yn );
}


bool TextureRectangle::areTextureAxesSwapped() const
{
    return textureplane_->areTextureAxesSwapped();
}
    

void TextureRectangle::setDisplayTransformation( const mVisTrans* tr )
{
    const Coord3 center = getCenter();
    const Coord3 width = getWidth();
    displaytrans_ = tr;
    setCenter( center );
    setWidth( width );
}
