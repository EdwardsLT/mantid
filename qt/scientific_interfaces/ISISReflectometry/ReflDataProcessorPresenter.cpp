#include "ReflDataProcessorPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorView.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/WorkspaceNameUtils.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include "MantidQtWidgets/Common/ParseNumerics.h"
#include "MantidQtWidgets/Common/ProgressPresenter.h"
#include "ReflFromStdStringMap.h"

using namespace MantidQt::MantidWidgets::DataProcessor;
using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

// unnamed namespace
namespace {

/** Return the minimum number of slices for all rows in the group
 * or return 0 if there are no rows
 */
size_t getMinimumSlicesForGroup(const GroupData &group) {
  if (group.size() < 1)
    return 0;

  size_t minNumberOfSlices = std::numeric_limits<size_t>::max();

  for (const auto &row : group) {
    minNumberOfSlices =
        std::min(minNumberOfSlices, row.second->numberOfSlices());
  }

  return minNumberOfSlices;
}

/** Check whether the given row data contains a value for an angle
 */
bool hasAngle(RowData_sptr data) {
  // The angle is the second value in the row
  return (data->size() > 1 && !data->value(1).isEmpty());
}

/** Get the angle from the given row as a double. Throws if not specified
 */
double angle(RowData_sptr data) {
  if (!hasAngle(data))
    throw std::runtime_error(
        std::string("Error parsing angle: angle was not set"));

  bool ok = false;
  double angle = data->value(1).toDouble(&ok);

  if (!ok)
    throw std::runtime_error("Error parsing angle: " +
                             data->value(1).toStdString());

  return angle;
}
}

TimeSlicingInfo::TimeSlicingInfo(QString type, QString values)
    : m_type(std::move(type)), m_values(std::move(values)) {
  // For custom/log value slicing the slices are always the same so we can
  // set them straight away
  if (isCustom())
    parseCustom();
  if (isLogValue())
    parseLogValue();
}

void TimeSlicingInfo::addSlice(const double startTime, const double stopTime) {
  // Add a slice if it doesn't already exist
  if (std::find(m_startTimes.begin(), m_startTimes.end(), startTime) ==
          m_startTimes.end() &&
      std::find(m_stopTimes.begin(), m_stopTimes.end(), stopTime) ==
          m_stopTimes.end()) {
    m_startTimes.push_back(startTime);
    m_stopTimes.push_back(stopTime);
  }
}

void TimeSlicingInfo::clearSlices() {
  m_startTimes.clear();
  m_stopTimes.clear();
}

/** Parses the values string to extract custom time slicing
 */
void TimeSlicingInfo::parseCustom() {

  auto timeStr = values().split(",");
  std::vector<double> times;
  std::transform(timeStr.begin(), timeStr.end(), std::back_inserter(times),
                 [](const QString &astr) { return parseDouble(astr); });

  size_t numSlices = times.size() > 1 ? times.size() - 1 : 1;

  // Add the start/stop times
  if (times.size() == 1) {
    addSlice(0, times[0]);
  } else {
    for (size_t i = 0; i < numSlices; i++) {
      addSlice(times[i], times[i + 1]);
    }
  }
}

/** Parses the vlaues string to extract log value filter and time slicing
 */
void TimeSlicingInfo::parseLogValue() {

  // Extract the slicing and log values from the input which will be of the
  // format e.g. "Slicing=0,10,20,30,LogFilter=proton_charge"
  auto strMap = parseKeyValueQString(values());
  QString timeSlicing = strMap.at("Slicing");
  m_values = timeSlicing;
  m_logFilter = strMap.at("LogFilter");

  parseCustom();
}

