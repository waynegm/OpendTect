#ifndef vistexture_h
#define vistexture_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture.h,v 1.4 2003-01-27 13:13:12 kristofer Exp $
________________________________________________________________________


-*/

#include "vissceneobj.h"

class DataClipper;
class BasicTask;
class visBaseTextureColorIndexMaker;

namespace visBase
{

class VisColorTab;
class ThreadWorker;

/*!\brief
is a base class for Texture2 and Texture3 and should not be used directly.

A number of caches are implemented to minimize the calculation-time
when the colortable is updated.

If ThreadWorker is set, it utilizes mt processing.

*/

class Texture : public SceneObject
{
public:
    			
    void		setAutoScale( bool yn );
    bool		autoScale() const;

    void		setColorTab( VisColorTab& );
    VisColorTab&	getColorTab();

    void		setClipRate( float );
    			/*!< Relayed to colortable */
    float		clipRate() const;
    			/*!< Relayed to colortable */

    const TypeSet<float>& getHistogram() const;

    void		setUseTransperancy(bool yn);
    bool		usesTransperancy() const;

    void		setThreadWorker( ThreadWorker* );
    ThreadWorker*	getThreadWorker();


protected:
    			Texture();
    			~Texture();
    void		setResizedData( float*, int sz );
    			/*!< Is taken over by me */

    virtual unsigned char* getTexturePtr()				= 0;
    virtual void	finishEditing()					= 0;

private:
    void		colorTabChCB( CallBacker* );
    void		colorSeqChCB( CallBacker* );
    void		autoscaleChCB( CallBacker* );

    void		clipData();
    void		makeColorIndexes();
    void		makeTexture();
    void		makeColorTables();

    float*		datacache;
    unsigned char*	indexcache;
    int			cachesize;

    unsigned char*	red;
    unsigned char*	green;
    unsigned char*	blue;
    unsigned char*	trans;

    TypeSet<float>	histogram;

    bool		usetrans;

    VisColorTab*	colortab;
    ObjectSet<visBaseTextureColorIndexMaker> colorindexers;
    ObjectSet<BasicTask> texturemakers;
    ThreadWorker*	threadworker;
};

};

#endif
