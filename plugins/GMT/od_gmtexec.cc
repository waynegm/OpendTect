/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman K Singh
 Date:		July 2008
________________________________________________________________________

-*/

#include "batchprog.h"
#include "file.h"
#include "filepath.h"
#include "gmtpar.h"
#include "initgmtplugin.h"
#include "keystrs.h"
#include "oddirs.h"
#include "strmprov.h"
#include "moddepmgr.h"

#include <iostream>

#define mErrFatalRet(msg) \
{ \
    strm << msg << od_newline; \
    od_ostream tmpstrm( tmpfp.fullPath() ); \
    outputfp.setFileName( ".gmtcommands4" ); \
    if ( File::exists(outputfp.fullPath()) ) \
	StreamProvider( outputfp.fullPath() ).remove(); \
	\
    File::changeDir( cwd.buf() ); \
    tmpstrm << "Failed" << od_newline; \
    tmpstrm << "Failed to create map"; \
    return false; \
}


bool BatchProgram::go( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllNonUI() );
    GMT::initStdClasses();
    const char* psfilenm = pars().find( sKey::FileName() );
    File::Path outputfp( psfilenm );
    const BufferString cwd = File::getCurrentPath();
    if ( cwd.size() > 255 )
	mErrStrmRet("Error: Current working directory path length too big")

    File::changeDir( outputfp.pathOnly() );
    if ( !psfilenm || !*psfilenm )
	mErrStrmRet("Output PS file missing")

    File::Path tmpfp( psfilenm );
    tmpfp.setExtension( "tmp" );
    IOPar legendspar;
    int legendidx = 0;
    legendspar.set( ODGMT::sKeyGroupName(), "Legend" );
    for ( int idx=0; ; idx++ )
    {
	IOPar* iop = pars().subselect( idx );
	if ( !iop ) break;

	PtrMan<GMTPar> par = GMTPF().create( *iop );
	if ( idx == 0 )
	{
	    FixedString bmres( par ? par->find(ODGMT::sKeyGroupName()) : 0 );
	    if ( bmres.isEmpty() )
		mErrFatalRet("Basemap parameters missing")
	}

	if ( !par->execute(strm,psfilenm) )
	{
	    BufferString msg = "Failed to post ";
	    msg += iop->find( ODGMT::sKeyGroupName() );
	    strm << msg << od_newline;
	    if ( idx )
		continue;
	    else
		mErrFatalRet("Please check your GMT installation")
	}

	IOPar legpar;
	if ( par->fillLegendPar( legpar ) )
	{
	    legendspar.mergeComp( legpar, toString(legendidx++) );
	}

	if ( idx == 0 )
	{
	    Interval<int> xrg, yrg;
	    Interval<float> mapdim;
	    par->get( ODGMT::sKeyXRange(), xrg );
	    par->get( ODGMT::sKeyYRange(), yrg );
	    par->get( ODGMT::sKeyMapDim(), mapdim );
	    legendspar.set( ODGMT::sKeyMapDim(), mapdim );
	    legendspar.set( ODGMT::sKeyXRange(), xrg );
	    legendspar.set( ODGMT::sKeyYRange(), yrg );
	}
    }

    PtrMan<GMTPar> par = GMTPF().create( legendspar );
    if ( !par || !par->execute(strm, psfilenm) )
	strm << "Failed to post legends";

    strm << "Map created successfully" << od_endl;

    outputfp.setFileName( GMT::hasModernGMT() ? "gmt.history"
					      : ".gmtcommands4" );
    StreamProvider( outputfp.fullPath() ).remove();
    File::changeDir( cwd.buf() );
    StreamData sd = StreamProvider( tmpfp.fullPath() ).makeOStream();
    *sd.oStrm() << "Finished.\n";
    sd.close();
    return true;
}
