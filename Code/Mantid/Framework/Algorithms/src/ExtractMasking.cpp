//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/ExtractMasking.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(ExtractMasking)
    
    /// Sets documentation strings for this algorithm
    void ExtractMasking::initDocs()
    {
      this->setWikiSummary("Extracts the masking from a given workspace and places it in a new workspace. ");
      this->setOptionalMessage("Extracts the masking from a given workspace and places it in a new workspace.");
    }
    

    using Kernel::Direction;
    using Geometry::IDetector_const_sptr;
    using namespace API;

    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------

    /**
     * Declare the algorithm properties
     */
    void ExtractMasking::init()
    {
      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","", Direction::Input),
          "A workspace whose masking is to be extracted"
      );
      declareProperty(
          new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","", Direction::Output),
          "A workspace containing the masked spectra as zeroes and ones.");
    }

    /**
     * Execute the algorithm
     */
    void ExtractMasking::exec()
    {
      MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

      const int nHist = static_cast<int>(inputWS->getNumberHistograms());
      const int xLength(1), yLength(1);
      // Create a new workspace for the results, copy from the input to ensure that we copy over the instrument and current masking
      MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS, nHist, xLength, yLength);

      Progress prog(this,0.0,1.0,nHist);
      MantidVecPtr xValues;
      xValues.access() = MantidVec(1, 0.0);

      PARALLEL_FOR2(inputWS, outputWS)
      for( int i = 0; i < nHist; ++i )
      {
        PARALLEL_START_INTERUPT_REGION
        // Spectrum in the output workspace
        ISpectrum * outSpec = outputWS->getSpectrum(i);
        // Spectrum in the input workspace
        const ISpectrum * inSpec = inputWS->getSpectrum(i);

        // Copy X, spectrum number and detector IDs
        outSpec->setX(xValues);
        outSpec->copyInfoFrom(*inSpec);

        IDetector_const_sptr inputDet;
        bool inputIsMasked(false);
        try
        {
          inputDet = inputWS->getDetector(i);
          if( inputDet->isMasked() )
          {
            inputIsMasked = true;
          }
        }
        catch(Kernel::Exception::NotFoundError &)
        {
          inputIsMasked = false;
        }

        if( inputIsMasked )
        {
          outSpec->dataY()[0] = 0.0;
          outSpec->dataE()[0] = 0.0;
        }
        else
        {
          outSpec->dataY()[0] = 1.0;
          outSpec->dataE()[0] = 1.0;
        }
        prog.report();

        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      setProperty("OutputWorkspace", outputWS);
    }
    
  }
}

