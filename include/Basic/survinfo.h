#ifndef survinfo_h
#define survinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		9-4-1996
 RCS:		$Id: survinfo.h,v 1.100 2011-08-25 06:30:31 cvskris Exp $
________________________________________________________________________

-*/
 
 
#include "namedobj.h"
#include "ranges.h"
#include "rcol2coord.h"
#include "enums.h"
class ascostream;
class IOPar;
class CubeSampling;
class LatLong2Coord;


/*!\brief Holds survey general information.

The surveyinfo is the primary source for ranges and steps. It also provides
the transformation between inline/xline <-> coordinates and lat/long estimates.

Note: the Z range step is only a default. It should not be used further
because different cubes/lines have different sample rates.

The ranges are defined for two cubes: the entire survey, and a 'working area'.
Normally, you'll want to have the working area.

If you are an expert, and you feel you need more 'power', you may want to look
at the bottom part of the class too for some more public functions.

*/

mClass SurveyInfo : public NamedObject
{

    mGlobal friend const SurveyInfo&	SI();

public:

			~SurveyInfo();
    bool		isValid() const		{ return valid_; }
    bool		has2D() const;
    bool		has3D() const;

    StepInterval<int>	inlRange(bool work) const;
    StepInterval<int>	crlRange(bool work) const;
    const StepInterval<float>& zRange( bool work ) const;
    int			inlStep() const;
    int			crlStep() const;
    float		zStep() const;
    float		inlDistance() const; //!< distance for one increment
    float		crlDistance() const;
    float		computeArea(const Interval<int>& inl,
	    		     const Interval<int>& crl) const;	//!<returns m2
    float		computeArea(bool work) const ;		//!<returns m2

    Coord3		oneStepTranslation(const Coord3& planenormal) const;

    const CubeSampling&	sampling( bool work ) const
    			{ return work ? wcs_ : cs_; }

    Coord		transform( const BinID& b ) const
			{ return b2c_.transform(b); }
    BinID		transform(const Coord&) const;
    			/*!<\note BinID will be snapped using work step. */

    enum Unit		{ Second, Meter, Feet };
    Unit		xyUnit() const;
    Unit		zUnit() const;
    inline bool		xyInFeet() const	{ return xyinfeet_;}
    inline bool		zIsTime() const		{ return zistime_; }
    inline bool		zInMeter() const	{ return !zistime_ &&!zinfeet_;}
    inline bool		zInFeet() const		{ return !zistime_ && zinfeet_;}
    const char*		getXYUnitString(bool withparens=true) const;
    const char*		getZUnitString(bool withparens=true) const;
    float		zFactor() const;
    			//!< Factor between real and displayed unit in UI
    static float	zFactor(bool time);
    			//!< Factor between real and displayed unit in UI
    bool		depthsInFeetByDefault() const;

    Coord		minCoord(bool work) const;
    Coord		maxCoord(bool work) const;
    bool		isInside(const BinID&,bool work) const;
    bool		isReasonable(const BinID&) const;
				//!< Checks if in or near survey
    bool		isReasonable(const Coord&) const;
				//!< Checks if in or near survey
    Interval<int>	reasonableRange(bool inl) const;
    int			maxNrTraces(bool work) const;

    void		checkInlRange(Interval<int>&,bool work) const;
			//!< Makes sure range is inside
    void		checkCrlRange(Interval<int>&,bool work) const;
			//!< Makes sure range is inside
    void		checkZRange(Interval<float>&,bool work) const;
			//!< Makes sure range is inside
    bool		includes(const BinID&,const float,bool work) const;
			//!< Returns true when pos is inside survey-range

    void		snap(BinID&,BinID direction=BinID(0,0)) const;
			//!< dir = 0 : auto; -1 round downward, 1 round upward
    void		snapStep(BinID&,BinID direction=BinID(0,0)) const;
    			//!< see snap() for direction
    void		snapZ(float&,int direction=0) const;
    			//!< see snap() for direction

    const IOPar&	pars() const			{ return pars_; }
    void		putZDomain(IOPar&) const;

    // Some public fns moved to bottom because they are rarely used; some fns
    // that have 'no user servicable parts inside' are at the very bottom

    enum Pol2D      	{ No2D=0, Both2DAnd3D=1, Only2D=2 };

protected:

			SurveyInfo();
    bool		valid_;

    BufferString	datadir_;
    BufferString	dirname_;

    bool		zistime_;
    bool		zinfeet_; //!< only relevant if zistime_ equals false
    bool		xyinfeet_; //!< only relevant for a few cases
    BufferString	comment_;
    BufferString	wsprojnm_;
    BufferString	wspwd_;
    CubeSampling&	cs_;
    CubeSampling&	wcs_;
    IOPar&		pars_;

    RCol2Coord		b2c_;
    LatLong2Coord&	ll2c_;
    BinID		set3binids_[3];
    Coord		set3coords_[3];

    Pol2D		survdatatype_;
    bool		survdatatypeknown_;

    static SurveyInfo*	theinst_;
 
    static void		deleteInstance();
    void		handleLineRead(const BufferString&,const char*);
    bool		wrapUpRead();
    void		writeSpecLines(ascostream&) const;

    void		setTr(RCol2Coord::RCTransform&,const char*);
    void		putTr(const RCol2Coord::RCTransform&,
	    			ascostream&,const char*) const;

private:

    // ugly, but hard to avoid:
    friend class		IOMan;
    friend class		uiSurvey;
    friend class		uiSurveyInfoEditor;

    RCol2Coord::RCTransform	rdxtr_;
    RCol2Coord::RCTransform	rdytr_;

public:

	// These fns are rarely used by non-specialist classes.

    void		setWorkRange(const CubeSampling&);
    Notifier<SurveyInfo> workRangeChg;

    const RCol2Coord&	binID2Coord() const	{ return b2c_; }
    void		get3Pts(Coord c[3],BinID b[2],int& xline) const;
    const LatLong2Coord& latlong2Coord() const	{ return ll2c_; }
    bool		isClockWise() const;
    			/*!< Orientation is determined by rotating the
			     inline axis to the crossline axis. */
    float		computeAngleXInl() const;
    			/*!< It's the angle (0 to pi/2) between
			     the X-axis and the Inl-axis (not an inline) */
    void		setXYInFeet( bool yn=true ) { xyinfeet_ = yn; }
    void		setZUnit(bool istime,bool infeet=false);
    static float	defaultXYtoZScale(Unit,Unit);
    			/*!<Gives a ballpark figure of how to scale XY to
			    make it comparable to Z. */
    float		zScale() const;
    			/*!<Gives a ballpark figure of how to scale Z to        
			    make it comparable to XY. */

    static const char*	sKeyInlRange();
    static const char*	sKeyCrlRange();
    static const char*	sKeyXRange();
    static const char*	sKeyYRange();
    static const char*	sKeyZRange();
    static const char*	sKeyWSProjName();
    static const char*	sKeyXYInFt();
    static const char*	sKeyDpthInFt(); //!< Not used by SI, just a UI default
    static const char*	sKeySurvDataType();

    BufferString	getDirName() const	{ return dirname_; }

    			DeclareEnumUtils(Pol2D);
    Pol2D		survDataType() const	{ return survdatatype_; }
    void		setSurvDataType( Pol2D typ )	{ survdatatype_ = typ; }

    const char*		comment() const		{ return comment_.buf(); }

    const char*		getWSProjName() const	{ return wsprojnm_.buf(); }
    			// Password only in memory this session
    const char*		getWSPwd() const	{ return wspwd_.buf(); }

	// These fns are used by specialist classes. Know what you are doing!

    			SurveyInfo(const SurveyInfo&);
    SurveyInfo&		operator =(const SurveyInfo&);

    RCol2Coord&		getBinID2Coord() const
    			{ return const_cast<SurveyInfo*>(this)->b2c_; }
    LatLong2Coord&	getLatlong2Coord() const
    			{ return const_cast<SurveyInfo*>(this)->ll2c_; }
    IOPar&		getPars() const	
    			{ return const_cast<SurveyInfo*>(this)->pars_; }

    bool		write(const char* basedir=0) const;
    			//!< Write to .survey file
    void		savePars(const char* basedir=0) const;
    			//!< Write to .defs file
    static SurveyInfo*	read(const char*);
    void		setRange(const CubeSampling&,bool);
    const char*		set3Pts(const Coord c[3],const BinID b[2],int xline);
    void		gen3Pts();
    void		setComment( const char* s )	{ comment_ = s; }
    void		setInvalid() const;

    void		setWSProjName( const char* nm ) const
			{ const_cast<SurveyInfo*>(this)->wsprojnm_ = nm; }
    void		setWSPwd( const char* nm ) const
			{ const_cast<SurveyInfo*>(this)->wspwd_ = nm; }

};


mGlobal const SurveyInfo& SI();


#endif