/**
* Constructor
* @param whitelist : The set of properties we want to show as columns
* @param preprocessMap : A map containing instructions for pre-processing
* @param processor : A ProcessingAlgorithm
* @param postprocessor : A PostprocessingAlgorithm
* workspaces
* @param group : The zero-based index of this presenter within the tab.
* @param postprocessMap : A map containing instructions for post-processing.
* This map links column name to properties of the post-processing algorithm
* @param loader : The algorithm responsible for loading data
*/
ReflDataProcessorPresenter::ReflDataProcessorPresenter(
    const WhiteList &whitelist,
    const std::map<QString, PreprocessingAlgorithm> &preprocessMap,
    const ProcessingAlgorithm &processor,
    const PostprocessingAlgorithm &postprocessor, int group,
    const std::map<QString, QString> &postprocessMap, const QString &loader)
    : GenericDataProcessorPresenter(whitelist, preprocessMap, processor,
                                    postprocessor, group, postprocessMap,
                                    loader) {}

/**
* Destructor
*/
ReflDataProcessorPresenter::~ReflDataProcessorPresenter() {}

/**
 Process selected data
*/
void ReflDataProcessorPresenter::process() {

  // Get selected runs
  const auto newSelected = m_manager->selectedData(true);

  // Don't continue if there are no items to process
  if (newSelected.empty())
    return;

  // If slicing is not specified, process normally, delegating to
  // GenericDataProcessorPresenter
  TimeSlicingInfo slicing(m_mainPresenter->getTimeSlicingType(),
                          m_mainPresenter->getTimeSlicingValues());
  if (!slicing.hasSlicing()) {
    // Check if any input event workspaces still exist in ADS
    if (proceedIfWSTypeInADS(newSelected, true)) {
      setPromptUser(false); // Prevent prompting user twice
      GenericDataProcessorPresenter::process();
    }
    return;
  }

  m_selectedData = newSelected;

  // Check if any input non-event workspaces exist in ADS
  if (!proceedIfWSTypeInADS(m_selectedData, false))
    return;

  // Progress report
  int progress = 0;
  int maxProgress = static_cast<int>(m_selectedData.size());
  ProgressPresenter progressReporter(progress, maxProgress, maxProgress,
                                     m_progressView);

  // True if all groups were processed as event workspaces
  bool allGroupsWereEvent = true;
  // True if errors where encountered when reducing table
  bool errors = false;

  // Loop in groups
  for (const auto &item : m_selectedData) {

    // Group of runs
    GroupData group = item.second;

    try {
      // First load the runs.
      bool allEventWS = loadGroup(group);

      if (allEventWS) {
        // Process the group
        if (processGroupAsEventWS(item.first, group, slicing))
          errors = true;

        // Notebook not implemented yet
        if (m_view->getEnableNotebook()) {
          /// @todo Implement save notebook for event-sliced workspaces.
          // The per-slice input properties are stored in the RowData but
          // at the moment GenerateNotebook just uses the parent row
          // saveNotebook(m_selectedData);
          GenericDataProcessorPresenter::giveUserWarning(
              "Notebook not implemented for sliced data yet",
              "Notebook will not be generated");
        }

      } else {
        // Process the group
        if (processGroupAsNonEventWS(item.first, group))
          errors = true;
        // Notebook
        if (m_view->getEnableNotebook())
          saveNotebook(m_selectedData);
      }

      if (!allEventWS)
        allGroupsWereEvent = false;

    } catch (...) {
      errors = true;
    }
    progressReporter.report();
  }

  if (!allGroupsWereEvent)
    m_view->giveUserWarning(
        "Some groups could not be processed as event workspaces", "Warning");
  if (errors)
    m_view->giveUserWarning("Some errors were encountered when "
                            "reducing table. Some groups may not have "
                            "been fully processed.",
                            "Warning");

  progressReporter.clear();
}

/** Loads a group of runs. Tries loading runs as event workspaces. If any of the
* workspaces in the group is not an event workspace, stops loading and re-loads
* all of them as non-event workspaces. We need the workspaces to be of the same
* type to process them together.
*
* @param group :: the group of runs
* @return :: true if all runs were loaded as event workspaces. False otherwise
*/
bool ReflDataProcessorPresenter::loadGroup(const GroupData &group) {

  // Set of runs loaded successfully
  std::set<QString> loadedRuns;

  for (const auto &row : group) {

    // The run number
    auto runNo = row.second->value(0);
    // Try loading as event workspace
    bool eventWS = loadEventRun(runNo);
    if (!eventWS) {
      // This run could not be loaded as event workspace. We need to load and
      // process the whole group as non-event data.
      for (const auto &rowNew : group) {
        // The run number
        auto runNo = rowNew.second->value(0);
        // Load as non-event workspace
        loadNonEventRun(runNo);
      }
      // Remove monitors which were loaded as separate workspaces
      for (const auto &run : loadedRuns) {
        AnalysisDataService::Instance().remove(
            ("TOF_" + run + "_monitors").toStdString());
      }
      return false;
    }
    loadedRuns.insert(runNo);
  }
  return true;
}

