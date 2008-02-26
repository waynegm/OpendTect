/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellwriter.cc,v 1.9 2008-02-26 09:17:36 cvsnanne Exp $";

#include "wellwriter.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "ascstream.h"
#include "errh.h"
#include "strmprov.h"
#include "keystrs.h"
#include "separstr.h"
#include "iopar.h"
#include <iostream>


Well::Writer::Writer( const char* f, const Well::Data& w )
	: Well::IO(f,false)
    	, wd(w)
{
}


bool Well::Writer::wrHdr( std::ostream& strm, const char* fileky ) const
{
    ascostream astrm( strm );
    if ( !astrm.putHeader(fileky) )
    {
	BufferString msg( "Cannot write to " );
	msg += fileky;
	msg += " file";
	ErrMsg( msg );
	return false;
    }
    return true;
}


bool Well::Writer::put() const
{
    return putInfo()
	&& putLogs()
	&& putMarkers()
	&& putD2T();
}


bool Well::Writer::putInfo() const
{
    StreamData sd = mkSD( sExtWell );
    if ( !sd.usable() ) return false;

    const bool isok = putInfo( *sd.ostrm );
    sd.close();
    return isok;
}


bool Well::Writer::putInfo( std::ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyWell) ) return false;

    ascostream astrm( strm );
    astrm.put( Well::Info::sKeyuwid, wd.info().uwid );
    astrm.put( Well::Info::sKeyoper, wd.info().oper );
    astrm.put( Well::Info::sKeystate, wd.info().state );
    astrm.put( Well::Info::sKeycounty, wd.info().county );
    char str[80]; wd.info().surfacecoord.fill( str );
    astrm.put( Well::Info::sKeycoord, str );
    astrm.put( Well::Info::sKeyelev, wd.info().surfaceelev );
    astrm.newParagraph();

    return putTrack( strm );
}


bool Well::Writer::putTrack( std::ostream& strm ) const
{
    for ( int idx=0; idx<wd.track().size(); idx++ )
    {
	const Coord3& c = wd.track().pos(idx);
	BufferString bs( c.x ); strm << bs << '\t';
	bs = c.y; strm << bs << '\t';
	bs = c.z; strm << bs << '\t';
	strm << wd.track().dah(idx) << '\n';
    }
    return strm.good();
}


bool Well::Writer::putLogs() const
{
    for ( int idx=0; idx<wd.logs().size(); idx++ )
    {
	StreamData sd = mkSD( sExtLog, idx+1 );
	if ( !sd.usable() ) break;

	const Well::Log& wl = wd.logs().getLog(idx);
	if ( !putLog(*sd.ostrm,wl) )
	{
	    BufferString msg( "Could not write log: '" );
	    msg += wl.name();
	    msg += "'";
	    ErrMsg( msg );
	    sd.close();
	    return false;
	}
	sd.close();
    }

    return true;
}


bool Well::Writer::putLog( std::ostream& strm, const Well::Log& wl ) const
{
    if ( !wrHdr(strm,sKeyLog) ) return false;

    ascostream astrm( strm );
    astrm.put( sKey::Name, wl.name() );
    if ( *wl.unitMeasLabel() )
	astrm.put( Well::Log::sKeyUnitLbl, wl.unitMeasLabel() );
    astrm.newParagraph();

    Interval<int> wrintv( 0, wl.size()-1 );
    float dah, val;
    for ( ; wrintv.start<wl.size(); wrintv.start++ )
    {
	dah = wl.dah(wrintv.start); val = wl.value(wrintv.start);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }
    for ( ; wrintv.stop>=0; wrintv.stop-- )
    {
	dah = wl.dah(wrintv.stop); val = wl.value(wrintv.stop);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }

    for ( int idx=wrintv.start; idx<=wrintv.stop; idx++ )
    {
	dah = wl.dah(idx); val = wl.value(idx);
	if ( mIsUdf(dah) )
	    continue;
	if ( mIsUdf(val) )
	    strm << dah << '\t' << sKey::FloatUdf << '\n';
	else
	    strm << dah << '\t' << val << '\n';
    }

    return strm.good();
}


bool Well::Writer::putMarkers() const
{
    StreamData sd = mkSD( sExtMarkers );
    if ( !sd.usable() ) return false;

    const bool isok = putMarkers( *sd.ostrm );
    sd.close();
    return isok;
}


bool Well::Writer::putMarkers( std::ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyMarkers) ) return false;

    ascostream astrm( strm );
    for ( int idx=0; idx<wd.markers().size(); idx++ )
    {
	BufferString basekey; basekey += idx+1;
	const Well::Marker& wm = *wd.markers()[idx];

	BufferString key = IOPar::compKey( basekey, sKey::Name );
	astrm.put( key, wm.name() );
	key = IOPar::compKey( basekey, sKey::Desc );
	FileMultiString fms;
	fms += wm.istop_ ? "T" : "B"; fms += wm.desc_;
	astrm.put( key, fms );
	key = IOPar::compKey( basekey, sKey::Color );
	BufferString bs; wm.color_.fill( bs.buf() );
	astrm.put( key, bs );
	key = IOPar::compKey( basekey, Well::Marker::sKeyDah );
	astrm.put( key, wm.dah_ );
	key = IOPar::compKey( basekey, sKey::StratRef );
	astrm.put( key, wm.stratlevelid_ );
    }

    return strm.good();
}


bool Well::Writer::putD2T() const
{
    if ( !wd.d2TModel() ) return true;

    StreamData sd = mkSD( sExtD2T );
    if ( !sd.usable() ) return false;

    const bool isok = putD2T( *sd.ostrm );
    sd.close();
    return isok;
}


bool Well::Writer::putD2T( std::ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyD2T) ) return false;

    ascostream astrm( strm );
    const Well::D2TModel& d2t = *wd.d2TModel();
    astrm.put( sKey::Name, d2t.name() );
    astrm.put( sKey::Desc, d2t.desc );
    astrm.put( D2TModel::sKeyDataSrc, d2t.datasource );
    astrm.newParagraph();

    for ( int idx=0; idx<d2t.size(); idx++ )
	strm << d2t.dah(idx) << '\t' << d2t.t(idx) << '\n';
    return strm.good();
}
