/******************************************************************************\

    CAMotics is an Open-Source simulation and CAM software.
    Copyright (C) 2011-2015 Joseph Coffland <joseph@cauldrondevelopment.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

\******************************************************************************/

#include "Workpiece.h"

using namespace std;
using namespace cb;
using namespace CAMotics;


Workpiece::Workpiece(const Rectangle3R &r) :
  Rectangle3R(r), center(r.getCenter()) {
  Vector3R halfDim = r.getDimensions() / 2;
  halfDim2 = halfDim * halfDim;
}


real Workpiece::depth(const Vector3R &p) const {
  real d2 = p.distanceSquared(closestPointOnSurface(p));
  return Rectangle3R::contains(p) ? d2 : -d2;
}
