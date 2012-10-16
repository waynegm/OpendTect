#ifndef vismaterial_h
#define vismaterial_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "color.h"
#include "visnodestate.h"

namespace osg {
    class Material;
    class Array;
};

class IOPar;

namespace visBase
{

/*!\brief


*/

mClass(visBase) Material : public NodeState
{
public:
    			Material();

    Notifier<Material>	change;

    void		setFrom(const Material&);

    void		setColor(const Color&,int=0);
    const Color&	getColor(int matnr=0) const;

    void		setDiffIntensity(float,int=0);
			/*!< Should be between 0 and 1 */
    float		getDiffIntensity(int=0) const;

    void		setAmbience(float);
			/*!< Should be between 0 and 1 */
    float		getAmbience() const;

    void		setSpecIntensity(float);
			/*!< Should be between 0 and 1 */
    float		getSpecIntensity() const;

    void		setEmmIntensity(float);
			/*!< Should be between 0 and 1 */
    float		getEmmIntensity() const;

    void		setShininess(float);
			/*!< Should be between 0 and 1 */
    float		getShininess() const;

    void		setTransparency(float,int idx=0);
			/*!< Should be between 0 and 1 */
    float		getTransparency(int idx=0) const;

    int			usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    int			nrOfMaterial() const;
    
    const osg::Array*	getColorArray() const;

protected:
			~Material();
    void		setMinNrOfMaterials(int);
    void		updateMaterial(int);
    void		createArray();

    TypeSet<Color>	color_;
    TypeSet<float>	diffuseintencity_;
    TypeSet<float>	transparency_;
    
    float		ambience_;
    float		specularintensity_;
    float		emmissiveintensity_;
    float		shininess_;

    osg::Material*	material_;
    osg::Array*		colorarray_;

    static const char*	sKeyColor();
    static const char*	sKeyAmbience();
    static const char*	sKeyDiffIntensity();
    static const char*	sKeySpectralIntensity();
    static const char*	sKeyEmmissiveIntensity();
    static const char*	sKeyShininess();
    static const char*	sKeyTransparency();
};

} // namespace visBase


#endif

