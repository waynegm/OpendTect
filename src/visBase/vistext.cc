/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vistext.h"

#include "iopar.h"
#include "vistransform.h"
#include "vismaterial.h"
#include "vispickstyle.h"
#include "separstr.h"
#include "keystrs.h"
#include "dirlist.h"
#include "oddirs.h"
#include "filepath.h"


#include <osgText/Text>

mCreateFactoryEntry( visBase::Text2 );


namespace visBase
{

Text::Text()
    : text_( new osgText::Text )
{
    text_->setAxisAlignment( osgText::TextBase::SCREEN );
    text_->setCharacterSizeMode(osgText::TextBase::SCREEN_COORDS );
    setFontData( fontdata_ ); //trigger update of font_
}


Text::~Text()
{
}
    
    
osg::Drawable& Text::getDrawable()
{ return *text_; }


const osg::Drawable& Text::getDrawable() const
{ return *text_; }
    

void Text::setPosition( const osg::Vec3f& pos )
{
    text_->setPosition( pos );
}
    
    
void Text::setPosition( const Coord3& pos )
{
    setPosition( osg::Vec3((float) pos.x, (float) pos.y, (float) pos.y ) );
}
    
    
Coord3 Text::getPosition() const
{
    const osg::Vec3 pos = text_->getPosition();
    return Coord3( pos[0], pos[1], pos[2] );
}
    
    
void Text::setFontData( const FontData& fd )
{
    fontdata_ = fd;
    
    osg::ref_ptr<osgText::Font> osgfont = OsgFontCreator::create( fontdata_ );
    if ( osgfont )
	text_->setFont( osgfont );
    text_->setCharacterSize( fontdata_.pointSize() );
}



void Text::setText( const char* newtext )
{
    text_->setText( newtext );
}
    
    
void Text::getText( BufferString& res ) const
{
    std::string str = text_->getText().createUTF8EncodedString();
    res = str.data();
}


void Text::setJustification( Justification just )
{
    if ( just==Center )
	text_->setAlignment(osgText::TextBase::CENTER_CENTER );
    else if ( just==Left )
	text_->setAlignment( osgText::TextBase::LEFT_CENTER );
    else
	text_->setAlignment( osgText::TextBase::RIGHT_CENTER );
}

    
Text2::Text2()
    : VisualObjectImpl( false )
    , texts_( false )
    , geode_( new osg::Geode )
{
    geode_->setNodeMask( ~visBase::BBoxTraversal );
    addChild( geode_ );
}
    

int Text2::addText()
{
    Text* newtext = new Text;
    texts_ += newtext;
    geode_->addDrawable( &newtext->getDrawable() );
    return texts_.size()-1;
}
    
    
void Text2::removeText( const Text* txt )
{
    const int idx = texts_.indexOf( txt );
    if ( idx<0 )
	return;
    
    geode_->removeDrawable( &texts_[idx]->getDrawable() );
    texts_.remove( idx );
}

    
const Text* Text2::text( int idx ) const
{
    return texts_.validIdx( idx ) ? texts_[idx] : 0;
}
 
    
Text* Text2::text( int idx )
{
    if ( !idx && !texts_.size() )
	addText();
    
    return texts_.validIdx( idx ) ? texts_[idx] : 0;
}
    
    
void Text2::setFontData( const FontData& fd )
{
    for ( int idx=0; idx<texts_.size(); idx++ )
	texts_[idx]->setFontData( fd );
}
    
    
PtrMan<OsgFontCreator> creator = 0;
    
    
osgText::Font* OsgFontCreator::create(const FontData& fd)
{
    return creator ? creator->createFont(fd) : 0;
}
    

void OsgFontCreator::setCreator( OsgFontCreator* cr)
{
    creator = cr;
}

    
}; // namespace visBase
