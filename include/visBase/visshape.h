#ifndef visshape_h
#define visshape_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "indexedshape.h"

namespace osg { class Geometry; class Geode; class Switch; class PrimitiveSet; }

namespace visBase
{

class NodeState;
class ForegroundLifter;
class VisColorTab;
class Material;
class Texture2;
class Texture3;
class Coordinates;
class Normals;
class TextureCoords;


#undef mDeclSetGetItem
#define mDeclSetGetItem( ownclass, clssname, variable ) \
protected: \
    clssname*		   gt##clssname() const; \
public: \
    inline clssname*	   get##clssname()	 { return gt##clssname(); } \
    inline const clssname* get##clssname() const { return gt##clssname(); } \
    void		   set##clssname(clssname*)


mExpClass(visBase) Shape : public VisualObject
{
public:

   
    mDeclSetGetItem( Shape,	Texture2, texture2_ );
    mDeclSetGetItem( Shape,	Texture3, texture3_ );
    mDeclSetGetItem( Shape,	Material, material_ );

    void			setMaterialBinding( int );
    static int			cOverallMaterialBinding()	{ return 0; }
    static int			cPerVertexMaterialBinding()	{ return 2; }

    int				getMaterialBinding() const;

    int				usePar(const IOPar&);
    void			fillPar(IOPar&) const;
    void			setTwoSidedLight(bool);

    
protected:
				Shape();
    virtual			~Shape();
    
    Texture2*			texture2_;
    Texture3*			texture3_;
    Material*			material_;

    static const char*		sKeyOnOff();
    static const char*		sKeyTexture();
    static const char*		sKeyMaterial();
};


mExpClass(visBase) VertexShape : public Shape
{
public:
    static VertexShape*	create()
			mCreateDataObj(VertexShape);
    
    void		setPrimitiveType(Geometry::PrimitiveSet::PrimitiveType);
			//!<Should be called before adding statesets

    mDeclSetGetItem( VertexShape, Coordinates, coords_ );
    mDeclSetGetItem( VertexShape, Normals, normals_ );
    mDeclSetGetItem( VertexShape, TextureCoords, texturecoords_ );
    
    void		removeSwitch();
			/*!<Will turn the object permanently on.
			 \note Must be done before giving away the
			 SoNode with getInventorNode() to take
			 effect. */

    void		setDisplayTransformation( const mVisTrans* );
    			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates, 
			     you will have to setTransformation again.  */
    const mVisTrans*	getDisplayTransformation() const;
    			/*!<\note Direcly relayed to the coordinates */
    
    int			getNormalBindType();

    void		dirtyCoordinates();

    void		addPrimitiveSet(Geometry::PrimitiveSet*);
    void		removePrimitiveSet(const Geometry::PrimitiveSet*);
    void		removeAllPrimitiveSets();
    int			nrPrimitiveSets() const;
    virtual void	touchPrimitiveSet(int)			{}
    Geometry::PrimitiveSet*	getPrimitiveSet(int);
    void		setMaterial( Material* mt );
    void		materialChangeCB( CallBacker*  );
    
protected:
    			VertexShape( Geometry::PrimitiveSet::PrimitiveType,
				     bool creategeode );
    			~VertexShape();
    
    void		setupGeode();
    
    virtual void	addPrimitiveSetToScene(osg::PrimitiveSet*);
    virtual void	removePrimitiveSetFromScene(const osg::PrimitiveSet*);

    Normals*		normals_;
    Coordinates*	coords_;
    TextureCoords*	texturecoords_;

    osg::Node*		node_;
    
    osg::Geode*		geode_;
    osg::Geometry*	osggeom_;
    
    ObjectSet<Geometry::PrimitiveSet>		primitivesets_;
    Geometry::PrimitiveSet::PrimitiveType	primitivetype_;
};

#undef mDeclSetGetItem
    
    
mExpClass(visBase) IndexedShape : public VertexShape
{
public:
    
    int		nrCoordIndex() const;
    void	setCoordIndex(int pos,int idx);
    void	setCoordIndices(const int* idxs, int sz);
    		/*!<\note idxs are not copied, and caller must ensure
			  they remain in memory. */
    void	setCoordIndices(const int* idxs, int sz, int start);
    		/*!<\note idxs are copied */

    void	removeCoordIndexAfter(int);
    int		getCoordIndex(int) const;

    int		nrTextureCoordIndex() const;
    void	setTextureCoordIndex(int pos,int idx);
    void	setTextureCoordIndices(const int* idxs,int sz);
    		/*!<\note idxs are not copied, and caller must ensure
			  they remain in memory. */
    void	setTextureCoordIndices(const int* idxs, int sz, int start);
    		/*!<\note idxs are copied */
    void	removeTextureCoordIndexAfter(int);
    int		getTextureCoordIndex(int) const;

    int		nrNormalIndex() const;
    void	setNormalIndex(int pos,int idx);
    void	setNormalIndices(const int* idxs,int sz);
    		/*!<\note idxs are not copied, and caller must ensure
			  they remain in memory. */
    void	setNormalIndices(const int* idxs, int sz, int start);
    		/*!<\note idxs are copied */
    void	removeNormalIndexAfter(int);
    int		getNormalIndex(int) const;

    int		nrMaterialIndex() const;
    void	setMaterialIndex(int pos,int idx);
    void	setMaterialIndices(const int* idxs,int sz);
    		/*!<\note idxs are not copied, and caller must ensure
			  they remain in memory. */
    void	setMaterialIndices(const int* idxs, int sz, int start);
    		/*!<\note idxs are copied */
    void	removeMaterialIndexAfter(int);
    int		getMaterialIndex(int) const;

    int		getClosestCoordIndex(const EventInfo&) const;
    void	replaceShape(SoNode*);

protected:
		IndexedShape( Geometry::PrimitiveSet::PrimitiveType );
};
    
    
class PrimitiveSetCreator : public Geometry::PrimitiveSetCreator
{
    Geometry::PrimitiveSet* doCreate( bool, bool );
};
    


}

#endif

