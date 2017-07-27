#ifndef profilebase_h
#define profilebase_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Decemeber 2016
________________________________________________________________________


-*/

#include "wellmod.h"
#include "wellmarker.h"
#include "multiid.h"
#include "coord.h"
#include "bufstringset.h"
#include "zaxistransform.h"

class ProfilePosProvider;

mExpClass(Well) ProfileBase
{
public:

    virtual		~ProfileBase()				{}
    bool		isWell() const	{ return !wellid_.isUdf(); }
    virtual bool	isCtrl() const				= 0;
    virtual bool	isPrimary() const			= 0;
    virtual const char* typeStr() const				= 0;
    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);
    bool		moveMarker(int,float newz,bool pushothers=false,
				   bool pullothers=false);
    void		createMarkersFrom( const ProfileBase* p0,
					   const ProfileBase* p1);
    void		positionMarkersNotInFar(
				const Well::MarkerSet&,
				const ObjectSet<const Well::Marker>&,float);
    float		pos_;
    Well::MarkerSet	markers_;
    MultiID		wellid_;
    Coord		coord_;
    BufferString	name() const;
protected:
			ProfileBase(float pos)
			    : pos_(pos), wellid_(MultiID::udf())	{}

    void		copyFrom(const ProfileBase&);
    void		shiftMarkerZVals(Interval<int>,float);
    void		ensureNoMarkerZCrossings(Interval<int>,bool,float);
};


mExpClass(Well) ProfileModelBase :  public CallBacker
{
public:
			~ProfileModelBase();
    int			size() const	{ return profs_.size();}
    bool		isEmpty() const { return profs_.isEmpty();}
    int			nrWells(bool unique=false) const;


    const ProfileBase*	get(int idx) const
			{ return profs_.validIdx(idx) ? profs_[idx] : 0; }
    ProfileBase*	get(int idx)
			{ return profs_.validIdx(idx) ? profs_[idx] : 0; }
    int			set(ProfileBase*,bool replace_if_at_same_pos);
				//!< new or existing profiles (after pos_ chg)
				//!< will reposition profile if necessary
				//!< returns -1 only if you pass null
    inline int		add( ProfileBase* p )	{ return set( p, true ); }
    int			nearestIndex(float pos) const;
				//!< returns -1 only if empty
    Interval<int>	getIndexes(float pos,bool noinvalid=false) const;
				//!< -1 ->  none before, size() -> none after
				//!< if start == stop then exact match
    int			indexOf(const MultiID&) const;
    int			indexOf(const ProfileBase&) const;
    int			ctrlIndexBefore(float pos) const;
    int			ctrlIndexAfter(float pos) const;
    int			wellIndexBefore(float pos) const;
    int			wellIndexAfter(float pos) const;
    int			primaryIndexBefore(float pos) const;
    int			primaryIndexAfter(float pos) const;
    int			idxBefore(float pos,bool& isatequalpos) const;
    float		getMaxZ() const;

    void		removeAll();
    void		removeCtrlProfiles();
    void		removeProfiles(const char* typestr=0);
    void		removeAtSamePos(int idxtokeep=-1);
    void		removeMarker( const char* mrkrnm);
    void		setZTransform( const ZAxisTransform* trans )
			{ ztransform_ = trans; }
    const ZAxisTransform* zTransfrom() const	{ return ztransform_; }
    float		getDepthVal(float timeval,float profpos,
				    const ProfilePosProvider*,
				    bool depthintvdss=true) const;

    ObjectSet<ProfileBase> profs_;
protected:
    float		getDepthValBetweenWellPos(float t,float profpos,
						bool depthintvdss) const;
    float		getDepthValAtWellPos(float t,const ProfileBase&,
				    bool depthintvdss=true) const;
    ConstRefMan<ZAxisTransform> ztransform_;
};

mExpClass(Well) ProfileFactory
{
public:
    typedef ProfileBase*	(*CreateFunc)(float pos);

    void			addCreateFunc(CreateFunc,const char*);
    ProfileBase*		create(float,const char* typestr=0);
    ProfileBase*		create(const char* typestr=0);
protected:
    TypeSet<CreateFunc>		createfuncs_;
    BufferStringSet		keys_;
};

mGlobal(Well) ProfileFactory&	ProfFac();
#endif
