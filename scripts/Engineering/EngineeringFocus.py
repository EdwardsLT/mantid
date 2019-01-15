# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as simple
import os

banks = 2
run_number = "301567"
calibration_directory = "/home/sjenkins/user/{0}/EnginX_Mantid/Calibration"
focus_directory = "/home/sjenkins/user/{0}/EnginX_Mantid/Focus"
user = "301566"
van_run = "236516"
van_name = "ENGINX" + ("0" * (8 - len(van_run))) + van_run
calibration_directory = calibration_directory.format(user)
focus_drectory = focus_directory.format(user)
ws_to_focus = simple.Load(Filename="ENGINX" + run_number, OutputWorkspace="engg_focus_input")
van_integrated_ws = simple.Load(Filename=os.join(calibration_directory,
                                                 (van_name + "_precalculated_vanadium_run_integration.nxs")))
van_curves_ws = simple.Load(Filename=os.join(calibration_directory,
                                             (van_name + "_precalculated_vanadium_run_bank_curves.nxs")))
for i in range(1, banks + 1):
    simple.EnggFocus(InputWorkspace=ws_to_focus, OutputWorkspace="engg_focus_output_bank_1",
                     VanIntegrationWorkspace=van_integrated_ws, VanCurvesWorkspace=van_curves_ws,
                     Bank=str(i), NormaliseByCurrent=1)
print("done")