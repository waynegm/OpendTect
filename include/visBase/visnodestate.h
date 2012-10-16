#ifndef visnodestate_h
#define visnodestate_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		Oct 2012
 RCS:		$Id: viscoord.h 26346 2012-09-24 05:48:51Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________


-*/

#include "visosg.h"
#include "objectset.h"

namespace osg { class StateSet; class StateAttribute; }

namespace visBase
{
    
/*!Baseclass for objects manipulating the osg::StateSet. */

class NodeState : public CallBacker
{ mRefCountImpl(NodeState);
public:

    void			setStateSet(osg::StateSet*);

protected:
				NodeState();
    
    template <class T> T*	addAttribute(T* a) { doAdd(a); return a; }

    
    ObjectSet<osg::StateAttribute>	attributes_;
    
private:
    void				doAdd(osg::StateAttribute*);
    void				doRemove(osg::StateAttribute*);
    OsgRefMan<osg::StateSet>		stateset_;
};

};

#endif

