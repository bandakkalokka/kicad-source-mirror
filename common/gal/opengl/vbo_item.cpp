/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * @file vbo_item.cpp
 * @brief Class to handle an item held in a Vertex Buffer Object.
 */

#include <gal/opengl/vbo_container.h>
#include <gal/opengl/vbo_item.h>
#include <cstring>

using namespace KiGfx;

VBO_ITEM::VBO_ITEM( VBO_CONTAINER* aContainer ) :
        m_offset( 0 ),
        m_size( 0 ),
        m_container( aContainer ),
        m_isDirty( true ),
        m_transform( NULL )
{
    // By default no shader is used
    m_shader[0] = 0;

    // The item's size is not known yet, so we just start an item in the container
    aContainer->StartItem( this );
}


VBO_ITEM::~VBO_ITEM()
{
    m_container->Free( this );
}


void VBO_ITEM::PushVertex( VBO_VERTEX* aVertex )
{
    if( m_transform != NULL )
    {
        // Apply transformations
        glm::vec4 vertex( aVertex->x, aVertex->y, aVertex->z, 1.0f );
        vertex = *m_transform * vertex;

        // Replace only coordinates, leave color as it is
        aVertex->x = vertex.x;
        aVertex->y = vertex.y;
        aVertex->z = vertex.z;
    }

    // Apply currently used color
    aVertex->r = m_color[0];
    aVertex->g = m_color[1];
    aVertex->b = m_color[2];
    aVertex->a = m_color[3];

    // Apply currently used shader
    for( int i = 0; i < ShaderStride; ++i )
    {
        aVertex->shader[i] = m_shader[i];
    }

    m_container->Add( this, aVertex );

    m_size++;
    m_isDirty = true;
}


void VBO_ITEM::PushVertices( VBO_VERTEX* aVertices, GLuint aSize )
{
    for( unsigned int i = 0; i < aSize; ++i )
    {
        PushVertex( &aVertices[i] );
    }
}


VBO_VERTEX* VBO_ITEM::GetVertices()
{
    if( m_isDirty )
        Finish();

    return m_container->GetVertices( this );
}


void VBO_ITEM::ChangeColor( const COLOR4D& aColor )
{
    wxASSERT_MSG( false, wxT( "This was not tested yet" ) );

    if( m_isDirty )
        Finish();

    // Point to color of vertices
    VBO_VERTEX* vertexPtr = GetVertices();
    const GLfloat newColor[] = { aColor.r, aColor.g, aColor.b, aColor.a };

    for( unsigned int i = 0; i < m_size; ++i )
    {
        memcpy( &vertexPtr->r, newColor, ColorByteSize );

        // Move on to the next vertex
        vertexPtr++;
    }
}


void VBO_ITEM::Finish()
{
    // The unknown-sized item has just ended, so we need to inform the container about it
    m_container->EndItem();

    m_isDirty = false;
}
