#ifndef vistopbotimage_h
#define	vistopbotimage_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Ranojay Sen
 Date:		June 2009
 RCS:		$Id$
________________________________________________________________________

-*/


#include "visbasemod.h"
#include "visobject.h"
#include "position.h"

namespace visBase
{

class TriangleStripSet;
class Image;

mExpClass(visBase) TopBotImage : public VisualObjectImpl
{
public:
    static TopBotImage*		create()
				mCreateDataObj(TopBotImage);

    void			setDisplayTransformation(const mVisTrans*);
    void			setPos(const Coord& tl,const Coord& br,
				       float z);
    const Coord&		topLeft() const	    { return pos0_.coord(); }
    const Coord&		bottomRight() const { return pos2_.coord(); }

    void			setImageFilename(const char*);
    const char*			getImageFilename() const;

    void			setTransparency(float); // 0-1
    float			getTransparency() const; // returns value 0-1
    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

protected:
    				~TopBotImage();

    void			updateCoords();

    TriangleStripSet*		imgshape_;
    Image*			image_;
    const mVisTrans*		trans_;
    Coord3			pos0_;
    Coord3			pos1_;
    Coord3			pos2_;
    Coord3			pos3_;
    BufferString		filenm_;
    
    static const char*		sKeyTopLeftCoord();
    static const char*		sKeyBottomRightCoord();
    static const char*		sKeyFileNameStr();
   
};

} // namespace visBase

#endif 

