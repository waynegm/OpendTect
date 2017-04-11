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
						     int totalnrprofs=0);
    virtual		~ProfileModelFromEventCreator();

    void		preparePositions();
    void		prepareZTransform(TaskRunner* tr=0);
    void		setNewProfiles();
    bool		go(TaskRunner* tr=0);
    uiString		warnMsg() const			{ return warnmsg_; }
    uiString		errMsg() const			{ return errmsg_; }

			// settable with default
    IOPar&		t2dpar_;
    int			totalnrprofs_;
    MovePol		movepol_; // only for Single, Multi ignores

protected:

    ProfileModelBase&	model_;
    ProfilePosDataSet&	ppds_;
    ZAxisTransform*	t2dtr_;
    bool		needt2d_;
    uiString		warnmsg_;
    uiString		errmsg_;

    virtual bool	isSingleEvent() const		=0;
    float		getDepthVal(float pos,float z) const;
    void		addNewPositions();
    void		getKnownDepths(const ProfileModelFromEventData::Event&);
    void		getEventZVals(const ProfileModelFromEventData::Event&);
    void		getZOffsets(const ProfileModelFromEventData::Event&);
    float		getInterpolatedDepth(int ippd,
				const ProfileModelFromEventData::Event&) const;
    bool		interpolateIntersectMarkersUsingEV(
				const ProfileModelFromEventData::Event&);
    void		setMarkerDepths(
				const ProfileModelFromEventData::Event&);
    void		sortMarkers();

    virtual bool	doGo(TaskRunner*)		= 0;
    virtual void	fillCoords()			= 0;
    virtual bool	setIntersectMarkers()		= 0;

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
				const ProfileModelFromEventData::Event&);

    static bool		canDoWork(const ProfileModelBase&);

			// settable
    float		sectionangle_;	// only used if model has exactly 1 well
    float		sectionlength_; // ditto

protected:

    const ProfileModelFromEventData::Event& event_;

    virtual bool	isSingleEvent() const		{ return true; }
    virtual bool	setIntersectMarkers();
    virtual bool	doGo(TaskRunner*);
    virtual void	fillCoords();

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
					ProfileModelFromEventData&);
				~ProfileModelFromMultiEventCreator();
    BufferStringSet		markersRemoved() const
				{ return markersremoved_; }

protected:

    float				getEventDepthVal(int evidx,
						      const ProfileBase&);
    float				getZOffset(int evidx,
						   const ProfileBase&);
    virtual bool			setIntersectMarkers();
    float				getIntersectMarkerDepth(
						int evidx,const ProfileBase&);
    bool				interpolateIntersectMarkers();
    void				checkAndRemoveMarkers();
    void				interpolateMarkersBetweenEvents();
    int					tiedToEventIdx( const char* mnm) const;
    void				removeMarkersFromProfiles(const char*);
    virtual bool			doGo(TaskRunner*);
    virtual void			fillCoords();
    virtual bool			isSingleEvent() const	{ return false;}
    int					getTopBottomEventMarkerIdx(
					     const ProfileBase&,int imarker,
					     bool findtop) const;
    bool				getTopBotEventValsWRTMarker(
					    const char* mnm,const ProfileBase&,
					    float& mdah,float& topevdah,
					    float& botevdah,float& relpos)const;

    ProfileModelFromEventData&		data_;
    BufferStringSet			markersremoved_;
};


#endif