/** Get a list of workspace property names for the workspaces
 * that will be affected by slicing, i.e. the input run and
 * all of the output workspaces will be sliced.
 *
 */
std::vector<QString>
ReflDataProcessorPresenter::getSlicedWorkspacePropertyNames() const {
  // For the input properties, the InputWorkspace is the only one that is
  // sliced. Transmission workspaces are not sliced.
  auto workspaceProperties = std::vector<QString>{"InputWorkspace"};
  auto outputProperties = m_processor.outputProperties();
  workspaceProperties.insert(workspaceProperties.end(),
                             outputProperties.begin(), outputProperties.end());
  return workspaceProperties;
}

/** Process a row as event-sliced data
 * @param rowData : the row to process
 * @param slicing : the time-slicing info
 * @return : true if processed successfully
 */
bool ReflDataProcessorPresenter::reduceRowAsEventWS(RowData_sptr rowData,
                                                    TimeSlicingInfo &slicing) {

  // Preprocess the row. Note that this only needs to be done once and
  // not for each slice because the slice data can be inferred from the
  // row data
  preprocessOptionValues(rowData);
  // Get the (preprocessed) input workspace name for the reduction. The
  // input runs are from the first column in the whitelist and we look up
  // the associated algorithm property value in the options.
  auto const &runName =
      rowData->preprocessedOptionValue(m_processor.defaultInputPropertyName());

  // Do time slicing now if using uniform slicing because this is dependant on
  // the start/stop times of the current input workspace
  if (slicing.isUniform() || slicing.isUniformEven()) {
    slicing.clearSlices();
    parseUniform(slicing, runName);
  }

  const auto slicedWorkspaceProperties = getSlicedWorkspacePropertyNames();

  // Clear slices from any previous reduction because they will be
  // recreated
  rowData->clearSlices();

  for (size_t i = 0; i < slicing.numberOfSlices(); i++) {
    try {
      // Create the slice
      QString sliceSuffix = takeSlice(runName, slicing, i);
      auto slice = rowData->addSlice(sliceSuffix, slicedWorkspaceProperties);
      // Run the algorithm
      const auto alg = createAndRunAlgorithm(slice->preprocessedOptions());

      // Populate any empty values in the row with output from the algorithm.
      // Note that this overwrites the data each time with the results
      // from the latest slice. It would be good to do some validation
      // that the results are the same for each slice e.g. the resolution
      // should always be the same.
      updateModelFromAlgorithm(alg, rowData);
    } catch (...) {
      return false;
    }
  }

  return true;
}

/** Processes a group of runs
*
* @param groupID :: An integer number indicating the id of this group
* @param group :: the group of event workspaces
* @param slicing :: Info about how time slicing should be performed
* @return :: true if errors were encountered
*/
bool ReflDataProcessorPresenter::processGroupAsEventWS(
    int groupID, const GroupData &group, TimeSlicingInfo &slicing) {

  bool errors = false;
  bool multiRow = group.size() > 1;

  for (const auto &row : group) {

    const auto rowID = row.first;      // Integer ID of this row
    const auto rowData = row.second;   // data values for this row
    auto runNo = row.second->value(0); // The run number

    // Set up all data required for processing the row
    if (!initRowForProcessing(rowData))
      return true;

    if (!reduceRowAsEventWS(rowData, slicing))
      return true;

    // Update the model with the results
    m_manager->update(groupID, rowID, rowData->data());
  }

  // Post-process (if needed)
  if (multiRow) {
    // Get the number of slices common to all groups
    auto numGroupSlices = getMinimumSlicesForGroup(group);

    addNumGroupSlicesEntry(groupID, numGroupSlices);

    // Loop through each slice index
    for (size_t i = 0; i < numGroupSlices; i++) {
      // Create a group containing the relevant slice from each row
      GroupData sliceGroup;
      for (const auto rowKvp : group) {
        auto rowIndex = rowKvp.first;
        auto rowData = rowKvp.second;
        sliceGroup[rowIndex] = rowData->getSlice(i);
      }
      // Post process the group of slices
      try {
        postProcessGroup(sliceGroup);
      } catch (...) {
        errors = true;
      }
    }
  }

  return errors;
}

