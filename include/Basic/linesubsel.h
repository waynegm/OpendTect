#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2018
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bin2d.h"
#include "horsubsel.h"
#include "manobjectset.h"
class TrcKeyZSampling;


/*!\brief subselection of a 2D line geometry. */

mExpClass(Basic) LineSubSel : public ArrRegSubSel2D
			    , public Survey::GeomSubSel
{
public:

    mUseType( Pos,		IdxSubSel1D );
    mUseType( Pos,		IdxSubSelData );
    mUseType( Data,		idx_type );
    mUseType( Data,		size_type );
    mUseType( IdxSubSelData,	pos_type );
    mUseType( IdxSubSelData,	pos_steprg_type );
    mUseType( Survey,		HorSubSel );
    mUseType( Survey,		GeomSubSel );
    typedef IdxSubSel1D		TrcNrSubSel;
    typedef IdxSubSelData	TrcNrSubSelData;
    typedef pos_type		trcnr_type;

			LineSubSel(GeomID);
			LineSubSel(const Geometry2D&);
			LineSubSel(const pos_steprg_type&);
			LineSubSel(const pos_steprg_type&,const z_steprg_type&);
			LineSubSel(const pos_steprg_type&,const ZSubSel&);
			LineSubSel(const LineHorSubSel&);
			LineSubSel(GeomID,trcnr_type);
			LineSubSel(const Bin2D&);
			LineSubSel(const TrcKeySampling&);
			LineSubSel(const TrcKeyZSampling&);

    const TrcNrSubSelData& trcNrSubSel() const	{ return hss_.trcNrSubSel(); }
    TrcNrSubSelData& trcNrSubSel()		{ return hss_.trcNrSubSel(); }

    pos_steprg_type	trcNrRange() const	{ return hss_.trcNrRange(); }
    pos_steprg_type	fullTrcNrRange() const	{ return hss_.fullTrcNrRange();}
    idx_type		idx4TrcNr( trcnr_type trcnr ) const
			{ return hss_.idx4TrcNr( trcnr ); }
    trcnr_type		trcNr4Idx( idx_type idx ) const
			{ return hss_.trcNr4Idx( idx ); }
    size_type		nrTrcs() const		{ return hss_.nrTrcs(); }
    void		setTrcNrRange( const pos_steprg_type& rg )
			{ hss_.setTrcNrRange( rg ); }
    bool		isAll() const override
					{ return GeomSubSel::isAll(); }
    bool		includes( trcnr_type trcnr ) const
					{ return hss_.includes( trcnr ); }
    totalsz_type	totalSize() const override
					{ return GeomSubSel::totalSize(); }
    void		merge(const LineSubSel&);
    void		limitTo(const LineSubSel&);

    LineHorSubSel&	lineHorSubSel()		{ return hss_; }
    const LineHorSubSel& lineHorSubSel() const	{ return hss_; }

    const Geometry2D&	geometry2D() const	{ return hss_.geometry2D(); }

    static const LineSubSel&	empty();
    static LineSubSel&		dummy();

protected:

    LineHorSubSel hss_;

		mImplArrRegSubSelClone(LineSubSel)

    HorSubSel&	gtHorSubSel() const override
		{
		    return mSelf().hss_;
		}

    Data&	gtData( idx_type idim ) const override
		{
		    const Data* ret = &zSubSelData();
		    if ( idim < 1 )
			ret = &trcNrSubSel();
		    return const_cast<Data&>( *ret );
		}

};


mExpClass(Basic) LineSubSelSet : public ManagedObjectSet<LineSubSel>
{
public:

    mUseType( Survey::SubSel,	totalsz_type );
    mUseType( Pos,		GeomID );
    mUseType( LineSubSel,	trcnr_type );

			LineSubSelSet()		{}
			LineSubSelSet(const LineHorSubSelSet&);
    void		setToAll();

    bool		isAll() const;
    totalsz_type	totalSize() const;
    bool		hasAllLines() const;
    bool		hasFullRange() const;
    bool		hasFullZRange() const;
    void		merge(const LineSubSelSet&);
    void		limitTo(const LineSubSelSet&);
    void		addStepout(trcnr_type);

    bool		includes( GeomID gid ) const
			{ return doFind( gid ); }
    bool		includes( const Bin2D& b2d ) const
			{ return includes( b2d.geomID(), b2d.trcNr() ); }
    bool		includes( const GeomID& gid, trcnr_type trcnr ) const
			{
			    const auto* lss = doFind( gid );
			    return lss && lss->includes( trcnr );
			}

    LineSubSel*		find( GeomID gid )	      { return doFind( gid ); }
    const LineSubSel*	find( GeomID gid ) const      { return doFind( gid ); }

protected:

    LineSubSel*		doFind(GeomID) const;

};
