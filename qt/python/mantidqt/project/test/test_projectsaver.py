# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import unittest
import tempfile

import os
from shutil import rmtree

from mantid.api import AnalysisDataService as ADS
from mantid.simpleapi import CreateSampleWorkspace
from mantidqt.project import projectsaver


project_file_ext = ".mtdproj"
working_directory = tempfile.mkdtemp()


class ProjectSaverTest(unittest.TestCase):
    def tearDown(self):
        ADS.clear()

    def setUp(self):
        # In case it was hard killed and is still present
        if os.path.isdir(working_directory):
            rmtree(working_directory)

    def test_only_one_workspace_saving(self):
        ws1_name = "ws1"
        ADS.addOrReplace(ws1_name, CreateSampleWorkspace(OutputWorkspace=ws1_name))
        project_saver = projectsaver.ProjectSaver(project_file_ext)
        file_name = working_directory + "/" + os.path.basename(working_directory) + project_file_ext

        workspaces_string = "\"workspaces\": [\"ws1\"]"

        project_saver.save_project(workspace_to_save=[ws1_name], directory=working_directory)

        # Check project file is saved correctly
        f = open(file_name, "r")
        file_string = f.read()
        self.assertTrue(workspaces_string in file_string)

        # Check workspace is saved
        list_of_files = os.listdir(working_directory)
        self.assertEqual(len(list_of_files), 2)
        self.assertTrue(os.path.basename(working_directory) + project_file_ext in list_of_files)
        self.assertTrue(ws1_name + ".nxs" in list_of_files)

    def test_only_multiple_workspaces_saving(self):
        ws1_name = "ws1"
        ws2_name = "ws2"
        ws3_name = "ws3"
        ws4_name = "ws4"
        ws5_name = "ws5"
        CreateSampleWorkspace(OutputWorkspace=ws1_name)
        CreateSampleWorkspace(OutputWorkspace=ws2_name)
        CreateSampleWorkspace(OutputWorkspace=ws3_name)
        CreateSampleWorkspace(OutputWorkspace=ws4_name)
        CreateSampleWorkspace(OutputWorkspace=ws5_name)
        project_saver = projectsaver.ProjectSaver(project_file_ext)
        file_name = working_directory + "/" + os.path.basename(working_directory) + project_file_ext

        workspaces_string = "\"workspaces\": [\"ws1\", \"ws2\", \"ws3\", \"ws4\", \"ws5\"]"
        project_saver.save_project(workspace_to_save=[ws1_name, ws2_name, ws3_name, ws4_name, ws5_name],
                                   directory=working_directory)

        # Check project file is saved correctly
        f = open(file_name, "r")
        file_string = f.read()
        self.assertTrue(workspaces_string in file_string)

        # Check workspace is saved
        list_of_files = os.listdir(working_directory)
        self.assertEqual(len(list_of_files), 6)
        self.assertTrue(os.path.basename(working_directory) + project_file_ext in list_of_files)
        self.assertTrue(ws1_name + ".nxs" in list_of_files)
        self.assertTrue(ws2_name + ".nxs" in list_of_files)
        self.assertTrue(ws3_name + ".nxs" in list_of_files)
        self.assertTrue(ws4_name + ".nxs" in list_of_files)
        self.assertTrue(ws5_name + ".nxs" in list_of_files)

    def test_only_saving_one_workspace_when_multiple_are_present_in_the_ADS(self):
        ws1_name = "ws1"
        ws2_name = "ws2"
        ws3_name = "ws3"
        CreateSampleWorkspace(OutputWorkspace=ws1_name)
        CreateSampleWorkspace(OutputWorkspace=ws2_name)
        CreateSampleWorkspace(OutputWorkspace=ws3_name)
        project_saver = projectsaver.ProjectSaver(project_file_ext)
        file_name = working_directory + "/" + os.path.basename(working_directory) + project_file_ext
        workspaces_string = "\"workspaces\": [\"ws1\"]"
        project_saver.save_project(workspace_to_save=[ws1_name], directory=working_directory)

        # Check project file is saved correctly
        f = open(file_name, "r")
        file_string = f.read()
        self.assertTrue(workspaces_string in file_string)

        # Check workspace is saved
        list_of_files = os.listdir(working_directory)
        self.assertEqual(len(list_of_files), 2)
        self.assertTrue(os.path.basename(working_directory) + project_file_ext in list_of_files)
        self.assertTrue(ws1_name + ".nxs" in list_of_files)


class ProjectWriterTest(unittest.TestCase):
    def tearDown(self):
        ADS.clear()

    def setUp(self):
        # In case it was hard killed and is still present
        if os.path.isdir(working_directory):
            rmtree(working_directory)

    def test_write_out_empty_workspaces(self):
        workspace_list = []
        project_writer = projectsaver.ProjectWriter(working_directory, workspace_list, project_file_ext)
        file_name = working_directory + "/" + os.path.basename(working_directory) + project_file_ext

        workspaces_string = "\"workspaces\": []"

        project_writer.write_out()

        f = open(file_name, "r")
        file_string = f.read()
        self.assertTrue(workspaces_string in file_string)

    def test_write_out_on_just_workspaces(self):
        workspace_list = ["ws1", "ws2", "ws3", "ws4"]
        project_writer = projectsaver.ProjectWriter(working_directory, workspace_list, project_file_ext)
        file_name = working_directory + "/" + os.path.basename(working_directory) + project_file_ext

        workspaces_string = "\"workspaces\": [\"ws1\", \"ws2\", \"ws3\", \"ws4\"]"

        project_writer.write_out()
        f = open(file_name, "r")
        file_string = f.read()
        self.assertTrue(workspaces_string in file_string)


if __name__ == "__main__":
    unittest.main()
