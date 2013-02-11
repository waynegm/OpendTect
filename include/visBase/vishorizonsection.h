#ifndef vishorizonsection_h
#define vishorizonsection_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "arrayndimpl.h"
#include "rowcol.h"
#include "thread.h"
#include "visobject.h"
#include "geomelement.h"

class BinIDValueSet;
class Color;
class DataPointSet;

class ZAxisTransform;
class TaskRunner;

namespace osg
{
    class Group;
    class CullStack;
}


namespace Geometry { class BinIDSurface; }
namespace ColTab { class Sequence; class MapperSetup; }
namespace osgGeo { class LayeredTexture;  }

namespace visBase
{
    class TextureChannel2RGBA;    
    class HorizonSectionTile;
    class TextureChannels;

/*!Horizon geometry is divided into 64*64 pixel tiles. Each tile has it's own 
  glue edge to merge into it's neighbors in case of different resolutions. Each
  tile has it's own coordinates and normals, but they all share the same texture  coordinates since the have the same size. Each tile holds its wireframe. It 
  would only turn on wireframe or lines and points depends if you use wireframe
  or not. */

mExpClass(visBase) HorizonSection : public VisualObjectImpl
{
public:
    static HorizonSection*	create() mCreateDataObj(HorizonSection);

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
    void			setZAxisTransform(ZAxisTransform*,TaskRunner*);

    void			setRightHandSystem(bool);

    				//Texture information
    void                        useChannel(bool);
    int                         nrChannels() const;
    void                        addChannel();
    void                        removeChannel(int);
    void                        swapChannels(int,int);

    int                         nrVersions(int channel) const;
    void                        setNrVersions(int channel,int);
    int                         activeVersion(int channel) const;
    void                        selectActiveVersion(int channel,int);

    void			setColTabSequence(int channel,
	    					  const ColTab::Sequence&);
    const ColTab::Sequence*	getColTabSequence(int channel) const;
    void			setColTabMapperSetup(int channel,
						const ColTab::MapperSetup&,
						TaskRunner*);
    const ColTab::MapperSetup*	getColTabMapperSetup(int channel) const;
    const TypeSet<float>*	getHistogram(int channel) const;

    void			setTransparency(int ch,unsigned char yn);
    unsigned char		getTransparency(int ch) const;

    void			getDataPositions(DataPointSet&,double zoff,
	    					 int sid,TaskRunner*) const;
    void			setTextureData(int channel,const DataPointSet*,
	    				       int sid,TaskRunner*);
    const BinIDValueSet*	getCache(int channel) const;
    void			inValidateCache(int channel);

    void			setChannels2RGBA(TextureChannel2RGBA*);
    TextureChannel2RGBA*	getChannels2RGBA();
    const TextureChannel2RGBA*	getChannels2RGBA() const;
    TextureChannels*		getChannels() const	{ return channels_; }

    				//Geometry stuff
    void			setSurface(Geometry::BinIDSurface*,bool conn,
	    				   TaskRunner*);
    Geometry::BinIDSurface*	getSurface() const	{ return geometry_; }
    StepInterval<int>		displayedRowRange() const;
    StepInterval<int>		displayedColRange() const;
    void			setDisplayRange(const StepInterval<int>&,
	    					const StepInterval<int>&);

    void			useWireframe(bool);
    bool			usesWireframe() const;
    
    char			nrResolutions() const;
    char			currentResolution() const;
    void			setResolution(int,TaskRunner*);

    void			setWireframeColor(Color col);
    osgGeo::LayeredTexture*	getOsgTexture() const;
    void			updateAutoResolution( osg::CullStack* );
    void			updatePrimitiveSets();
    void			turnOsgOn( bool );


protected:
    				~HorizonSection();
    friend class		HorizonSectionTile;			
    friend class		HorizonTileRenderPreparer;
    friend class		TileResolutionData;
    void			surfaceChangeCB(CallBacker*);
    void			surfaceChange(const TypeSet<GeomPosID>*,
	    				      TaskRunner*);
    void			removeZTransform();
    void			updateZAxisVOI();

    void			updateTexture(int channel,const DataPointSet*,
	    				      int sectionid);

    void			updateTileTextureOrigin(const RowCol& texorig);
    void			updateTileArray();
    HorizonSectionTile*		createTile(int rowidx,int colidx);

    void			resetAllTiles(TaskRunner*);
    void			updateNewPoints(const TypeSet<GeomPosID>*,
	    					TaskRunner*);
    void			setSizeParameters();




    Geometry::BinIDSurface*	geometry_;
    RowCol			origin_;

    bool			userchangedisplayrg_;
    StepInterval<int>		displayrrg_;
    StepInterval<int>		displaycrg_;
    Threads::Mutex		updatelock_;
    ObjectSet<BinIDValueSet>	cache_;

    TextureChannels*		channels_;
    TextureChannel2RGBA*	channel2rgba_;
 
    Array2DImpl<HorizonSectionTile*> tiles_;
    bool			usewireframe_;

    const mVisTrans*		transformation_;
    ZAxisTransform*		zaxistransform_;
    int				zaxistransformvoi_; 
    				//-1 not needed by zaxistransform, -2 not set
				
    int				desiredresolution_;
    bool			ismoving_;
    double			cosanglexinl_;
    double			sinanglexinl_;
    float			rowdistance_;
    float			coldistance_;
    Material*			wireframematerial_;

    bool			tesselationlock_;

    int				mnrcoordspertileside_;
    int 			mtotalnrcoordspertile_;
    int 			mtilesidesize_;
    int 			mtilelastidx_;
    int 			mtotalnormalsize_;
    unsigned char 		mlowestresidx_;
    int 			mhorsectnrres_;

    int*			spacing_;
    int*			nrcells_;
    int*			normalstartidx_;
    int*			normalsidesize_;

    int				tesselationqueueid_;

    osg::Group*		    	osghorizon_;
    Threads::SpinLock		lock_;

    static const char*		sKeySectionID()	{ return "Section ID"; }
};

};


#endif