/** Processes a group of non-event workspaces
*
* @param groupID :: An integer number indicating the id of this group
* @param group :: the group of event workspaces
* @return :: true if errors were encountered
*/
bool ReflDataProcessorPresenter::processGroupAsNonEventWS(int groupID,
                                                          GroupData &group) {

  bool errors = false;

  for (auto &row : group) {
    auto rowData = row.second;
    // Set up all data required for processing the row
    if (!initRowForProcessing(rowData))
      return true;
    // Do the reduction
    reduceRow(rowData);
    // Update the tree
    m_manager->update(groupID, row.first, rowData->data());
  }

  // Post-process (if needed)
  if (group.size() > 1) {
    try {
      postProcessGroup(group);
    } catch (...) {
      errors = true;
    }
  }

  return errors;
}

Mantid::API::IEventWorkspace_sptr
ReflDataProcessorPresenter::retrieveWorkspace(QString const &name) const {
  return AnalysisDataService::Instance().retrieveWS<IEventWorkspace>(
      name.toStdString());
}

/** Retrieves a workspace from the AnalysisDataService based on it's name.
 *
 * @param name :: The name of the workspace to retrieve.
 * @return A pointer to the retrieved workspace or null if the workspace does
 *not exist or
 * is not an event workspace.
 */
Mantid::API::IEventWorkspace_sptr
ReflDataProcessorPresenter::retrieveWorkspaceOrCritical(
    QString const &name) const {
  IEventWorkspace_sptr mws;
  if (workspaceExists(name)) {
    auto mws = retrieveWorkspace(name);
    if (mws == nullptr) {
      m_view->giveUserCritical("Workspace to slice " + name +
                                   " is not an event workspace!",
                               "Time slicing error");
      return nullptr;
    } else {
      return mws;
    }
  } else {
    m_view->giveUserCritical("Workspace to slice not found: " + name,
                             "Time slicing error");
    return nullptr;
  }
}

/** Parses a string to extract uniform time slicing
 *
 * @param slicing :: Info about how time slicing should be performed
 * @param wsName :: The name of the workspace to be sliced
 */
void ReflDataProcessorPresenter::parseUniform(TimeSlicingInfo &slicing,
                                              const QString &wsName) {

  IEventWorkspace_sptr mws = retrieveWorkspaceOrCritical(wsName);
  if (mws != nullptr) {
    const auto run = mws->run();
    const auto totalDuration = run.endTime() - run.startTime();
    double totalDurationSec = totalDuration.total_seconds();
    double sliceDuration = .0;
    int numSlices = 0;

    if (slicing.isUniformEven()) {
      numSlices = parseDenaryInteger(slicing.values());
      sliceDuration = totalDurationSec / numSlices;
    } else if (slicing.isUniform()) {
      sliceDuration = parseDouble(slicing.values());
      numSlices = static_cast<int>(ceil(totalDurationSec / sliceDuration));
    }

    // Add the start/stop times
    for (int i = 0; i < numSlices; i++) {
      slicing.addSlice(sliceDuration * i, sliceDuration * (i + 1));
    }
  }
}

bool ReflDataProcessorPresenter::workspaceExists(
    QString const &workspaceName) const {
  return AnalysisDataService::Instance().doesExist(workspaceName.toStdString());
}

