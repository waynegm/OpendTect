#ifndef vistext_h
#define vistext_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-22-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visosg.h"
#include "fontdata.h"
#include "visobject.h"
#include "position.h"

namespace osgText { class Text; class Font; }

namespace osg { class Drawable; class Vec3f; class Geode; }

namespace visBase
{

mClass(visBase) Text
{
public:
				Text();
				~Text();
    enum			Justification { Left, Right, Center };
    
    void			setPosition(const osg::Vec3f&);
    void			setPosition(const Coord3&);
    Coord3			getPosition() const;

    void			setFontData(const FontData&);
    const FontData&		getFontData() const	{ return fontdata_; }

    void			setText(const char*);
    void			getText(BufferString&) const;

    void			setJustification(Justification);

    osg::Drawable&		getDrawable();
    const osg::Drawable&	getDrawable() const;
    
protected:

    OsgRefMan<osgText::Text>	text_;
    
    FontData			fontdata_;
};
    
    
mClass(visBase) OsgFontCreator
{
public:
    virtual			~OsgFontCreator() 			{}
    static osgText::Font*	create(const FontData&);
protected:
    static void			setCreator(OsgFontCreator*);
    virtual osgText::Font*	createFont(const FontData&)		= 0;
};
    
    
mClass(visBase) Text2 : public VisualObjectImpl
{
public:
    static Text2*		create()
				mCreateDataObj(Text2);
				    
    int				nrTexts() const		{ return texts_.size();}
    int				addText();
    void			removeText(const Text*);
    
    void			setFontData(const FontData&);
    
    const Text*			text(int idx=0) const;
    Text*			text(int idx=0);
	
protected:
    OsgRefMan<osg::Geode>	geode_;
    ManagedObjectSet<Text>	texts_;
};


}; // Namespace

#endif

