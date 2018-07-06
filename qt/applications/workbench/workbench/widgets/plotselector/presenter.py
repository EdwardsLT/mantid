#  This file is part of the mantid workbench.
#
#  Copyright (C) 2018 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import absolute_import, print_function

from .model import PlotSelectorModel
from .view import PlotSelectorView


class PlotSelectorPresenter(object):
    """
    Presenter for the plot selector widget. This class can be
    responsible for the creation of the model and view, passing in
    the GlobalFigureManager as an argument, or the presenter and view
    can be passed as arguments (only intended for testing).
    """
    def __init__(self, global_figure_manager, view=None, model=None):
        """
        Initialise the presenter, creating the view and model, and
        setting the initial plot list
        :param global_figure_manager: The GlobalFigureManager class
        :param view: Optional - a view to use instead of letting the
                     class create one (intended for testing)
        :param model: Optional - a model to use instead of letting
                      the class create one (intended for testing)
        """
        # Create model and view, or accept mocked versions
        if view is None:
            self.widget = PlotSelectorView(self)
        else:
            self.widget = view
        if model is None:
            self.model = PlotSelectorModel(self, global_figure_manager)
        else:
            self.model = model

        # Make sure the plot list is up to date
        self.update_plot_list()

    def update_plot_list(self):
        """
        Updates the plot list in the model and the view. Filter text
        is applied to the updated selection if required.
        """
        self.model.update_plot_list()
        filter_text = self.widget.get_filter_text()
        if not filter_text:
            self.widget.set_plot_list(self.model.plot_list)
        else:
            self._filter_plot_list_by_string(filter_text)

    # ------------------------ Plot Closing -------------------------

    def close_action_called(self):
        """
        This is called by the view when closing plots is requested
        (e.g. pressing close or delete).
        """
        selected_plots = self.widget.get_all_selected_plot_names()
        self._close_plots(selected_plots)

    def _close_plots(self, list_of_plots):
        """
        Accepts a list of plot names to close
        :param list_of_plots: A list of strings containing plot names
        """
        for plot_name in list_of_plots:
            self.model.close_plot(plot_name)

    # ----------------------- Plot Filtering ------------------------

    def filter_text_changed(self):
        """
        Called by the view when the filter text is changed (e.g. by
        typing or clearing the text)
        """
        filter_text = self.widget.get_filter_text()
        self._filter_plot_list_by_string(filter_text)

    def _filter_plot_list_by_string(self, filter_text):
        """
        Given a string to filter on this updates the list of plots
        in the view or shows all plots if the string is empty
        :param filter_text: A string containing the filter text
        """
        if not filter_text:
            self.widget.set_plot_list(self.model.plot_list)
        else:
            filtered_plot_list = []
            for plot_name in self.model.plot_list:
                if filter_text.lower() in plot_name.lower():
                    filtered_plot_list.append(plot_name)
            self.widget.set_plot_list(filtered_plot_list)

    # ----------------------- Plot Selection ------------------------

    def list_double_clicked(self):
        """
        When a list item is double clicked the view calls this method
        to bring the selected plot to the front
        """
        plot_name = self.widget.get_currently_selected_plot_name()
        self.model.make_plot_active(plot_name)
