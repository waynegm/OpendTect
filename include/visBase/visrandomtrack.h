#ifndef visrandomtrack_h
#define visrandomtrack_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"
#include "ranges.h"
#include "visosg.h"

namespace osgGeo
{
    class TexturePanelStripNode;
}

template <class T> class Array2D;
class IOPar;

namespace visBase
{
class EventCatcher;
class Material;
class VisColorTab;
class TextureChannels;

/*!\brief

*/

mExpClass(visBase) RandomTrack : public VisualObjectImpl
{
public:
    static RandomTrack*		create()
				mCreateDataObj(RandomTrack);

    void			showDragger( bool yn );
    bool			isDraggerShown() const;
    void			moveObjectToDraggerPos();
    void			moveDraggerToObjectPos();

    int				nrKnots() const;
    void			setTrack(const TypeSet<Coord>&);
    void			addKnot(const Coord&);
    void			insertKnot(int idx,const Coord&);
    Coord			getKnotPos(int) const;
    Coord			getDraggerKnotPos(int) const;
    void			setKnotPos(int,const Coord&);
    void			setDraggerKnotPos(int,const Coord&);
    void			removeKnot(int);

    void			setDepthInterval(const Interval<float>&);
    const Interval<float>	getDepthInterval() const;
    void			setDraggerDepthInterval(const Interval<float>&);
    const Interval<float>	getDraggerDepthInterval() const;

    void			setXrange( const StepInterval<float>& );
    void			setYrange( const StepInterval<float>& );
    void			setZrange( const StepInterval<float>& );
    				/*!< sets limits for dragging */

    void			setDraggerSize( const Coord3& );
    Coord3			getDraggerSize() const;

    void			setClipRate( Interval<float> );
    Interval<float>		clipRate() const;

    void			setAutoScale( bool yn );
    bool			autoScale() const;

    void			setColorTab( VisColorTab& );
    VisColorTab&		getColorTab();
    const TypeSet<float>&	getHistogram() const;

    void			setResolution(int);
    int				getResolution() const;

    void			useTexture(bool);
    bool			usesTexture() const;

    int				getSectionIdx() const { return sectionidx_; }

    void			setData(int sectn,const Array2D<float>*,int tp);
    				/*!< section ranges from 0 to nrKnots-2 */

    void			setColorPars(bool,bool,const Interval<float>&);
    const Interval<float>&	getColorDataRange() const;

    Notifier<RandomTrack>	knotnrchange;
    CNotifier<RandomTrack,int>	knotmovement;
    				/*!< Sends the index of the knot moving */

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
    
protected:
    				~RandomTrack();
    void			rebuild();
    void			createDragger();
    
    osgGeo::TexturePanelStripNode*	node_;
    
    RefMan<TextureChannels>	channels_;

    Interval<float>		depthrg_;
    TypeSet<Coord>		knots_;
    int				sectionidx_;

    RefMan<const mVisTrans>	transformation_;
};

};

#endif


