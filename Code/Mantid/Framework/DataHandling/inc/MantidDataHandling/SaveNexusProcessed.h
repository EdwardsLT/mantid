#ifndef MANTID_DATAHANDLING_SAVENEXUSPROCESSED_H_
#define MANTID_DATAHANDLING_SAVENEXUSPROCESSED_H_
/*WIKI* 


The algorithm SaveNexusProcessed will write a Nexus data file from the named workspace.
The file name can be an absolute or relative path and should have the extension
.nxs, .nx5 or .xml.
Warning - using XML format can be extremely slow for large data sets and generate very large files.
Both the extensions nxs and nx5 will generate HDF5 files.

The optional parameters can be used to control which spectra are saved into the file (not yet implemented).
If spectrum_min and spectrum_max are given, then only that range to data will be loaded.

A Mantid Nexus file may contain several workspace entries each labelled with an integer starting at 1.
If the file already contains n workspaces, the new one will be labelled n+1.


===Time series data===
TimeSeriesProperty data within the workspace will be saved as NXlog sections in  the Nexus file.
Only floating point logs are stored and loaded at present.

===Subalgorithms used===

None





*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidNexus/NexusFileIO.h"
#include <climits>

namespace Mantid
{
  namespace DataHandling
  {
    /** @class SaveNexusProcessed SaveNexusProcessed.h DataHandling/SaveNexusProcessed.h

    Saves a workspace as a Nexus Processed entry in a Nexus file. 
    SaveNexusProcessed is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init() & exec() methods.

    Required Properties:
    <UL>
    <LI> Filename - The name of and path to the output NeXus file (may exist) </LI>
    <LI> InputWorkspace - The name of the workspace to store the file </LI>
    <LI> Title - the title to describe the saved processed data
    </UL>

    Copyright &copy; 2007-2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport SaveNexusProcessed : public API::Algorithm
    {
    public:
      /// Default constructor
      SaveNexusProcessed();
      /// Destructor
      ~SaveNexusProcessed() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SaveNexusProcessed";};
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;};
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling";}

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();

      void getSpectrumList(std::vector<int> & spec, Mantid::API::MatrixWorkspace_const_sptr matrixWorkspace);

      template<class T>
      static void appendEventListData( std::vector<T> events, size_t offset, double * tofs, float * weights, float * errorSquareds, int64_t * pulsetimes);

      void execEvent(Mantid::NeXus::NexusFileIO * nexusFile,const bool uniformSpectra,const std::vector<int> spec);
	    /// sets non workspace properties for the algorithm
      void setOtherProperties(IAlgorithm* alg,const std::string & propertyName,const std::string &propertyValue,int perioidNum);

      /// The name and path of the input file
      std::string m_filename;
      /// The name and path of the input file
      std::string m_entryname;
      /// The title of the processed data section
      std::string m_title;
      /// Pointer to the local workspace
      API::MatrixWorkspace_const_sptr m_inputWorkspace;
      /// Pointer to the local workspace, cast to EventWorkspace
      DataObjects::EventWorkspace_const_sptr m_eventWorkspace;

      /// Progress bar
      API::Progress * prog;

    };

  } // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_SAVENEXUSPROCESSED_H_*/
