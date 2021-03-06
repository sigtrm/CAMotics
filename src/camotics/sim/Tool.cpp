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

#include "Tool.h"

#include <camotics/cutsim/ConicSweep.h>
#include <camotics/cutsim/CompositeSweep.h>
#include <camotics/cutsim/SpheroidSweep.h>


#include <cbang/String.h>
#include <cbang/Math.h>
#include <cbang/Exception.h>
#include <cbang/xml/XMLWriter.h>
#include <cbang/util/DefaultCatch.h>

#include <string.h>


using namespace std;
using namespace cb;
using namespace CAMotics;


const char *Tool::VARS = "XYZABCUVWRIJQ";
Tool Tool::null;


Tool::Tool(unsigned number, unsigned pocket, ToolUnits units) :
  number(number), pocket(pocket), units(units),
  shape(ToolShape::TS_CYLINDRICAL) {

  Axes::clear();
  memset(vars, 0, sizeof(vars));
  setShape(ToolShape::TS_CYLINDRICAL);

  if (units == ToolUnits::UNITS_MM) {
    setLength(10);
    setRadius(1);

  } else {
    setLength(25.4);
    setRadius(25.4 / 16);
  }

  setSnubDiameter(0);
}


string Tool::getSizeText() const {
  double diameter = getDiameter();
  double length = getLength();

  if (getUnits() == ToolUnits::UNITS_INCH) {
    diameter /= 25.4;
    length /= 25.4;
  }

  return String::printf("%gx%g", diameter, length) +
    String::toLower(getUnits().toString());
}


string Tool::getText() const {
  if (!description.empty()) return description;

  return getSizeText() + " " +
    String::capitalize(String::toLower(getShape().toString()));
}


SmartPointer<Sweep> Tool::getSweep() const {
  switch (getShape()) {
  case ToolShape::TS_CYLINDRICAL:
    return new ConicSweep(getLength(), getRadius(), getRadius());

  case ToolShape::TS_CONICAL:
    return new ConicSweep(getLength(), getRadius(), 0);

  case ToolShape::TS_BALLNOSE: {
    SmartPointer<CompositeSweep> composite = new CompositeSweep;
    composite->add(new SpheroidSweep(getRadius(), 2 * getRadius()), 0);
    composite->add(new ConicSweep(getLength(), getRadius(), getRadius()),
                   getRadius());
    return composite;
  }

  case ToolShape::TS_SPHEROID:
    return new SpheroidSweep(getRadius(), getLength());

  case ToolShape::TS_SNUBNOSE:
    return new ConicSweep(getLength(), getRadius(), getSnubDiameter() / 2);
  }

  THROWS("Invalid tool shape " << getShape());
}


ostream &Tool::print(ostream &stream) const {
  stream << "T" << number << " R" << getRadius() << " L" << getLength();
  return Axes::print(stream);
}


void Tool::read(const XMLAttributes &attrs) {
  if (attrs.has("units")) try {
      setUnits(ToolUnits::parse(attrs["units"]));
    } CATCH_ERROR;

  if (attrs.has("shape")) try {
      setShape(ToolShape::parse(attrs["shape"]));
    } CATCH_ERROR;

  double scale = getUnits() == ToolUnits::UNITS_INCH ? 25.4 : 1;

  if (attrs.has("length"))
    setLength(String::parseDouble(attrs["length"]) * scale);
  else THROWS("Tool " << number << " missing length");

  if (attrs.has("radius"))
    setRadius(String::parseDouble(attrs["radius"]) * scale);
  else if (attrs.has("diameter"))
    setRadius(String::parseDouble(attrs["diameter"]) / 2.0 * scale);
  else THROWS("Tool " << number << " has neither radius or diameter");

  if (attrs.has("snub_diameter"))
    setSnubDiameter(String::parseDouble(attrs["snub_diameter"]) * scale);

  if (attrs.has("front_angle"))
    setFrontAngle(String::parseDouble(attrs["front_angle"]));

  if (attrs.has("back_angle"))
    setBackAngle(String::parseDouble(attrs["back_angle"]));

  if (attrs.has("orientation"))
    setOrientation(String::parseDouble(attrs["orientation"]));
}


void Tool::write(XMLWriter &writer) const {
  double scale = getUnits() == ToolUnits::UNITS_INCH ? 1.0 / 25.4 : 1;
  const double small = 0.0000001;

  XMLAttributes attrs;
  attrs["number"] = String(number);
  attrs["units"] = getUnits().toString();
  attrs["shape"] = getShape().toString();
  attrs["length"] = String(getLength() * scale);
  attrs["radius"] = String(getRadius() * scale);
  if (getShape() == ToolShape::TS_SNUBNOSE && small < getSnubDiameter())
    attrs["snub_diameter"] = String(getSnubDiameter() * scale);

  if (small < getFrontAngle()) attrs["front_angle"] = String(getFrontAngle());
  if (small < getBackAngle()) attrs["back_angle"] = String(getBackAngle());
  if (small < getOrientation()) attrs["orientation"] = String(getOrientation());

  writer.simpleElement("tool", getDescription(), attrs);
}


void Tool::write(JSON::Sink &sink, bool withNumber) const {
  sink.beginDict();

  double scale = units == ToolUnits::UNITS_INCH ? 1.0 / 25.4 : 1;

  if (withNumber) sink.insert("number", number);
  sink.insert("units", getUnits().toString());
  sink.insert("shape", getShape().toString());
  sink.insert("length", getLength() * scale);
  sink.insert("diameter", getDiameter() * scale);
  if (getShape() == ToolShape::TS_SNUBNOSE)
    sink.insert("snub_diameter", getSnubDiameter() * scale);
  if (getFrontAngle()) sink.insert("font_angle", getFrontAngle());
  if (getBackAngle()) sink.insert("back_angle", getBackAngle());
  if (getOrientation()) sink.insert("orientation", getOrientation());
  sink.insert("description", getDescription());

  sink.endDict();
}


void Tool::read(const JSON::Value &value) {
  setNumber(value.getU32("number", number));

  if (value.hasString("units"))
    units = ToolUnits::parse(value.getString("units"));

  double scale = units == ToolUnits::UNITS_INCH ? 25.4 : 1;

  if (value.hasString("shape"))
    shape = ToolShape::parse(value.getString("shape"));

  if (value.hasNumber("length"))
    setLength(value.getNumber("length") * scale);

  if (value.hasNumber("diameter"))
    setDiameter(value.getNumber("diameter") * scale);

  if (value.hasNumber("snub_diameter"))
    setSnubDiameter(value.getNumber("snub_diameter") * scale);

  if (value.hasNumber("front_angle"))
    setFrontAngle(value.getNumber("front_angle"));

  if (value.hasNumber("back_angle"))
    setBackAngle(value.getNumber("back_angle"));

  if (value.hasNumber("orientation"))
    setOrientation(value.getNumber("orientation"));

  setDescription(value.getString("description", ""));
}
