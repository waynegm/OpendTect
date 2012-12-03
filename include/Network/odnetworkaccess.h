#ifndef odnetworkaccess_h
#define odnetworkaccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Oct 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "executor.h"
#include "networkmod.h"
#include "strmprov.h"

class QEventLoop;
class QNetworkAccessManager;
class ODNetworkReply;

/*!< Use these functions to download one or more files using HTTP protocol*/
mGlobal(Network) bool	    getFile(const char* url,const char* outpath,
				    BufferString& errmsg,TaskRunner* tr);

mGlobal(Network) bool	    getFiles(BufferStringSet& urls,const char* outpath,
				     BufferString& errmsg,TaskRunner* tr);

mGlobal(Network) void	    setHttpProxy(const char* hostname,int port,bool 
					 auth=false,const char* username=0,
					 const char* password=0);

//!>Provides file download facility
mClass(Network) FileDownloader : public SequentialTask
{
public:
			    FileDownloader(const char* url,const char* path);
			    FileDownloader(const BufferStringSet&,const char*);
			    ~FileDownloader();

    const char*		    message() const;
    int			    nextStep();
    od_int64		    nrDone() const;
    const char*		    nrDoneText() const;
    od_int64		    totalNr() const;

protected:

    bool		    errorOccured();
    void		    getFileSize();
    bool		    readData();

    od_int64		    nrdone_;
    od_int64		    totalnr_;
    BufferString	    msg_;

    BufferStringSet	    files_;
    BufferStringSet	    outputpath_;
    int			    nrfilesdownloaded_;

    QEventLoop*		    qeventloop_;
    ODNetworkReply*	    odnr_;

    bool		    init_;
    StreamData		    osd_;
};

mGlobal(Network) QNetworkAccessManager&  ODNA();

#endif
