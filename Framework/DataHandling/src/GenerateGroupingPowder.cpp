// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/GenerateGroupingPowder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/System.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/XML/XMLWriter.h>

#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Poco::XML;

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GenerateGroupingPowder)

/// Algorithm's name for identification. @see Algorithm::name
const std::string GenerateGroupingPowder::name() const {
  return "GenerateGroupingPowder";
}

/// Algorithm's version for identification. @see Algorithm::version
int GenerateGroupingPowder::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string GenerateGroupingPowder::category() const {
  return R"(DataHandling\Grouping;Transforms\Grouping;Diffraction\Utility)";
}

/** Initialize the algorithm's properties.
 */
void GenerateGroupingPowder::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "A workspace from which to generate the grouping.");
  auto positiveDouble = boost::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0.0);
  declareProperty("AngleStep", -1.0, positiveDouble,
                  "The angle step for grouping, in degrees.");
  declareProperty(make_unique<FileProperty>("GroupingFilename", "",
                                            FileProperty::Save, ".xml"),
                  "A grouping file that will be created.");
  declareProperty("GenerateParFile", true,
                  "If true, a par file with a corresponding name to the "
                  "grouping file will be generated.");
}

/** Execute the algorithm.
 */
void GenerateGroupingPowder::exec() {
  MatrixWorkspace_const_sptr input_ws = getProperty("InputWorkspace");
  const auto &detectorInfo = input_ws->detectorInfo();
  const auto &detectorIDs = detectorInfo.detectorIDs();
  if (detectorIDs.empty())
    throw std::invalid_argument("Workspace contains no detectors.");

  const double step = getProperty("AngleStep");
  const size_t numSteps = static_cast<size_t>(180. / step + 1);

  std::vector<std::vector<detid_t>> groups(numSteps);
  std::vector<double> twoThetaAverage(numSteps, 0.);
  std::vector<double> rAverage(numSteps, 0.);

  for (size_t i = 0; i < detectorIDs.size(); ++i) {
    if (detectorInfo.isMonitor(i))
      continue;

    const double tt = detectorInfo.twoTheta(i) * Geometry::rad2deg;
    const double r = detectorInfo.l2(i);
    const size_t where = static_cast<size_t>(tt / step);
    groups[where].emplace_back(detectorIDs[i]);
    twoThetaAverage[where] += tt;
    rAverage[where] += r;
  }

  const std::string XMLfilename = getProperty("GroupingFilename");

  // XML
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> pRoot = pDoc->createElement("detector-grouping");
  pDoc->appendChild(pRoot);
  pRoot->setAttribute("instrument", input_ws->getInstrument()->getName());

  size_t goodGroups(0);
  for (size_t i = 0; i < numSteps; ++i) {
    size_t gSize = groups.at(i).size();
    if (gSize > 0) {
      ++goodGroups;
      std::stringstream spID, textvalue;
      spID << i;
      AutoPtr<Element> pChildGroup = pDoc->createElement("group");
      pChildGroup->setAttribute("ID", spID.str());
      pRoot->appendChild(pChildGroup);

      std::copy(groups.at(i).begin(), groups.at(i).end(),
                std::ostream_iterator<size_t>(textvalue, ","));
      std::string text = textvalue.str();
      const size_t found = text.rfind(',');
      if (found != std::string::npos) {
        text.erase(found, 1); // erase the last comma
      }

      AutoPtr<Element> pDetid = pDoc->createElement("detids");
      AutoPtr<Text> pText1 = pDoc->createTextNode(text);
      pDetid->appendChild(pText1);
      pChildGroup->appendChild(pDetid);
    }
  }
  if (goodGroups == 0) {
    g_log.error("Something terrible has happened: I cannot find any detectors "
                "with scattering angle between 0 and 180 degrees");
    throw Exception::InstrumentDefinitionError(
        "Detector scattering angles not between 0 and 180 degrees");
  }

  DOMWriter writer;
  writer.setNewLine("\n");
  writer.setOptions(XMLWriter::PRETTY_PRINT);

  std::ofstream ofs(XMLfilename.c_str());
  if (!ofs) {
    g_log.error("Unable to create file: " + XMLfilename);
    throw Exception::FileError("Unable to create file: ", XMLfilename);
  }
  ofs << "<?xml version=\"1.0\"?>\n";

  writer.writeNode(ofs, pDoc);
  ofs.close();

  // PAR file
  const bool generatePar = getProperty("GenerateParFile");
  if (generatePar) {
    std::string PARfilename = XMLfilename;
    PARfilename.replace(PARfilename.end() - 3, PARfilename.end(), "par");
    std::ofstream outPAR_file(PARfilename.c_str());
    if (!outPAR_file) {
      g_log.error("Unable to create file: " + PARfilename);
      throw Exception::FileError("Unable to create file: ", PARfilename);
    }
    // Write the number of detectors to the file.
    outPAR_file << " " << goodGroups << '\n';

    for (size_t i = 0; i < numSteps; ++i) {
      const size_t gSize = groups.at(i).size();
      if (gSize > 0) {
        outPAR_file << std::fixed << std::setprecision(3);
        outPAR_file.width(10);
        outPAR_file << rAverage.at(i) / static_cast<double>(gSize);
        outPAR_file.width(10);
        outPAR_file << twoThetaAverage.at(i) / static_cast<double>(gSize);
        outPAR_file.width(10);
        outPAR_file << 0.;
        outPAR_file.width(10);
        outPAR_file << step * Geometry::deg2rad * rAverage.at(i) /
                           static_cast<double>(gSize);
        outPAR_file.width(10);
        outPAR_file << 0.01;
        outPAR_file.width(10);
        outPAR_file << (groups.at(i)).at(0) << '\n';
      }
    }

    // Close the file
    outPAR_file.close();
  }
}

} // namespace DataHandling
} // namespace Mantid
