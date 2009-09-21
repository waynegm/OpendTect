#ifndef visscene_h
#define visscene_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visscene.h,v 1.19 2009-09-21 07:10:27 cvskarthika Exp $
________________________________________________________________________


-*/


#include "sets.h"
#include "visdatagroup.h"

class SoEnvironment;

namespace visBase
{
    class SelectionManager;
    class EventCatcher;
    class PolygonOffset;
    class DirectionalLight;

/*!\brief
    Scene manages all DataObjects and has some managing
    functions such as the selection management and variables that should
    be common for the whole scene.
*/

mClass Scene : public DataObjectGroup
{
public:
    static Scene*	create()
			mCreateDataObj(Scene);

    void		addObject(DataObject*);
    void		insertObject(int idx,DataObject*);

    void		setAmbientLight(float);
    float		ambientLight() const;

    void		setDirectionalLight(const DirectionalLight&);
    void		setDirectionalLight(int, int, float);
    DirectionalLight*	getDirectionalLight() const;

    bool		blockMouseSelection(bool yn);
    			/*!<\returns previous status. */

    SoNode*		getInventorNode();
    EventCatcher&	eventCatcher();

protected:
    virtual		~Scene();
    EventCatcher&	events_;

private:
    int			mousedownid_;

    void		mousePickCB(CallBacker*);

    SoEnvironment*	environment_;
    PolygonOffset*	polygonoffset_;
    DirectionalLight*	directionallight_;
    SoGroup*		selroot_;
    bool		blockmousesel_;
};

};


#endif
