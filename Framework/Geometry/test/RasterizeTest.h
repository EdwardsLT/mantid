// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_RASTERIZETEST_H_
#define MANTID_GEOMETRY_RASTERIZETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Rasterize.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

class RasterizeTest : public CxxTest::TestSuite {
private:
  // all examples are taken from ShapeFactoryTest
  std::string cylinderXml =
      "<cylinder id=\"shape\"> " R
      "(<centre-of-bottom-base x=" 0.0 " y=" 0.0 " z=" 0.0 " /> )" R
                                                           "(<axis x=" 0.0 " y"
                                                                           "=" 0.0 " z=" 1 " /> )"
                                                                                           "<radius val=\"0.1\" /> "
                                                                                           "<height val=\"3\" /> "
                                                                                           "</cylinder>";

  std::string sphereXml = "<sphere id=\"shape\"> "
                          "(<centre x=\"4.1\"  y=\"2.1\" z=\"8.1\" /> )"
                          "<radius val=\"3.2\" /> "
                          "</sphere>"
                          "<algebra val=\"shape\" /> ";

  boost::shared_ptr<IObject> createCylinder() {
    return ShapeFactory().createShape(cylinderXml);
  }

  boost::shared_ptr<IObject> createSphere() {
    return ShapeFactory().createShape(sphereXml);
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RasterizeTest *createSuite() { return new RasterizeTest(); }
  static void destroySuite(RasterizeTest *suite) { delete suite; }

  void test_calculateCylinder() {
    boost::shared_ptr<IObject> cylinder = createCylinder();
    TS_ASSERT(cylinder);
    const auto raster =
        Rasterize::calculateCylinder(V3D(0., 0., 1.), cylinder, 3, 3);

    TS_ASSERT(raster.m_L1s.size() == raster.m_elementPositions.size());
    TS_ASSERT(raster.m_L1s.size() == raster.m_elementVolumes.size());

    // TODO more tests
  }

  void test_calculateCylinderFromSphere() {
    boost::shared_ptr<IObject> sphere = createSphere();
    TS_ASSERT(sphere);
    TS_ASSERT_THROWS(
        Rasterize::calculateCylinder(V3D(0., 0., 1.), sphere, 3, 3),
        std::logic_error);
  }
};

#endif /* MANTID_GEOMETRY_RASTERIZETEST_H_ */
