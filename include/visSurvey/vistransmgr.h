#ifndef vistransmgr_h
#define vistransmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "commondefs.h"
#include "vistransform.h"

class HorSampling;
class InlCrlSystem;

namespace visSurvey
{

class Scene;

mClass(visSurvey) SceneTransformManager
{
public:
    			SceneTransformManager()
			    : scene_(0)
			    , curzscale_( defZStretch() )
    			{}
    
    mVisTrans*		createUTM2DisplayTransform(const HorSampling&) const;
    			//!<Given to all objects in XY-space
    
    mVisTrans*		createICRotationTransform(const HorSampling&) const;
    			//!<Parent of all objects in IC space
    
    mVisTrans*		createICScalingTransform(const HorSampling&) const;
    			//!<Given to all objects in IC space

    void		computeICRotationTransform(const InlCrlSystem&,
					      	   mVisTrans* rotation,
						   mVisTrans* disptrans) const;

    static void		setZScale(mVisTrans*,float);
    float		defZStretch() const	{ return 2; }
    const char*		zStretchStr() const	{ return "Z Stretch"; }
    const char*		zOldStretchStr() const	{ return "Z Scale"; }

    void		setCurrentScene( Scene* scn ) { scene_ = scn; }
    Scene*		currentScene() const	{ return scene_; }

protected:

    Scene*		scene_;
    float		curzscale_;
};


mGlobal(visSurvey) SceneTransformManager& STM();

} // namespace visSurvey

#endif

