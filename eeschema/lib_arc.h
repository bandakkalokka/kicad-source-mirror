/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file lib_arc.h
 */

#ifndef _LIB_ARC_H_
#define _LIB_ARC_H_

#include <lib_draw_item.h>


class TRANSFORM;


class LIB_ARC : public LIB_ITEM
{
    enum SELECT_T               // When creating an arc: status of arc
    {
        ARC_STATUS_START,
        ARC_STATUS_END,
        ARC_STATUS_OUTLINE,
    };

    int      m_Radius;
    int      m_t1;              // First radius angle of the arc in 0.1 degrees.
    int      m_t2;              /* Second radius angle of the arc in 0.1 degrees. */
    wxPoint  m_ArcStart;
    wxPoint  m_ArcEnd;          /* Arc end position. */
    wxPoint  m_Pos;             /* Radius center point. */
    int      m_Width;           /* Line width */
    double   m_editCenterDistance;
    SELECT_T m_editSelectPoint;
    int      m_editState;
    int      m_editDirection;
    int      m_lastEditState;

    /**
     * Draws the arc.
     */
    void drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                      EDA_COLOR_T aColor, GR_DRAWMODE aDrawMode, void* aData,
                      const TRANSFORM& aTransform );

    /**
     * Draw the graphics when the arc is being edited.
     */
    void drawEditGraphics( EDA_RECT* aClipBox, wxDC* aDC, EDA_COLOR_T aColor );

    /**
     * Calculates the center, radius, and angles at \a aPosition when the arc is being edited.
     *
     * Note: The center may not necessarily be on the grid.
     *
     * @param aPosition - The current mouse position in drawing coordinates.
     */
    void calcEdit( const wxPoint& aPosition );

    /**
     * Calculate the radius and angle of an arc using the start, end, and center points.
     */
    void calcRadiusAngles();

public:
    LIB_ARC( LIB_COMPONENT * aParent );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~LIB_ARC() { }

    wxString GetClass() const
    {
        return wxT( "LIB_ARC" );
    }


    bool Save( OUTPUTFORMATTER& aFormatter );

    bool Load( LINE_READER& aLineReader, wxString& aErrorMsg );

    bool HitTest( const wxPoint& aPosition );

    bool HitTest( wxPoint aPosition, int aThreshold, const TRANSFORM& aTransform );

    EDA_RECT GetBoundingBox() const;

    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList );

    int GetPenSize() const;

    void BeginEdit( STATUS_FLAGS aEditMode, const wxPoint aStartPoint = wxPoint( 0, 0 ) );

    bool ContinueEdit( const wxPoint aNextPoint );

    void EndEdit( const wxPoint& aPosition, bool aAbort = false );

    void SetOffset( const wxPoint& aOffset );

    bool Inside( EDA_RECT& aRect ) const;

    void Move( const wxPoint& aPosition );

    wxPoint GetPosition() const { return m_Pos; }

    void MirrorHorizontal( const wxPoint& aCenter );

    void MirrorVertical( const wxPoint& aCenter );

    void Rotate( const wxPoint& aCenter, bool aRotateCCW = true );

    void Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
               const TRANSFORM& aTransform );

    int GetWidth() const { return m_Width; }

    void SetWidth( int aWidth ) { m_Width = aWidth; }

    wxString GetSelectMenuText() const;

    BITMAP_DEF GetMenuImage() const { return  add_arc_xpm; }

    EDA_ITEM* Clone() const;

private:

    /**
     * @copydoc LIB_ITEM::compare()
     *
     * The arc specific sort order is as follows:
     *      - Arc horizontal (X) position.
     *      - Arc vertical (Y) position.
     *      - Arc start angle.
     *      - Arc end angle.
     */
    int compare( const LIB_ITEM& aOther ) const;
};


#endif    // _LIB_ARC_H_
