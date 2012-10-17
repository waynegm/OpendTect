#ifndef visdragger_h
#define visdragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "position.h"
#include "visosg.h"

class Color;
namespace osgManipulator { class Dragger; }
namespace osg { class MatrixTransform; class Node; class Switch; }

namespace visBase
{

/*! \brief Class for simple draggers
*/

class Transformation;
    
mClass(visBase) DraggerBase : public DataObject
{
public:
    Notifier<DraggerBase>	started;
    Notifier<DraggerBase>	motion;
    Notifier<DraggerBase>	finished;
    
    void			setDisplayTransformation( const mVisTrans* );
    const mVisTrans*		getDisplayTransformation() const;

protected:
				DraggerBase();
    				~DraggerBase();
    
    const mVisTrans*		displaytrans_;
};


mClass(visBase) Dragger : public DraggerBase
{
public:
    static Dragger*		create()
    				mCreateDataObj(Dragger);

    enum Type			{ Translate1D, Translate2D, Translate3D,
    				  Scale3D };
    void			setDraggerType(Type);

    void			setPos(const Coord3&);
    Coord3			getPos() const;

    void			setSize(const Coord3&);
    Coord3			getSize() const;

    void			setRotation(const Coord3&,float);
    void			setDefaultRotation();

    void			turnOn(bool);
    bool			isOn() const;

    
    void			setOwnShape(DataObject*,
	    				    const char* partname );
    				/*!< Sets a shape on the dragger.
				    \note The object will not be reffed,
					  so it's up to the caller to make sure
					  it remains in memory */
    bool			selectable() const;

    NotifierAccess*		rightClicked() { return &rightclicknotifier_; }
    const TypeSet<int>*		rightClickedPath() const;
    const EventInfo*		rightClickedEventInfo() const;

protected:
    				~Dragger();
    void			triggerRightClick(const EventInfo* eventinfo);
    osg::Node*			osgNode();
    
    Notifier<Dragger>		rightclicknotifier_;
    const EventInfo*		rightclickeventinfo_;

    OsgRefMan<osg::Switch>		onoff_;
    OsgRefMan<osgManipulator::Dragger>	dragger_;
    OsgRefMan<osg::MatrixTransform>	positiontransform_;
};

} // namespace visBase

#endif

