/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file geometry_utils.cpp
 * @brief a few functions useful in geometry calculations.
 */

#include <cstdint>
#include <algorithm>         // for max, min

//#include <eda_rect.h>
#include <geometry/geometry_utils.h>
#include <math/util.h>      // for KiROUND

// To approximate a circle by segments, a minimal seg count is mandatory.
// Note that this is rarely used as the maxError constraint will yield a higher
// segment count on everything but very small circles.  (Even a 0.125mm track
// with a 0.01mm maximum deviation yields 11 segments.)
#define MIN_SEGCOUNT_FOR_CIRCLE 8

int GetArcToSegmentCount( int aRadius, int aErrorMax, double aArcAngleDegree )
{
    // calculate the number of segments to approximate a circle by segments
    // given the max distance between the middle of a segment and the circle

    // avoid divide-by-zero
    aRadius = std::max( 1, aRadius );

    // error relative to the radius value:
    double rel_error = (double)aErrorMax / aRadius;
    // minimal arc increment in degrees:
    double arc_increment = 180 / M_PI * acos( 1.0 - rel_error ) * 2;

    // Ensure a minimal arc increment reasonable value for a circle
    // (360.0 degrees). For very small radius values, this is mandatory.
    arc_increment = std::min( 360.0/MIN_SEGCOUNT_FOR_CIRCLE, arc_increment );

    int segCount = KiROUND( fabs( aArcAngleDegree ) / arc_increment );

    // Ensure at least two segments are used for algorithmic safety
    return std::max( segCount, 2 );
}


int CircleToEndSegmentDeltaRadius( int aRadius, int aSegCount )
{
    // The minimal seg count is 3, otherwise we cannot calculate the result
    // in practice, the min count is clamped to 8 in kicad
    if( aSegCount <= 2 )
        aSegCount = 3;

    // The angle between the center of the segment and one end of the segment
    // when the circle is approximated by aSegCount segments
    double alpha = M_PI / aSegCount;

    // aRadius is the radius of the circle tangent to the middle of each segment
    // and aRadius/cos(aplha) is the radius of the circle defined by seg ends
    int delta = KiROUND( aRadius * ( 1/cos(alpha) - 1 ) );

    return delta;
}

// When creating polygons to create a clearance polygonal area, the polygon must
// be same or bigger than the original shape.
// Polygons are bigger if the original shape has arcs (round rectangles, ovals,
// circles...).  However, when building the solder mask layer modifying the shapes
// when converting them to polygons is not acceptable (the modification can break
// calculations).
// So one can disable the shape expansion within a particular scope by allocating
// a DISABLE_ARC_CORRECTION.

static bool s_disable_arc_correction = false;

DISABLE_ARC_RADIUS_CORRECTION::DISABLE_ARC_RADIUS_CORRECTION()
{
    s_disable_arc_correction = true;
}

DISABLE_ARC_RADIUS_CORRECTION::~DISABLE_ARC_RADIUS_CORRECTION()
{
    s_disable_arc_correction = false;
}

int GetCircleToPolyCorrection( int aMaxError )
{
    // Push all the error to the outside by increasing the radius
    return s_disable_arc_correction ? 0 : aMaxError;
}

