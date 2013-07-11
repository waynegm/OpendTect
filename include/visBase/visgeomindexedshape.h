#ifndef visgeomindexedshape_h
#define visgeomindexedshape_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "valseries.h"
#include "visobject.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "visshape.h"

namespace Geometry { class IndexedGeometry; }
class TaskRunner;
class DataPointSet;

namespace visBase
{
class Transformation;
class Coordinates;
class Normals;
class TextureCoords;
class ForegroundLifter;
class VertexShape;

/*!Visualisation for Geometry::IndexedShape. */

mExpClass(visBase) GeomIndexedShape : public VisualObjectImpl
{
public:
    static GeomIndexedShape*	create()
				mCreateDataObj(GeomIndexedShape);

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setSurface(Geometry::IndexedShape*,
	    				   TaskRunner* = 0);
    				//!<Does not become mine, should remain
				//!<in memory

    bool			touch(bool forall,TaskRunner* =0);

    void			set3DLineRadius(float radius,
	    					bool constantonscreen=true,
						float maxworldsize=-1);
    				/*!<If radius is less than 0, a normal
				    line will be drawn. */
    void			renderOneSide(int side);
    				/*!< 0 = visisble from both sides.
				     1 = visible from positive side
				     -1 = visible from negative side. */

    void			enableColTab(bool);
    bool			isColTabEnabled() const;
    void			setDataMapper(const ColTab::MapperSetup&,
	    				      TaskRunner*);
    const ColTab::MapperSetup*	getDataMapper() const;
    void			setDataSequence(const ColTab::Sequence&);
    const ColTab::Sequence*	getDataSequence() const;

    void			getAttribPositions(DataPointSet&,
					mVisTrans* extratrans,
	    				TaskRunner*) const;
    void			setAttribData(const DataPointSet&,
	    				TaskRunner*);

    void			setMaterial(Material*);
    void			updateMaterialFrom(const Material*);

protected:
				~GeomIndexedShape();
    void			reClip();
    void			mapAttributeToColorTableMaterial();
    void			matChangeCB(CallBacker*);
    void			updateGeometryMaterial();
    
    mExpClass(visBase)			ColorHandler
    {
    public:
					ColorHandler();
					~ColorHandler();
	ColTab::Mapper			mapper_;
	ColTab::Sequence                sequence_;
	visBase::Material*		material_;
	ArrayValueSeries<float,float>	attributecache_;
    };

    static const char*			sKeyCoordIndex() { return "CoordIndex";}

    ColorHandler*				colorhandler_;

    float					lineradius_;
    bool					lineconstantonscreen_;
    float					linemaxsize_;

    Geometry::IndexedShape*			shape_;
    VertexShape*				vtexshape_;
    bool					colortableenabled_ ;
    int						renderside_;
       					    /*!< 0 = visisble from both sides.
					       1 = visible from positive side
					      -1 = visible from negative side.*/
    Material*					singlematerial_;
    Material*					coltabmaterial_;
    ColTab::Sequence		                sequence_;
   
};

};
	
#endif