/** Loads an event workspace and puts it into the ADS
*
* @param runNo :: The run number as a string
* @return :: True if algorithm was executed. False otherwise
*/
bool ReflDataProcessorPresenter::loadEventRun(const QString &runNo) {

  bool runFound;
  QString outName;
  QString prefix = "TOF_";
  QString instrument = m_view->getProcessInstrument();

  outName = findRunInADS(runNo, prefix, runFound);
  if (!runFound || !workspaceExists(outName + "_monitors") ||
      retrieveWorkspace(outName) == nullptr) {
    // Monitors must be loaded first and workspace must be an event workspace
    loadRun(runNo, instrument, prefix, "LoadEventNexus", runFound);
  }

  return runFound;
}

/** Loads a non-event workspace and puts it into the ADS
*
* @param runNo :: The run number as a string
*/
void ReflDataProcessorPresenter::loadNonEventRun(const QString &runNo) {

  bool runFound; // unused but required
  auto prefix = QString("TOF_");
  auto instrument = m_view->getProcessInstrument();

  findRunInADS(runNo, prefix, runFound);
  if (!runFound)
    loadRun(runNo, instrument, prefix, m_loader, runFound);
}

/** Tries loading a run from disk
 *
 * @param run : The name of the run
 * @param instrument : The instrument the run belongs to
 * @param prefix : The prefix to be prepended to the run number
 * @param loader : The algorithm used for loading runs
 * @param runFound : Whether or not the run was actually found
 * @returns string name of the run
 */
QString ReflDataProcessorPresenter::loadRun(const QString &run,
                                            const QString &instrument,
                                            const QString &prefix,
                                            const QString &loader,
                                            bool &runFound) {

  runFound = true;
  auto const fileName = instrument + run;
  auto const outputName = prefix + run;

  IAlgorithm_sptr algLoadRun =
      AlgorithmManager::Instance().create(loader.toStdString());
  algLoadRun->initialize();
  algLoadRun->setProperty("Filename", fileName.toStdString());
  algLoadRun->setProperty("OutputWorkspace", outputName.toStdString());
  if (loader == "LoadEventNexus")
    algLoadRun->setProperty("LoadMonitors", true);
  algLoadRun->execute();
  if (!algLoadRun->isExecuted()) {
    // Run not loaded from disk
    runFound = false;
    return "";
  }

  return outputName;
}

/** Takes a slice from a run and puts the 'sliced' workspace into the ADS
*
* @param runName :: The input workspace name as a string
* @param slicing :: Info about how time slicing should be performed
* @param sliceIndex :: The index of the slice being taken
* @return :: the suffix used for the slice name
*/
QString ReflDataProcessorPresenter::takeSlice(const QString &runName,
                                              TimeSlicingInfo &slicing,
                                              size_t sliceIndex) {

  QString sliceSuffix = "_slice_" + QString::number(sliceIndex);
  QString sliceName = runName + sliceSuffix;
  QString monName = runName + "_monitors";
  QString filterAlg =
      slicing.logFilter().isEmpty() ? "FilterByTime" : "FilterByLogValue";

  auto startTime = slicing.startTime(sliceIndex);
  auto stopTime = slicing.stopTime(sliceIndex);

  // Filter the run using the appropriate filter algorithm
  IAlgorithm_sptr filter =
      AlgorithmManager::Instance().create(filterAlg.toStdString());
  filter->initialize();
  filter->setProperty("InputWorkspace", runName.toStdString());
  filter->setProperty("OutputWorkspace", sliceName.toStdString());
  if (filterAlg == "FilterByTime") {
    filter->setProperty("StartTime", startTime);
    filter->setProperty("StopTime", stopTime);
  } else { // FilterByLogValue
    filter->setProperty("MinimumValue", startTime);
    filter->setProperty("MaximumValue", stopTime);
    filter->setProperty("TimeTolerance", 1.0);
    filter->setProperty("LogName", slicing.logFilter().toStdString());
  }

  filter->execute();

  // Obtain the normalization constant for this slice
  IEventWorkspace_sptr mws = retrieveWorkspace(runName);
  double total = mws->run().getProtonCharge();
  mws = retrieveWorkspace(sliceName);
  double slice = mws->run().getProtonCharge();
  double scaleFactor = slice / total;

  IAlgorithm_sptr scale = AlgorithmManager::Instance().create("Scale");
  scale->initialize();
  scale->setProperty("InputWorkspace", monName.toStdString());
  scale->setProperty("Factor", scaleFactor);
  scale->setProperty("OutputWorkspace", "__" + monName.toStdString() + "_temp");
  scale->execute();

  IAlgorithm_sptr rebinDet =
      AlgorithmManager::Instance().create("RebinToWorkspace");
  rebinDet->initialize();
  rebinDet->setProperty("WorkspaceToRebin", sliceName.toStdString());
  rebinDet->setProperty("WorkspaceToMatch",
                        "__" + monName.toStdString() + "_temp");
  rebinDet->setProperty("OutputWorkspace", sliceName.toStdString());
  rebinDet->setProperty("PreserveEvents", false);
  rebinDet->execute();

  IAlgorithm_sptr append = AlgorithmManager::Instance().create("AppendSpectra");
  append->initialize();
  append->setProperty("InputWorkspace1",
                      "__" + monName.toStdString() + "_temp");
  append->setProperty("InputWorkspace2", sliceName.toStdString());
  append->setProperty("OutputWorkspace", sliceName.toStdString());
  append->setProperty("MergeLogs", true);
  append->execute();

  // Remove temporary monitor ws
  AnalysisDataService::Instance().remove("__" + monName.toStdString() +
                                         "_temp");

  return sliceSuffix;
}

