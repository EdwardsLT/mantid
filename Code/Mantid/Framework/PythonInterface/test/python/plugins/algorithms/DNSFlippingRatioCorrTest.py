import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
import numpy as np
import mantid.simpleapi as api
from mantid.simpleapi import DNSFlippingRatioCorr


class DNSFlippingRatioCorrTest(unittest.TestCase):
    workspaces = []
    __sf_bkgrws = None
    __nsf_bkgrws = None
    __sf_nicrws = None
    __nsf_nicrws = None

    def _create_fake_workspace(self, wsname, dataY, flipper):
        """
        creates DNS workspace with fake data
        """
        ndet = 24
        dataX = np.zeros(2*ndet)
        dataX.fill(4.2 + 0.00001)
        dataX[::2] -= 0.000002
        dataE = np.sqrt(dataY)
        # create workspace
        api.CreateWorkspace(OutputWorkspace=wsname, DataX=dataX, DataY=dataY,
                            DataE=dataE, NSpec=ndet, UnitX="Wavelength")
        outws = api.mtd[wsname]
        api.LoadInstrument(outws, InstrumentName='DNS')
        p_names = 'deterota,wavelength,slit_i_left_blade_position,slit_i_right_blade_position,\
            slit_i_lower_blade_position,slit_i_upper_blade_position,polarisation,flipper'
        p_values = '-7.53,4.2,10,10,5,20,x,' + flipper
        api.AddSampleLogMultiple(Workspace=outws, LogNames=p_names, LogValues=p_values, ParseType=True)
        # create the normalization workspace
        dataY.fill(1.0)
        dataE.fill(1.0)
        api.CreateWorkspace(OutputWorkspace=wsname + '_NORM', DataX=dataX, DataY=dataY,
                            DataE=dataE, NSpec=ndet, UnitX="Wavelength")
        normws = api.mtd[wsname + '_NORM']
        api.LoadInstrument(normws, InstrumentName='DNS')
        api.AddSampleLogMultiple(Workspace=normws, LogNames=p_names, LogValues=p_values, ParseType=True)

        return outws

    def setUp(self):
        dataY = np.array([2997., 2470., 2110., 1818., 840., 1095., 944., 720., 698., 699., 745., 690.,
                          977., 913., 906., 1007., 1067., 1119., 1467., 1542., 2316., 1536., 1593., 1646.])
        self.__sf_bkgrws = self._create_fake_workspace('__sf_bkgrws', dataY/620.0, flipper='ON')
        self.workspaces.append('__sf_bkgrws')
        dataY = np.array([14198., 7839., 4386., 3290., 1334., 1708., 1354., 1026., 958., 953., 888., 847.,
                          1042., 1049., 1012., 1116., 1294., 1290., 1834., 1841., 2740., 1750., 1965., 1860.])
        self.__nsf_bkgrws = self._create_fake_workspace('__nsf_bkgrws', dataY/619.0, flipper='OFF')
        self.workspaces.append('__nsf_bkgrws')
        dataY = np.array([5737., 5761., 5857., 5571., 4722., 5102., 4841., 4768., 5309., 5883., 5181., 4455.,
                          4341., 4984., 3365., 4885., 4439., 4103., 4794., 14760., 10516., 4445., 5460., 3942.])
        self.__nsf_nicrws = self._create_fake_workspace('__nsf_nicrws', dataY/58.0, flipper='OFF')
        self.workspaces.append('__nsf_nicrws')
        dataY = np.array([2343., 2270., 2125., 2254., 1534., 1863., 1844., 1759., 1836., 2030., 1848., 1650.,
                          1555., 1677., 1302., 1750., 1822., 1663., 2005., 4025., 3187., 1935., 2331., 2125.])
        self.__sf_nicrws = self._create_fake_workspace('__sf_nicrws', dataY/295.0, flipper='ON')
        self.workspaces.append('__sf_nicrws')

    def tearDown(self):
        for wsname in self.workspaces:
            if api.AnalysisDataService.doesExist(wsname + '_NORM'):
                api.DeleteWorkspace(wsname + '_NORM')
            if api.AnalysisDataService.doesExist(wsname):
                api.DeleteWorkspace(wsname)
        self.workspaces = []

    def test_DNSNormWorkspaceExists(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test1"
        api.DeleteWorkspace(self.__sf_bkgrws.getName() + '_NORM')
        self.assertRaises(RuntimeError, DNSFlippingRatioCorr, SFDataWorkspace=self.__sf_nicrws.getName(),
                          NSFDataWorkspace=self.__nsf_nicrws.getName(), SFNiCrWorkspace=self.__sf_nicrws.getName(),
                          NSFNiCrWorkspace=self.__nsf_nicrws.getName(), SFBkgrWorkspace=self.__sf_bkgrws.getName(),
                          NSFBkgrWorkspace=self.__nsf_bkgrws.getName(), SFOutputWorkspace=outputWorkspaceName+'SF',
                          NSFOutputWorkspace=outputWorkspaceName+'NSF')
        return

    def test_DNSFlipperValid(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test2"
        self.assertRaises(RuntimeError, DNSFlippingRatioCorr, SFDataWorkspace=self.__nsf_nicrws.getName(),
                          NSFDataWorkspace=self.__nsf_nicrws.getName(), SFNiCrWorkspace=self.__sf_nicrws.getName(),
                          NSFNiCrWorkspace=self.__nsf_nicrws.getName(), SFBkgrWorkspace=self.__sf_bkgrws.getName(),
                          NSFBkgrWorkspace=self.__nsf_bkgrws.getName(), SFOutputWorkspace=outputWorkspaceName+'SF',
                          NSFOutputWorkspace=outputWorkspaceName+'NSF')
        return

    def test_DNSPolarisationValid(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test3"
        api.AddSampleLog(Workspace=self.__nsf_nicrws, LogName='polarisation', LogText='y', LogType='String')
        self.assertRaises(RuntimeError, DNSFlippingRatioCorr, SFDataWorkspace=self.__sf_nicrws.getName(),
                          NSFDataWorkspace=self.__nsf_nicrws.getName(), SFNiCrWorkspace=self.__sf_nicrws.getName(),
                          NSFNiCrWorkspace=self.__nsf_nicrws.getName(), SFBkgrWorkspace=self.__sf_bkgrws.getName(),
                          NSFBkgrWorkspace=self.__nsf_bkgrws.getName(), SFOutputWorkspace=outputWorkspaceName+'SF',
                          NSFOutputWorkspace=outputWorkspaceName+'NSF')
        return

    def test_DNSFRSelfCorrection(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test4"
        # consider normalization=1.0 as set in self._create_fake_workspace
        dataws_sf = self.__sf_nicrws - self.__sf_bkgrws
        dataws_nsf = self.__nsf_nicrws - self.__nsf_bkgrws
        alg_test = run_algorithm("DNSFlippingRatioCorr", SFDataWorkspace=dataws_sf,
                                 NSFDataWorkspace=dataws_nsf, SFNiCrWorkspace=self.__sf_nicrws.getName(),
                                 NSFNiCrWorkspace=self.__nsf_nicrws.getName(), SFBkgrWorkspace=self.__sf_bkgrws.getName(),
                                 NSFBkgrWorkspace=self.__nsf_bkgrws.getName(), SFOutputWorkspace=outputWorkspaceName+'SF',
                                 NSFOutputWorkspace=outputWorkspaceName+'NSF', DoubleSpinFlipScatteringProbability=0.0)

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws_sf = AnalysisDataService.retrieve(outputWorkspaceName + 'SF')
        ws_nsf = AnalysisDataService.retrieve(outputWorkspaceName + 'NSF')
        # dimensions
        self.assertEqual(24, ws_sf.getNumberHistograms())
        self.assertEqual(24, ws_nsf.getNumberHistograms())
        self.assertEqual(2,  ws_sf.getNumDims())
        self.assertEqual(2,  ws_nsf.getNumDims())
        # data array: spin-flip must be zero
        for i in range(24):
            self.assertAlmostEqual(0.0, ws_sf.readY(i))
        # data array: non spin-flip must be nsf - sf^2/nsf
        nsf = np.array(dataws_nsf.extractY())
        sf = np.array(dataws_sf.extractY())
        refdata = nsf - sf*sf/nsf
        for i in range(24):
            self.assertAlmostEqual(refdata[i], ws_nsf.readY(i))

        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + 'SF')
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + 'NSF')
        run_algorithm("DeleteWorkspace", Workspace=dataws_sf)
        run_algorithm("DeleteWorkspace", Workspace=dataws_nsf)
        return

    def test_DNSTwoTheta(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test5"
        # rotate detector bank to different angles
        dataws_sf = self.__sf_nicrws - self.__sf_bkgrws
        dataws_nsf = self.__nsf_nicrws - self.__nsf_bkgrws
        api.RotateInstrumentComponent(dataws_sf, "bank0", X=0, Y=1, Z=0, Angle=-7.53)
        api.RotateInstrumentComponent(dataws_nsf, "bank0", X=0, Y=1, Z=0, Angle=-7.53)
        api.RotateInstrumentComponent(self.__sf_nicrws, "bank0", X=0, Y=1, Z=0, Angle=-8.02)
        api.RotateInstrumentComponent(self.__nsf_nicrws, "bank0", X=0, Y=1, Z=0, Angle=-8.02)
        api.RotateInstrumentComponent(self.__sf_bkgrws, "bank0", X=0, Y=1, Z=0, Angle=-8.54)
        api.RotateInstrumentComponent(self.__nsf_bkgrws, "bank0", X=0, Y=1, Z=0, Angle=-8.54)
        # apply correction
        alg_test = run_algorithm("DNSFlippingRatioCorr", SFDataWorkspace=dataws_sf,
                                 NSFDataWorkspace=dataws_nsf, SFNiCrWorkspace=self.__sf_nicrws.getName(),
                                 NSFNiCrWorkspace=self.__nsf_nicrws.getName(), SFBkgrWorkspace=self.__sf_bkgrws.getName(),
                                 NSFBkgrWorkspace=self.__nsf_bkgrws.getName(), SFOutputWorkspace=outputWorkspaceName+'SF',
                                 NSFOutputWorkspace=outputWorkspaceName+'NSF', DoubleSpinFlipScatteringProbability=0.0)

        self.assertTrue(alg_test.isExecuted())
        ws_sf = AnalysisDataService.retrieve(outputWorkspaceName + 'SF')
        ws_nsf = AnalysisDataService.retrieve(outputWorkspaceName + 'NSF')
        # dimensions
        self.assertEqual(24, ws_sf.getNumberHistograms())
        self.assertEqual(24, ws_nsf.getNumberHistograms())
        self.assertEqual(2,  ws_sf.getNumDims())
        self.assertEqual(2,  ws_nsf.getNumDims())
        # 2theta angles must not change after correction has been applied
        tthetas = np.array([7.53 + i*5 for i in range(24)])
        for i in range(24):
            det = ws_sf.getDetector(i)
            self.assertAlmostEqual(tthetas[i], np.degrees(ws_sf.detectorSignedTwoTheta(det)))
            det = ws_nsf.getDetector(i)
            self.assertAlmostEqual(tthetas[i], np.degrees(ws_nsf.detectorSignedTwoTheta(det)))

        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + 'SF')
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + 'NSF')
        run_algorithm("DeleteWorkspace", Workspace=dataws_sf)
        run_algorithm("DeleteWorkspace", Workspace=dataws_nsf)

        return

    def test_DNSFRVanaCorrection(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test6"
        # create fake vanadium data workspaces
        dataY = np.array([1811., 2407., 3558., 3658., 3352., 2321., 2240., 2617., 3245., 3340., 3338., 3310.,
                          2744., 3212., 1998., 2754., 2791., 2509., 3045., 3429., 3231., 2668., 3373., 2227.])
        __sf_vanaws = self._create_fake_workspace('__sf_vanaws', dataY/58.0, flipper='ON')
        self.workspaces.append('__sf_vanaws')
        dataY = np.array([2050., 1910., 2295., 2236., 1965., 1393., 1402., 1589., 1902., 1972., 2091., 1957.,
                          1593., 1952., 1232., 1720., 1689., 1568., 1906., 2001., 2051., 1687., 1975., 1456.])
        __nsf_vanaws = self._create_fake_workspace('__nsf_vanaws', dataY/58.0, flipper='OFF')
        self.workspaces.append('__nsf_vanaws')
        # consider normalization=1.0 as set in self._create_fake_workspace
        dataws_sf = __sf_vanaws - self.__sf_bkgrws
        dataws_nsf = __nsf_vanaws - self.__nsf_bkgrws
        alg_test = run_algorithm("DNSFlippingRatioCorr", SFDataWorkspace=dataws_sf,
                                 NSFDataWorkspace=dataws_nsf, SFNiCrWorkspace=self.__sf_nicrws.getName(),
                                 NSFNiCrWorkspace=self.__nsf_nicrws.getName(), SFBkgrWorkspace=self.__sf_bkgrws.getName(),
                                 NSFBkgrWorkspace=self.__nsf_bkgrws.getName(), SFOutputWorkspace=outputWorkspaceName+'SF',
                                 NSFOutputWorkspace=outputWorkspaceName+'NSF', DoubleSpinFlipScatteringProbability=0.0)

        self.assertTrue(alg_test.isExecuted())
        # check whether the data are correct
        ws_sf = AnalysisDataService.retrieve(outputWorkspaceName + 'SF')
        ws_nsf = AnalysisDataService.retrieve(outputWorkspaceName + 'NSF')
        # dimensions
        self.assertEqual(24, ws_sf.getNumberHistograms())
        self.assertEqual(24, ws_nsf.getNumberHistograms())
        self.assertEqual(2,  ws_sf.getNumDims())
        self.assertEqual(2,  ws_nsf.getNumDims())
        # data array: for vanadium ratio sf/nsf must be around 2
        ws = ws_sf/ws_nsf
        for i in range(24):
            self.assertAlmostEqual(2.0, np.around(ws.readY(i)))

        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + 'SF')
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + 'NSF')
        run_algorithm("DeleteWorkspace", Workspace=dataws_sf)
        run_algorithm("DeleteWorkspace", Workspace=dataws_nsf)
        run_algorithm("DeleteWorkspace", Workspace=ws)
        return

    def test_DNSDoubleSpinFlip(self):
        outputWorkspaceName = "DNSFlippingRatioCorrTest_Test7"
        f = 0.2
        # create fake vanadium data workspaces
        dataY = np.array([1811., 2407., 3558., 3658., 3352., 2321., 2240., 2617., 3245., 3340., 3338., 3310.,
                          2744., 3212., 1998., 2754., 2791., 2509., 3045., 3429., 3231., 2668., 3373., 2227.])
        __sf_vanaws = self._create_fake_workspace('__sf_vanaws', dataY/58.0, flipper='ON')
        self.workspaces.append('__sf_vanaws')
        dataY = np.array([2050., 1910., 2295., 2236., 1965., 1393., 1402., 1589., 1902., 1972., 2091., 1957.,
                          1593., 1952., 1232., 1720., 1689., 1568., 1906., 2001., 2051., 1687., 1975., 1456.])
        __nsf_vanaws = self._create_fake_workspace('__nsf_vanaws', dataY/58.0, flipper='OFF')
        self.workspaces.append('__nsf_vanaws')
        # consider normalization=1.0 as set in self._create_fake_workspace
        dataws_sf = __sf_vanaws - self.__sf_bkgrws
        dataws_nsf = __nsf_vanaws - self.__nsf_bkgrws
        alg_test = run_algorithm("DNSFlippingRatioCorr", SFDataWorkspace=dataws_sf,
                                 NSFDataWorkspace=dataws_nsf, SFNiCrWorkspace=self.__sf_nicrws.getName(),
                                 NSFNiCrWorkspace=self.__nsf_nicrws.getName(), SFBkgrWorkspace=self.__sf_bkgrws.getName(),
                                 NSFBkgrWorkspace=self.__nsf_bkgrws.getName(), SFOutputWorkspace=outputWorkspaceName+'SF',
                                 NSFOutputWorkspace=outputWorkspaceName+'NSF', DoubleSpinFlipScatteringProbability=0.0)

        self.assertTrue(alg_test.isExecuted())
        alg_test = run_algorithm("DNSFlippingRatioCorr", SFDataWorkspace=dataws_sf,
                                 NSFDataWorkspace=dataws_nsf, SFNiCrWorkspace=self.__sf_nicrws.getName(),
                                 NSFNiCrWorkspace=self.__nsf_nicrws.getName(), SFBkgrWorkspace=self.__sf_bkgrws.getName(),
                                 NSFBkgrWorkspace=self.__nsf_bkgrws.getName(), SFOutputWorkspace=outputWorkspaceName+'SF1',
                                 NSFOutputWorkspace=outputWorkspaceName+'NSF1', DoubleSpinFlipScatteringProbability=f)

        self.assertTrue(alg_test.isExecuted())

        # check whether the data are correct
        ws_sf = AnalysisDataService.retrieve(outputWorkspaceName + 'SF')
        ws_sf1 = AnalysisDataService.retrieve(outputWorkspaceName + 'SF1')
        ws_nsf = AnalysisDataService.retrieve(outputWorkspaceName + 'NSF')
        ws_nsf1 = AnalysisDataService.retrieve(outputWorkspaceName + 'NSF1')
        # dimensions
        self.assertEqual(24, ws_sf.getNumberHistograms())
        self.assertEqual(24, ws_sf1.getNumberHistograms())
        self.assertEqual(24, ws_nsf.getNumberHistograms())
        self.assertEqual(24, ws_nsf1.getNumberHistograms())
        self.assertEqual(2,  ws_sf.getNumDims())
        self.assertEqual(2,  ws_sf1.getNumDims())
        self.assertEqual(2,  ws_nsf.getNumDims())
        self.assertEqual(2,  ws_nsf1.getNumDims())
        # data array: sf must not change
        for i in range(24):
            self.assertAlmostEqual(ws_sf.readY(i), ws_sf1.readY(i))
        # data array: nsf1 = nsf - sf*f
        nsf = np.array(ws_nsf.extractY())
        sf = np.array(ws_sf.extractY())
        refdata = nsf - sf*f
        for i in range(24):
            self.assertAlmostEqual(refdata[i], ws_nsf1.readY(i))

        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + 'SF')
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + 'SF1')
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + 'NSF')
        run_algorithm("DeleteWorkspace", Workspace=outputWorkspaceName + 'NSF1')
        run_algorithm("DeleteWorkspace", Workspace=dataws_sf)
        run_algorithm("DeleteWorkspace", Workspace=dataws_nsf)
        return

if __name__ == '__main__':
    unittest.main()
