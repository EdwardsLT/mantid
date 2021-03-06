from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.muon_context import MuonContext
from mantid.api import ITableWorkspace, WorkspaceGroup
from mantid import api


class InstrumentWidgetModel(object):
    """
    The model holds the muon context and interacts with it, only able to modify the pre-processing parts of each
    run.

    The model should not take care of processing data, it should only interact with and modify the muon context data
    so that when processing is done from elsewhere the parameters of the pre-processing are up-to-date with the
    GUI.
    """

    def __init__(self, muon_data=MuonContext()):
        self._data = muon_data

    def clear_data(self):
        """When e.g. instrument changed"""
        self._data.clear()

    def get_file_time_zero(self):
        return self._data.loaded_data["TimeZero"]

    def get_user_time_zero(self):
        if "UserTimeZero" in self._data.loaded_data.keys():
            time_zero = self._data.loaded_data["UserTimeZero"]
        else:
            # default to loaded value, keep a record of the data vaue
            self._data.loaded_data["DataTimeZero"] = self._data.loaded_data["TimeZero"]
            time_zero = self._data.loaded_data["TimeZero"]
        return time_zero

    def get_file_first_good_data(self):
        return self._data.loaded_data["FirstGoodData"]

    def get_user_first_good_data(self):
        if "UserFirstGoodData" in self._data.loaded_data.keys():
            first_good_data = self._data.loaded_data["UserFirstGoodData"]
        else:
            # Default to loaded value
            self._data.loaded_data["FirstGoodData"] = self._data.loaded_data["FirstGoodData"]
            first_good_data = self._data.loaded_data["FirstGoodData"]
        return first_good_data

    def set_user_time_zero(self, time_zero):
        self._data.loaded_data["UserTimeZero"] = time_zero

    def set_user_first_good_data(self, first_good_data):
        self._data.loaded_data["UserFirstGoodData"] = first_good_data

    def get_dead_time_table_from_data(self):
        if self._data.is_multi_period():
            return self._data.loaded_data["DataDeadTimeTable"][0]
        else:
            return self._data.loaded_data["DataDeadTimeTable"]

    def get_dead_time_table(self):
        return self._data.dead_time_table

    def add_fixed_binning(self, fixed_bin_size):
        self._data.loaded_data["Rebin"] = str(fixed_bin_size)

    def add_variable_binning(self, rebin_params):
        self._data.loaded_data["Rebin"] = str(rebin_params)

    # ------------------------------------------------------------------------------------------------------------------
    # Dead Time
    # ------------------------------------------------------------------------------------------------------------------

    def load_dead_time(self):
        # TODO : Create this function
        pass

    def check_dead_time_file_selection(self, selection):
        try:
            table = api.AnalysisDataServiceImpl.Instance().retrieve(str(selection))
        except Exception:
            raise ValueError("Workspace " + str(selection) + " does not exist")
        assert isinstance(table, ITableWorkspace)
        # are column names correct?
        col = table.getColumnNames()
        if len(col) != 2:
            raise ValueError("Expected 2 columns, found ", str(max(0, len(col))))
        if col[0] != "spectrum" or col[1] != "dead-time":
            raise ValueError("Columns have incorrect names")
        rows = table.rowCount()
        if rows != self._data.loaded_workspace.getNumberHistograms():
            raise ValueError("Number of histograms do not match number of rows in dead time table")
        return True

    def set_dead_time_to_none(self):
        self._data.loaded_data["DeadTimeTable"] = None

    def set_dead_time_from_data(self):
        data_dead_time = self._data.loaded_data["DataDeadTimeTable"]
        if isinstance(data_dead_time, WorkspaceGroup):
            self._data.loaded_data["DeadTimeTable"] = data_dead_time[0]
        else:
            self._data.loaded_data["DeadTimeTable"] = data_dead_time

    def set_user_dead_time_from_ADS(self, name):
        dtc = api.AnalysisDataServiceImpl.Instance().retrieve(str(name))
        self._data.loaded_data["UserDeadTimeTable"] = dtc
        self._data.loaded_data["DeadTimeTable"] = dtc
