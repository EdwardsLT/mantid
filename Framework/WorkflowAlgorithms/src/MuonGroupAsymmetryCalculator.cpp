#include "MantidWorkflowAlgorithms/MuonGroupAsymmetryCalculator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::AlgorithmManager;
using Mantid::API::WorkspaceFactory;



namespace Mantid {
namespace WorkflowAlgorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
* @param inputWS :: [input] Input workspace group
* @param summedPeriods :: [input] Vector of period indexes to be summed
* @param subtractedPeriods :: [input] Vector of period indexes to be subtracted
* from summed periods
* @param groupIndex :: [input] Workspace index of the group to analyse
 */
MuonGroupAsymmetryCalculator::MuonGroupAsymmetryCalculator(
    const Mantid::API::WorkspaceGroup_sptr inputWS,
    const std::vector<int> summedPeriods,
    const std::vector<int> subtractedPeriods, const int groupIndex,const double start, const double end)
    : MuonGroupCalculator(inputWS, summedPeriods, subtractedPeriods,
		groupIndex) {
	MuonGroupCalculator::SetStartEnd(start, end);}


/**
* Calculates asymmetry between given group (specified via group index) and Muon
* exponential decay
* @returns Workspace containing result of calculation
*/
MatrixWorkspace_sptr MuonGroupAsymmetryCalculator::calculate() const {
  // The output workspace

  MatrixWorkspace_sptr tempWS;
  int numPeriods = m_inputWS->getNumberOfEntries();
  if (numPeriods > 1) {
    // Several period workspaces were supplied

    auto summedWS = sumPeriods(m_summedPeriods);
    auto subtractedWS = sumPeriods(m_subtractedPeriods);

    // Remove decay (summed periods ws)
    MatrixWorkspace_sptr asymSummedPeriods =
	EstimateAsymmetry(summedWS, m_groupIndex);

    if (!m_subtractedPeriods.empty()) {
      // Remove decay (subtracted periods ws)
      MatrixWorkspace_sptr asymSubtractedPeriods =
			EstimateAsymmetry(subtractedWS, m_groupIndex);

      // Now subtract
      tempWS = subtractWorkspaces(asymSummedPeriods, asymSubtractedPeriods);
    } else {
      tempWS = asymSummedPeriods;
    }
  } else {
    // Only one period was supplied
	  tempWS = EstimateAsymmetry(m_inputWS->getItem(0), m_groupIndex);// change -1 to m_groupIndex and follow through to store as a table for later. 

  }

  // Extract the requested spectrum
  MatrixWorkspace_sptr outWS = extractSpectrum(tempWS, m_groupIndex);
  return outWS;
}



/**
* Removes exponential decay from the given workspace.
* @param inputWS :: [input] Workspace to remove decay from
* @param index :: [input] GroupIndex (fit only the requested spectrum): use -1
* for "unset"
* @returns Result of the removal
*/
MatrixWorkspace_sptr
MuonGroupAsymmetryCalculator::removeExpDecay(const Workspace_sptr &inputWS,
                                             const int index) const {
  MatrixWorkspace_sptr outWS;
  // Remove decay
  if (inputWS) {
    IAlgorithm_sptr asym =
        AlgorithmManager::Instance().create("RemoveExpDecay");
    asym->setChild(true);
    asym->setProperty("InputWorkspace", inputWS);
    if (index > 0) {
      // GroupIndex as vector
      // Necessary if we want RemoveExpDecay to fit only the requested
      // spectrum
      std::vector<int> spec(1, index);
      asym->setProperty("Spectra", spec);
    }
    asym->setProperty("OutputWorkspace", "__NotUsed__");
    asym->execute();
    outWS = asym->getProperty("OutputWorkspace");
  }
  return outWS;
}
/**
* Removes exponential decay from the given workspace.
* @param inputWS :: [input] Workspace to remove decay from
* @param index :: [input] GroupIndex (fit only the requested spectrum): use -1
* for "unset"
* @returns Result of the removal
*/
MatrixWorkspace_sptr
MuonGroupAsymmetryCalculator::EstimateAsymmetry(const Workspace_sptr &inputWS,
	const int index) const {
	std::vector<double> normEst;
	MatrixWorkspace_sptr outWS;
	// Remove decay
	if (inputWS) {
		IAlgorithm_sptr asym =
			AlgorithmManager::Instance().create("EstimateMuonAsymmetryFromCounts");
		asym->setChild(true);
		asym->setProperty("InputWorkspace", inputWS);
		if (index > -1) {
			// GroupIndex as vector
			// Necessary if we want RemoveExpDecay to fit only the requested
			// spectrum
			std::vector<int> spec(1, index);
			asym->setProperty("Spectra", spec);
		}
		asym->setProperty("OutputWorkspace", "__NotUsed__");
		asym->setProperty("StartX", StartX);
		asym->setProperty("EndX", EndX);
		asym->execute();
		outWS = asym->getProperty("OutputWorkspace");
		auto tmp = asym->getPropertyValue("NormalizationConstant");
		normEst = convertToVec(tmp);
		ITableWorkspace_sptr table= WorkspaceFactory::Instance().createTable();
			API::AnalysisDataService::Instance().addOrReplace("norm", table);
			table->addColumn("double", "norm");
			table->addColumn("int", "spectra");

			for (double norm : normEst) {
				API::TableRow row = table->appendRow();

				row << norm<<index;
			}
	}
	return outWS;
}

std::vector<double> convertToVec(std::string const &list) {
	std::vector<double> vec;
	std::vector<std::string> tmpVec;
	boost::split(tmpVec, list, boost::is_any_of(","));
	std::transform(tmpVec.begin(), tmpVec.end(), std::back_inserter(vec),
		[](std::string const &element) { return std::stod(element); });
	return vec;
}
} // namespace WorkflowAlgorithms
} // namespace Mantid