/** Plots any currently selected rows */
void ReflDataProcessorPresenter::plotRow() {

  const auto selectedData = m_manager->selectedData();
  if (selectedData.size() == 0)
    return;

  // If slicing values are empty plot normally
  auto timeSlicingValues =
      m_mainPresenter->getTimeSlicingValues().toStdString();
  if (timeSlicingValues.empty()) {
    GenericDataProcessorPresenter::plotRow();
    return;
  }

  // Set of workspaces to plot
  QOrderedSet<QString> workspaces;
  // Set of workspaces not found in the ADS
  QSet<QString> notFound;
  // Get the property name for the default output workspace so we
  // can find the reduced workspace name for each slice
  const auto outputPropertyName = m_processor.defaultOutputPropertyName();

  for (const auto &selectedItem : selectedData) {

    auto groupData = selectedItem.second;

    for (const auto &rowItem : groupData) {
      auto rowData = rowItem.second;
      const size_t numSlices = rowData->numberOfSlices();

      for (size_t slice = 0; slice < numSlices; slice++) {
        auto sliceData = rowData->getSlice(slice);
        const auto sliceName =
            sliceData->preprocessedOptionValue(outputPropertyName);
        if (workspaceExists(sliceName))
          workspaces.insert(sliceName, nullptr);
        else
          notFound.insert(sliceName);
      }
    }
  }

  if (!notFound.isEmpty())
    issueNotFoundWarning("rows", notFound);

  plotWorkspaces(workspaces);
}

/** Plots any currently selected groups */
void ReflDataProcessorPresenter::plotGroup() {

  const auto selectedData = m_manager->selectedData();
  if (selectedData.size() == 0)
    return;

  // If slicing values are empty plot normally
  auto timeSlicingValues = m_mainPresenter->getTimeSlicingValues();
  if (timeSlicingValues.isEmpty()) {
    GenericDataProcessorPresenter::plotGroup();
    return;
  }

  // Set of workspaces to plot
  QOrderedSet<QString> workspaces;
  // Set of workspaces not found in the ADS
  QSet<QString> notFound;

  for (const auto &selectedItem : selectedData) {
    auto groupIndex = selectedItem.first;
    auto groupData = selectedItem.second;
    // Only consider multi-row groups
    if (groupData.size() < 2)
      continue;
    // We should always have a record of the number of slices for this group
    if (m_numGroupSlicesMap.count(groupIndex) < 1)
      throw std::runtime_error("Invalid group data for group " +
                               std::to_string(groupIndex));

    size_t numSlices = m_numGroupSlicesMap.at(groupIndex);

    for (size_t slice = 0; slice < numSlices; slice++) {

      const auto wsName = getPostprocessedWorkspaceName(groupData, slice);

      if (workspaceExists(wsName))
        workspaces.insert(wsName, nullptr);
      else
        notFound.insert(wsName);
    }
  }

  if (!notFound.isEmpty())
    issueNotFoundWarning("groups", notFound);

  plotWorkspaces(workspaces);
}

