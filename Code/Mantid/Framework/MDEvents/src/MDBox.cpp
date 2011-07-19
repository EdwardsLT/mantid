#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidNexus/NeXusFile.hpp"

namespace Mantid
{
namespace MDEvents
{

  //-----------------------------------------------------------------------------------------------
  /** Empty constructor */
  TMDE(MDBox)::MDBox()
   : IMDBox<MDE, nd>(),
     m_fileIndexStart(0), m_fileNumEvents(0), m_onDisk(false)
  {
  }

  //-----------------------------------------------------------------------------------------------
  /** Constructor
   * @param controller :: BoxController that controls how boxes split
   * @param depth :: splitting depth of the new box.
   */
  TMDE(MDBox)::MDBox(BoxController_sptr controller, const size_t depth)
        : m_fileIndexStart(0), m_fileNumEvents(0), m_onDisk(false)
  {
    if (controller->getNDims() != nd)
      throw std::invalid_argument("MDBox::ctor(): controller passed has the wrong number of dimensions.");
    this->m_BoxController = controller;
    this->m_depth = depth;
    // Give it a fresh ID from the controller.
    this->setId( controller->getNextId() );
  }

  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDE(
  void MDBox)::clear()
  {
    this->m_signal = 0.0;
    this->m_errorSquared = 0.0;
    m_fileNumEvents = 0;
    data.clear();
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this box */
  TMDE(
  size_t MDBox)::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of un-split MDBoxes in this box (including all children)
   * @return :: 1 always since this is just a MD Box*/
  TMDE(
  size_t MDBox)::getNumMDBoxes() const
  {
    return 1;
  }

  //-----------------------------------------------------------------------------------------------
  /// Fill a vector with all the boxes up to a certain depth
  TMDE(
  void MDBox)::getBoxes(std::vector<IMDBox<MDE,nd> *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/)
  {
    boxes.push_back(this);
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  TMDE(size_t MDBox)::getNPoints() const
  {
    if (m_onDisk)
      return m_fileNumEvents;
    else
      return data.size();
  }


  //-----------------------------------------------------------------------------------------------
  /** Set the start/end point in the file where the events are located
   * @param start :: start point,
   * @param numEvents :: number of events in the file   */
  TMDE(
  void MDBox)::setFileIndex(uint64_t start, uint64_t numEvents)
  {
    m_fileIndexStart = start;
    m_fileNumEvents = numEvents;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns a reference to the events vector contained within.
   */
  TMDE(
  std::vector< MDE > & MDBox)::getEvents()
  {
    if (m_onDisk)
    {
      ::NeXus::File * file = this->m_BoxController->getFile();
      if (file)
      {
        //if (m_BoxController->UseFile())
        data.clear();
        MDE::loadVectorFromNexusSlab(data, file, m_fileIndexStart, m_fileNumEvents);
      }
    }
    return data;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns a reference to the events vector contained within.
   */
  TMDE(
  const std::vector< MDE > & MDBox)::getEvents() const
  {
    if (m_onDisk)
    {
      ::NeXus::File * file = this->m_BoxController->getFile();
      if (file)
      {
        //if (m_BoxController->UseFile())
        data.clear();
        MDE::loadVectorFromNexusSlab(data, file, m_fileIndexStart, m_fileNumEvents);
      }
    }
    return data;
  }

  //-----------------------------------------------------------------------------------------------
  TMDE(
  void MDBox)::releaseEvents() const
  {
    //TODO: Check a dirty flag and save the data if required?
    if (m_onDisk)
    {
      // Free up memory by clearing the events
      data.clear();
      vec_t().swap(data); // Linux trick to really free the memory
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Allocate and return a vector with a copy of all events contained
   */
  TMDE(
  std::vector< MDE > * MDBox)::getEventsCopy()
  {
    std::vector< MDE > * out = new std::vector<MDE>();
    //Make the copy
    out->insert(out->begin(), data.begin(), data.end());
    return out;
  }

  //-----------------------------------------------------------------------------------------------
  /** Refresh the cache.
   * For MDBox, the signal/error is tracked on adding, so
   * this just calculates the centroid.
   */
  TMDE(
  void MDBox)::refreshCache(Kernel::ThreadScheduler * /*ts*/)
  {
#ifndef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    // Use the cached value if it is on disk
    if (!m_onDisk) // TODO: Dirty flag?
    {
      this->m_signal = 0;
      this->m_errorSquared = 0;

      typename std::vector<MDE>::const_iterator it_end = data.end();
      for(typename std::vector<MDE>::const_iterator it = data.begin(); it != it_end; it++)
      {
        const MDE & event = *it;
        this->m_signal += event.getSignal();
        this->m_errorSquared += event.getErrorSquared();
      }
    }
#endif
  }


  //-----------------------------------------------------------------------------------------------
  /** Calculate and ccache the centroid of this box.
   */
  TMDE(
  void MDBox)::refreshCentroid(Kernel::ThreadScheduler * /*ts*/)
  {
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] = 0;

    // Signal was calculated before (when adding)
    // Keep 0.0 if the signal is null. This avoids dividing by 0.0
    if (this->m_signal == 0) return;

    typename std::vector<MDE>::const_iterator it_end = data.end();
    for(typename std::vector<MDE>::const_iterator it = data.begin(); it != it_end; it++)
    {
      const MDE & event = *it;
      double signal = event.getSignal();
      for (size_t d=0; d<nd; d++)
      {
        // Total up the coordinate weighted by the signal.
        this->m_centroid[d] += event.getCenter(d) * signal;
      }
    }

    // Normalize by the total signal
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] /= this->m_signal;
  }


  //-----------------------------------------------------------------------------------------------
  /** Calculate the statistics for each dimension of this MDBox, using
   * all the contained events
   * @param stats :: nd-sized fixed array of MDDimensionStats, reset to 0.0 before!
   */
  TMDE(
  void MDBox)::calculateDimensionStats(MDDimensionStats * stats) const
  {
    typename std::vector<MDE>::const_iterator it_end = data.end();
    for(typename std::vector<MDE>::const_iterator it = data.begin(); it != it_end; it++)
    {
      const MDE & event = *it;
      for (size_t d=0; d<nd; d++)
      {
        stats[d].addPoint( event.getCenter(d) );
      }
    }
  }



  //-----------------------------------------------------------------------------------------------
  /** Add a MDEvent to the box.
   * @param event :: reference to a MDEvent to add.
   * */
  TMDE(
  void MDBox)::addEvent( const MDE & event)
  {
    dataMutex.lock();
    this->data.push_back(event);

#ifdef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    // Keep the running total of signal and error
    double signal = event.getSignal();
    this->m_signal += signal;
    this->m_errorSquared += event.getErrorSquared();
#endif

#ifdef MDBOX_TRACKCENTROID_WHENADDING
    // Running total of the centroid
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] += event.getCenter(d) * signal;
#endif

    dataMutex.unlock();
  }

  //-----------------------------------------------------------------------------------------------
  /** Add several events. No bounds checking is made!
   *
   * @param events :: vector of events to be copied.
   * @param start_at :: index to start at in vector
   * @param stop_at :: index to stop at in vector (exclusive)
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  size_t MDBox)::addEvents(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at)
  {
    dataMutex.lock();
    typename std::vector<MDE>::const_iterator start = events.begin()+start_at;
    typename std::vector<MDE>::const_iterator end = events.begin()+stop_at;
    // Copy all the events
    this->data.insert(this->data.end(), start, end);

#ifdef MDBOX_TRACK_SIGNAL_WHEN_ADDING
    //Running total of signal/error
    for(typename std::vector<MDE>::const_iterator it = start; it != end; it++)
    {
      double signal = it->getSignal();
      this->m_signal += signal;
      this->m_errorSquared += it->getErrorSquared();

#ifdef MDBOX_TRACKCENTROID_WHENADDING
    // Running total of the centroid
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] += it->getCenter(d) * signal;
#endif
    }
#endif

    dataMutex.unlock();
    return 0;
  }

  //-----------------------------------------------------------------------------------------------
  /** Perform centerpoint binning of events.
   * @param bin :: MDBin object giving the limits of events to accept.
   * @param fullyContained :: optional bool array sized [nd] of which dimensions are known to be fully contained (for MDSplitBox)
   */
  TMDE(
  void MDBox)::centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const
  {
    if (fullyContained)
    {
      // For MDSplitBox, check if we've already found that all dimensions are fully contained
      size_t d;
      for (d=0; d<nd; ++d)
      {
        if (!fullyContained[d]) break;
      }
      if (d == nd)
      {
//        std::cout << "MDBox at depth " << this->m_depth << " was fully contained in bin " << bin.m_index << ".\n";
        // All dimensions are fully contained, so just return the cached total signal instead of counting.
        bin.m_signal += this->m_signal;
        bin.m_errorSquared += this->m_errorSquared;
        return;
      }
    }

    typename std::vector<MDE>::const_iterator it = data.begin();
    typename std::vector<MDE>::const_iterator it_end = data.end();

    // For each MDEvent
    for (; it != it_end; ++it)
    {
      // Go through each dimension
      size_t d;
      for (d=0; d<nd; ++d)
      {
        // Check that the value is within the bounds given. (Rotation is for later)
        coord_t x = it->getCenter(d);
        if (x < bin.m_min[d])
          break;
        if (x >= bin.m_max[d])
          break;
      }
      // If the loop reached the end, then it was all within bounds.
      if (d == nd)
      {
        // Accumulate error and signal
        bin.m_signal += it->getSignal();
        bin.m_errorSquared += it->getErrorSquared();
      }
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** General (non-axis-aligned) centerpoint binning method.
   * TODO: TEST THIS!
   *
   * @param bin :: a MDBin object giving the limits, aligned with the axes of the workspace,
   *        of where the non-aligned bin MIGHT be present.
   * @param function :: a ImplicitFunction that will evaluate true for any coordinate that is
   *        contained within the (non-axis-aligned) bin.
   */
  TMDE(
  void MDBox)::generalBin(MDBin<MDE,nd> & bin, Mantid::API::ImplicitFunction & function) const
  {
    UNUSED_ARG(bin);

    typename std::vector<MDE>::const_iterator it = data.begin();
    typename std::vector<MDE>::const_iterator it_end = data.end();
    bool mask[3] = {false, false, false}; //HACK
    // For each MDEvent
    for (; it != it_end; ++it)
    {
      if (function.evaluate(it->getCenter(), mask, 3)) //HACK
      {
        // Accumulate error and signal
        bin.m_signal += it->getSignal();
        bin.m_errorSquared += it->getErrorSquared();
      }
    }
  }


  /** Integrate the signal within a sphere; for example, to perform single-crystal
   * peak integration.
   * The CoordTransform object could be used for more complex shapes, e.g. "lentil" integration, as long
   * as it reduces the dimensions to a single value.
   *
   * @param radiusTransform :: nd-to-1 coordinate transformation that converts from these
   *        dimensions to the distance (squared) from the center of the sphere.
   * @param radiusSquared :: radius^2 below which to integrate
   * @param[out] signal :: set to the integrated signal
   * @param[out] errorSquared :: set to the integrated squared error.
   */
  TMDE(
  void MDBox)::integrateSphere(CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const
  {
    typename std::vector<MDE>::const_iterator it = data.begin();
    typename std::vector<MDE>::const_iterator it_end = data.end();

    // For each MDEvent
    for (; it != it_end; ++it)
    {
      coord_t out[nd];
      radiusTransform.apply(it->getCenter(), out);
      if (out[0] < radiusSquared)
      {
        signal += it->getSignal();
        errorSquared += it->getErrorSquared();
      }
    }
  }

  //-----------------------------------------------------------------------------------------------
  /** Find the centroid of all events contained within by doing a weighted average
   * of their coordinates.
   *
   * @param radiusTransform :: nd-to-1 coordinate transformation that converts from these
   *        dimensions to the distance (squared) from the center of the sphere.
   * @param radiusSquared :: radius^2 below which to integrate
   * @param[out] centroid :: array of size [nd]; its centroid will be added
   * @param[out] signal :: set to the integrated signal
   */
  TMDE(
  void MDBox)::centroidSphere(CoordTransform & radiusTransform, const coord_t radiusSquared, coord_t * centroid, signal_t & signal) const
  {
    typename std::vector<MDE>::const_iterator it = data.begin();
    typename std::vector<MDE>::const_iterator it_end = data.end();

    // For each MDEvent
    for (; it != it_end; ++it)
    {
      coord_t out[nd];
      radiusTransform.apply(it->getCenter(), out);
      if (out[0] < radiusSquared)
      {
        double eventSignal = it->getSignal();
        signal += eventSignal;
        for (size_t d=0; d<nd; d++)
          centroid[d] += it->getCenter(d) * eventSignal;
      }
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Save the box's Event data to an open nexus file.
   *
   * @param file :: Nexus File object, must already by opened with MDE::prepareNexusData()
   */
  TMDE(
  void MDBox)::saveNexus(::NeXus::File * file) const
  {
    MDE::saveVectorToNexusSlab(this->data, file, m_fileIndexStart);
  }


  //-----------------------------------------------------------------------------------------------
  /** Load the box's Event data from an open nexus file.
   * The FileIndex start and numEvents must be set correctly already.
   *
   * @param file :: Nexus File object, must already by opened with MDE::openNexusData()
   */
  TMDE(
  void MDBox)::loadNexus(::NeXus::File * file)
  {
    this->data.clear();
    MDE::loadVectorFromNexusSlab(this->data, file, m_fileIndexStart, m_fileNumEvents);
  }


}//namespace MDEvents

}//namespace Mantid

