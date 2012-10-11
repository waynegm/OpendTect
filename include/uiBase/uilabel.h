#ifndef uilabel_h
#define uilabel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          7/9/2000
 RCS:           $Id$
________________________________________________________________________

-*/


#include "uibaseobject.h"
#include "draw.h"

class uiGroup;
class uiLabelBody;
class ioPixmap;

mFDQtclass(QLabel);

mClass(uiBase) uiLabel : public uiBaseObject
{
public:
                        uiLabel(const char*,uiBaseObject* = 0);

/*! \brief set text on label

    Note that the layout for the label is not updated when setting a new text.
    So, if the new text is too long, part of it might be invisible.
    Therefore, reserve enough space for it with setPrefWidthInChar.

*/
    virtual void        setText(const char*);
    const char*         text() const;
    void		setTextSelectable(bool yn=true);
    void		setPixmap(const ioPixmap&);

/*! 
    setting an alignment only makes sense if you reserve space using
    setPrefWidthInChar();
*/
    void		setAlignment(Alignment::HPos);
    Alignment::HPos	alignment() const;

private:
    virtual mQtclass(QWidget)*	getWidget(int,int);

       mQtclass(QLabel)*	qlabel_;
};

#endif