/** Asks user if they wish to proceed if the AnalysisDataService contains input
 * workspaces of a specific type
 *
 * @param data :: The data selected in the table
 * @param findEventWS :: Whether or not we are searching for event workspaces
 * @return :: Boolean - true if user wishes to proceed, false if not
 */
bool ReflDataProcessorPresenter::proceedIfWSTypeInADS(const TreeData &data,
                                                      const bool findEventWS) {

  QStringList foundInputWorkspaces;

  for (const auto &item : data) {
    const auto group = item.second;

    for (const auto &row : group) {
      bool runFound = false;
      auto runNo = row.second->value(0);
      auto outName = findRunInADS(runNo, "TOF_", runFound);

      if (runFound) {
        bool isEventWS = retrieveWorkspace(outName) != nullptr;
        if (findEventWS == isEventWS) {
          foundInputWorkspaces.append(outName);
        } else if (isEventWS) { // monitors must be loaded
          auto monName = outName + "_monitors";
          if (!workspaceExists(monName))
            foundInputWorkspaces.append(outName);
        }
      }
    }
  }

  if (foundInputWorkspaces.size() > 0) {
    // Input workspaces of type found, ask user if they wish to process
    auto foundStr = foundInputWorkspaces.join("\n");

    bool process = m_view->askUserYesNo(
        "Processing selected rows will replace the following workspaces:\n\n" +
            foundStr + "\n\nDo you wish to continue?",
        "Process selected rows?");

    if (process) {
      // Remove all found workspaces
      for (auto &wsName : foundInputWorkspaces) {
        AnalysisDataService::Instance().remove(wsName.toStdString());
      }
    }

    return process;
  }

  // No input workspaces of type found, proceed with reduction automatically
  return true;
}

/** Add entry for the number of slices for all rows in a group
*
* @param groupID :: The ID of the group
* @param numSlices :: Number of slices
*/
void ReflDataProcessorPresenter::addNumGroupSlicesEntry(int groupID,
                                                        size_t numSlices) {
  m_numGroupSlicesMap[groupID] = numSlices;
}

/** Get the processing options for a given row
 *
 * @param data : the row data
 * @throws :: if the settings the user entered are invalid
 * */
OptionsMap ReflDataProcessorPresenter::getProcessingOptions(RowData_sptr data) {
  // Return the global settings but also include the transmission runs,
  // which vary depending on which row is being processed
  auto options = m_processingOptions;

  // Get the angle for the current row. The angle is the second data item
  if (!hasAngle(data)) {
    if (m_mainPresenter->hasPerAngleTransmissionRuns()) {
      // The user has specified per-angle transmission runs on the settings
      // tab. In theory this is fine, but it could cause confusion when the
      // angle is not available in the data processor table because the
      // per-angle transmission runs will NOT be used. However, the angle will
      // be updated in the table AFTER reduction is run, so it might look like
      // it should have been used (and it WILL be used next time if reduction
      // is re-run).
      throw std::runtime_error(
          "An angle must be specified for all rows because "
          "per-angle transmission runs are specified in the "
          "Settings tab. Please enter angles for all runs, "
          "or remove the per-angle settings.");
    } else {
      // If per-angle transmission runs are not set then it's fine to just use
      // any default transmission runs, which will already be in the options.
      return options;
    }
  }

  // Insert the transmission runs as the "FirstTransmissionRun" property
  auto transmissionRuns =
      m_mainPresenter->getTransmissionRunsForAngle(angle(data));
  if (!transmissionRuns.isEmpty()) {
    options["FirstTransmissionRun"] = transmissionRuns;
  }

  return options;
}
}
}
