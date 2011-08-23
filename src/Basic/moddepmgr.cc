/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: moddepmgr.cc,v 1.1 2011-08-23 11:42:33 cvsbert Exp $";


#include "moddepmgr.h"
#include "debugmasks.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "strmprov.h"
#include "envvars.h"
#include "sharedlibs.h"
#include <iostream>


const OD::ModDepMgr& OD::ModDeps()
{
    static ModDepMgr* mgr = 0;
    if ( !mgr )
	mgr = new ModDepMgr;
    return *mgr;
}


OD::ModDepMgr::ModDepMgr( const char* mdfnm )
    : isrel_(true)
{
    if ( !mdfnm || !*mdfnm )
	mdfnm = "ModDeps.od";
    FilePath relfp( GetSoftwareDir(0) );
    relfp.add( "data" ).add( mdfnm );
    BufferString fnm( relfp.fullPath() );
    FilePath devfp( GetEnvVar("WORK") );
    devfp.add( "Pmake" ).add( mdfnm );
    if ( !File::exists(fnm) )
    {
	isrel_ = false;
	BufferString devfnm = devfp.fullPath();
	if ( !File::exists(devfnm) )
	{
	    if ( DBG::isOn(DBG_PROGSTART) )
	    {
		BufferString msg( "Ouch. No ",mdfnm," found.\nTried " );
		msg.add( fnm ). add( " and " ).add( devfnm ).add( "." );
		DBG::message( msg );
	    }
	    return;
	}
	fnm = devfnm;
    }

    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
    {
	if ( DBG::isOn(DBG_PROGSTART) )
	{
	    BufferString msg( "Ouch. Cannot read ",fnm,"." );
	    msg.add( fnm ). add( " and " ).add( fnm ).add( "." );
	    DBG::message( msg );
	}
	return;
    }

    if ( DBG::isOn(DBG_PROGSTART) )
	DBG::message( BufferString("Start reading ",fnm,".") );
    readDeps( *sd.istrm );
    if ( DBG::isOn(DBG_PROGSTART) )
	DBG::message( BufferString("Read ",fnm,".") );
    sd.close();

    relfp.set( GetBinPlfDir() );
#ifndef __win__
    relfp.add( "so" );
#endif
    relbindir_ = relfp.fullPath();
    devfp.setFileName( 0 );
    devfp.setFileName( "lib" ); devfp.add( GetPlfSubDir() );
    devfp.add( "G" );
    devbindir_ = devfp.fullPath();
    if ( !File::exists(devbindir_) )
    {
	devfp.setFileName( "OG" );
	devbindir_ = devfp.fullPath();
    }
}


void OD::ModDepMgr::readDeps( std::istream& strm )
{
    char linebuf[1024]; char wordbuf[256];

    while ( strm )
    {
	strm.getline( linebuf, 1024 );
	char* bufptr = linebuf; 
	mTrimBlanks(bufptr);
	if ( ! *bufptr || *bufptr == '#' )
	    continue;

	char* nextptr = (char*)getNextWord(bufptr,wordbuf);
	if ( ! wordbuf[0] ) continue;
	od_int64 l = strlen( wordbuf );
	if ( wordbuf[l-1] == ':' ) wordbuf[l-1] = '\0';
	if ( ! wordbuf[0] ) continue;

	*nextptr++ = '\0';
	mSkipBlanks(nextptr);

	ModDep* newdep = new ModDep( wordbuf ) ;
	BufferStringSet filedeps;
	while ( nextptr && *nextptr )
	{
	    mSkipBlanks(nextptr);
	    nextptr = (char*)getNextWord(nextptr,wordbuf);
	    if ( !wordbuf[0] ) break;

	    if ( wordbuf[1] != '.' || (wordbuf[0] != 'S' && wordbuf[0] != 'D') )
	    {
		if ( DBG::isOn(DBG_PROGSTART) )
		    DBG::message( BufferString("Found bad dep: ",wordbuf,".") );
		continue;
	    }

	    filedeps.add( wordbuf );
	}


	BufferStringSet depmods;
	for ( int idx=filedeps.size()-1; idx>=0; idx-- )
	{
	    const char* filedep = filedeps.get(idx).buf();
	    const char* modnm = filedep + 2;
	    if ( *filedep == 'S' )
	    {
		depmods.add( modnm );
	        continue;
	    }

	    const ModDep* depdep = find( modnm );
	    if ( !depdep )
	    {
		if ( DBG::isOn(DBG_PROGSTART) )
		    DBG::message( BufferString("Cannot find dep: ",modnm,".") );
		continue;
	    }

	    for ( int idep=depdep->mods_.size()-1; idep>=0; idep-- )
	    {
		const char* depdepmod = depdep->mods_.get(idep).buf();
		if ( depmods.indexOf(depdepmod) < 0 )
		    depmods.add( depdepmod );
	    }
	}
	if ( depmods.size() < 1 )
	    { delete newdep; continue; }
	deps_ += newdep;

	for ( int idx=depmods.size()-1; idx>=0; idx-- )
	    newdep->mods_ += depmods[idx];
    }
}


const OD::ModDep* OD::ModDepMgr::find( const char* nm ) const
{
    return ::find( deps_, nm );
}


void OD::ModDepMgr::ensureLoaded( const char* nm ) const
{
    const OD::ModDep* md = find( nm );
    if ( !md ) return;

    for ( int idep=md->mods_.size()-1; idep>=0; idep-- )
    {
	const BufferString& modnm( md->mods_.get(idep) );
	const int loadedidx = getLoadIdx( modnm );
	if ( loadedidx >= 0 )
	    continue;

	char libnm[256];
	SharedLibAccess::getLibName( md->mods_.get(idep), libnm );
	FilePath fp( isrel_ ? relbindir_ : devbindir_ );
	fp.add( libnm );
	SharedLibAccess* sla = new SharedLibAccess( fp.fullPath() );
	if ( !sla->isOK() )
	    { delete sla; continue; }

	loadedmods_.add( modnm );
	shlibaccs_ += sla;
    }

    ensureInited( nm );
}


void OD::ModDepMgr::ensureInited( const char* nm ) const
{
    const OD::ModDep* md = find( nm );
    if ( !md ) return;

    for ( int idep=md->mods_.size()-1; idep>=0; idep-- )
    {
	const BufferString& modnm( md->mods_.get(idep) );
	const int loadedidx = getLoadIdx( modnm );
	if ( loadedidx < 0 )
	    continue;

	const SharedLibAccess& sla = *shlibaccs_[loadedidx];
	BufferString fnnm( "od_" );
	fnnm.add( nm ).add( "_initStdClasses" );
	typedef void (*ModuleInitFn)(void);
	ModuleInitFn fn = (ModuleInitFn)sla.getFunction( fnnm );
	if ( fn )
	    (*fn)();
    }
}


int OD::ModDepMgr::getLoadIdx( const char* nm ) const
{
    return loadedmods_.indexOf( nm );
}
