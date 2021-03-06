// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_SAVEISAWPEAKSTEST_H_
#define MANTID_CRYSTAL_SAVEISAWPEAKSTEST_H_

#include "MantidCrystal/SaveIsawPeaks.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class SaveIsawPeaksTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SaveIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void do_test(int numRuns, size_t numBanks, size_t numPeaksPerBank) {
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular(4, 10, 1.0);
    PeaksWorkspace_sptr ws(new PeaksWorkspace());
    ws->setInstrument(inst);

    for (int run = 1000; run < numRuns + 1000; run++)
      for (size_t b = 1; b <= numBanks; b++)
        for (size_t i = 0; i < numPeaksPerBank; i++) {
          V3D hkl(static_cast<double>(i), static_cast<double>(i),
                  static_cast<double>(i));
          DblMatrix gon(3, 3, true);
          Peak p(inst, static_cast<detid_t>(b * 100 + i + 1 + i * 10),
                 static_cast<double>(i) * 1.0 + 0.5, hkl, gon);
          p.setRunNumber(run);
          p.setIntensity(static_cast<double>(i) + 0.1);
          p.setSigmaIntensity(sqrt(static_cast<double>(i)));
          p.setBinCount(static_cast<double>(i));
          p.setPeakNumber((run - 1000) *
                              static_cast<int>(numBanks * numPeaksPerBank) +
                          static_cast<int>(b * numPeaksPerBank + i));
          ws->addPeak(p);
        }

    std::string outfile = "./SaveIsawPeaksTest.peaks";
    SaveIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", outfile));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Test appending same file to check peak numbers
    SaveIsawPeaks alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize())
    TS_ASSERT(alg2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("Filename", outfile));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("AppendFile", true));
    TS_ASSERT_THROWS_NOTHING(alg2.execute(););

    // Get the file
    if (numPeaksPerBank > 0) {
      outfile = alg2.getPropertyValue("Filename");
      TS_ASSERT(Poco::File(outfile).exists());
      std::ifstream in(outfile.c_str());
      std::string line, line0;
      while (!in.eof()) // To get you all the lines.
      {
        getline(in, line0); // Saves the line in STRING.
        if (in.eof())
          break;
        line = line0;
      }
      TS_ASSERT_EQUALS(line, "3     71   -3   -3   -3    3.00     4.00    "
                             "27086  2061.553   0.24498   0.92730   3.500000   "
                             "14.3227        3       3.10    1.73   310");
    }

    if (Poco::File(outfile).exists())
      Poco::File(outfile).remove();
  }

  /// Test with an empty PeaksWorkspace
  void test_empty() { do_test(0, 0, 0); }

  /// Test with a few peaks
  void test_exec() { do_test(2, 4, 4); }
};

#endif /* MANTID_CRYSTAL_SAVEISAWPEAKSTEST_H_ */
