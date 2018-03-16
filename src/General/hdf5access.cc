/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : March 2018
-*/

#include "hdf5arraynd.h"
#include "arrayndimpl.h"
#include "iopar.h"
#include "uistrings.h"

mImplClassFactory( HDF5::AccessProvider, factory );

#define mRetNoFileInUiRv() mRetInternalInUiRv( uirv, sOpenFileFirst() )
#define mRetNoDataInUiRv() mRetInternalInUiRv( uirv, sNoDataPassed() )
#define mRetDataSpaceBad() { uirv.add( sBadDataSpace() ); return uirv; }

const char* HDF5::Access::sOpenFileFirst()
{ return "HDF5: Access not open. Use/check open()"; }
const char* HDF5::Access::sNeedScope()
{ return "HDF5: No valid scope set for data retrieval. Use/check setScope()"; }
const char* HDF5::Access::sNoDataPassed()
{ return "HDF5: Null data passed"; }


HDF5::AccessProvider* HDF5::AccessProvider::mkProv( int idx )
{
    if ( idx<0 || idx>=factory().size() )
	idx = factory().size()-1;
    if ( idx<0 )
	return 0;

    return factory().create( factory().key(idx) );
}


HDF5::Reader* HDF5::AccessProvider::mkReader( int idx )
{
    AccessProvider* prov = mkProv( idx );
    Reader* rdr = 0;
    if ( prov )
    {
	rdr = prov->getReader();
	delete prov;
    }
    return rdr;
}


HDF5::Writer* HDF5::AccessProvider::mkWriter( int idx )
{
    AccessProvider* prov = mkProv( idx );
    Writer* wrr = 0;
    if ( prov )
    {
	wrr = prov->getWriter();
	delete prov;
    }
    return wrr;
}


HDF5::Access::Access()
    : file_(0)
{
}


HDF5::Access::~Access()
{
    // cannot do closeFile(), the reader or writer has already been destructed
}


uiRetVal HDF5::Access::open( const char* fnm )
{
    closeFile();

    uiRetVal uirv;
    openFile( fnm, uirv );

    return uirv;
}


uiString HDF5::Access::sHDF5PackageDispName()
{
    return tr("HDF5 File Access");
}


uiString HDF5::Access::sHDF5Err()
{
    return tr("HDF5 Error");
}


uiString HDF5::Access::sFileNotOpen()
{
    return tr("Could not open HDF5 file");
}


uiRetVal HDF5::Writer::putInfo( const DataSetKey& dsky, const IOPar& iop )
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    if ( !iop.isEmpty() )
	ptInfo( dsky, iop, uirv );
    return uirv;
}


uiRetVal HDF5::Writer::putData( const DataSetKey& dsky, const ArrayNDInfo& inf,
				const void* data, ODDataType dt )
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    if ( !data )
	mRetNoDataInUiRv()
    else if ( inf.totalSize() < 1 )
	{ pErrMsg("zero dims"); }

    ptData( dsky, inf, data, dt, uirv );
    return uirv;
}


uiRetVal HDF5::Reader::getInfo( IOPar& iop ) const
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()

    gtInfo( iop, uirv );
    return uirv;
}


uiRetVal HDF5::Reader::getAll( void* data ) const
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    if ( !data )
	mRetNoDataInUiRv()
    const auto nrdims = nrDims();
    if ( nrdims < 1 )
	mRetDataSpaceBad()

    gtAll( data, uirv );
    return uirv;
}


uiRetVal HDF5::Reader::getPoint( NDPos pos, void* data ) const
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    if ( !data )
	mRetNoDataInUiRv()
    if ( !pos )
	mRetInternalInUiRv( uirv, "No position provided" )
    const auto nrdims = nrDims();
    if ( nrdims < 1 )
	mRetDataSpaceBad()

    NDPosBufSet pts;
    pts += mNDPosBufFromPos( pos, nrdims );
    gtPoints( pts, data, uirv );
    return uirv;
}


uiRetVal HDF5::Reader::getPoints( const NDPosBufSet& pts, void* data ) const
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    if ( pts.isEmpty() )
	return uirv;
    if ( !data )
	mRetNoDataInUiRv()
    const auto nrdims = nrDims();
    if ( nrdims < 1 )
	mRetDataSpaceBad()

    gtPoints( pts, data, uirv );
    return uirv;
}


uiRetVal HDF5::Reader::getSlab( const SlabSpec& spec, void* data ) const
{
    uiRetVal uirv;
    if ( !file_ )
	mRetNoFileInUiRv()
    if ( !data )
	mRetNoDataInUiRv()
    const auto nrdims = nrDims();
    if ( nrdims < 1 )
	mRetDataSpaceBad()
    if ( spec.size() != nrdims )
    {
	if ( spec.size() < nrdims )
	    mRetInternalInUiRv( uirv, "Specify all dimensions in SlabSpec" );
	pErrMsg( "Probably wrong: SlabSpec too big" );
    }

    gtSlab( spec, data, uirv );
    return uirv;
}
