#ifndef profilemodelcreator_h
#define profilemodelcreator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "multiid.h"
#include "bufstring.h"
#include "profilemodelfromeventdata.h"

class BufferStringSet;
class ProfileBase;
class ProfileModelBase;
class ProfilePosDataSet;
class ProfilePosProvider;
class TaskRunner;
class ZAxisTransform;
class ZValueProvider;
template <class T> class PolyLineND;



/*!\brief Base class for horizon users in profile mode */

mExpClass(Well) ProfileModelFromEventCreator
{
public:

    enum MovePol	{ MoveNone, MoveAll, MoveAbove, MoveBelow };

			ProfileModelFromEventCreator(ProfileModelBase&,
						     const ProfilePosProvider&,
						     int totalnrprofs=0);
    virtual		~ProfileModelFromEventCreator();

    void		preparePositions();
    void		prepareZTransform(TaskRunner* tr=0);
    void		setNewProfiles();
    bool		calculate();
    uiString		errMsg() const			{ return errmsg_; }

			// settable with default
    int			totalnrprofs_;
    MovePol		movepol_; // only for Single, Multi ignores

protected:

    ProfileModelBase&	model_;
    const ProfilePosProvider& profposprov_;
    ProfilePosDataSet&	ppds_;
    uiString		errmsg_;

    virtual bool	isSingleEvent() const		=0;
    float		getDepthVal(float pos,float z) const;
    void		addNewPositions();
    void		getKnownDepths(const ProfileModelFromEventData::Event&);
    void		setEventZVals(const ProfileModelFromEventData::Event&);
    virtual void	setZOffsets(const ProfileModelFromEventData::Event&)=0;
    void		setMarkerDepths(
				const ProfileModelFromEventData::Event&);
    void		sortMarkers();

    virtual bool	doCalculate()			= 0;
    void		fillCoords();

    int			addNewPPDsAfter(int,int,bool);
    void		addProfileToPPDs();
    void		interpolateZOffsets();
    bool		doPush(float,float) const;
    bool		doPull(float,float) const;

};


/*!\brief uses horizon to set depths of control profiles. By default uses
  first well profile for T2D conversion. */

mExpClass(Well) ProfileModelFromSingleEventCreator
				: public ProfileModelFromEventCreator
{ mODTextTranslationClass(ProfileModelFromSingleEventCreator);
public:

			ProfileModelFromSingleEventCreator(ProfileModelBase&,
				const ProfileModelFromEventData::Event&,
				const ProfilePosProvider&);

protected:

    const ProfileModelFromEventData::Event& event_;

    virtual bool	isSingleEvent() const		{ return true; }
    virtual void	setZOffsets(const ProfileModelFromEventData::Event&);
    virtual bool	doCalculate();

};


/*!\brief Uses multiple horizons in one go; does not use well coordinates but
 an external geometry.

 Important constraint: the horizon IDs must be sorted from top to bottom.

 */


mExpClass(Well) ProfileModelFromMultiEventCreator
				: public ProfileModelFromEventCreator
{ mODTextTranslationClass(ProfileModelFromMultiEventCreator);
public:

				ProfileModelFromMultiEventCreator(
					ProfileModelFromEventData&,
					const ProfilePosProvider&);
				~ProfileModelFromMultiEventCreator();

protected:

    void			reArrangeMarkers();
    void			interpolateMarkersBetweenEvents();
    virtual bool		doCalculate();
    virtual bool		isSingleEvent() const	{ return false;}
    virtual void		setZOffsets(
				    const ProfileModelFromEventData::Event&);
    int				getTopBottomEventMarkerIdx(
					     const ProfileBase&,int imarker,
					     bool findtop) const;
    bool			getTopBotEventValsWRTMarker(
					    const char* mnm,const ProfileBase&,
					    float& mdah,float& topevdah,
					    float& botevdah,float& relpos)const;

    ProfileModelFromEventData&	data_;
};


#endif
