#
## Tests for Muon GUIs
##

set ( TEST_PY_FILES
   AxisChangerPresenter_test.py
   AxisChangerView_test.py
   FFTModel_test.py
   FFTPresenter_test.py
   load_run_widget/loadrun_model_test.py
   load_run_widget/loadrun_presenter_current_run_test.py
   load_run_widget/loadrun_presenter_single_file_test.py
   load_run_widget/loadrun_presenter_multiple_file_test.py
   load_file_widget/loadfile_model_test.py
   load_file_widget/loadfile_presenter_single_file_test.py
   load_file_widget/loadfile_presenter_multiple_file_test.py
   load_file_widget/loadfile_view_test.py
   load_run_widget/loadrun_presenter_increment_decrement_test.py
   load_run_widget/loadrun_view_test.py
   loading_tab/loadwidget_presenter_test.py
   loading_tab/loadwidget_presenter_multiple_file_test.py
   loading_tab/loadwidget_presenter_failure_test.py
   grouping_tab/grouping_table_presenter_test.py
   grouping_tab/pairing_table_presenter_test.py
   grouping_tab/pairing_table_group_selector_test.py
   grouping_tab/pairing_table_alpha_test.py
   LoadWidgetModel_test.py
   LoadWidgetPresenter_test.py
   LoadWidgetView_test.py
   MaxEntModel_test.py
   MaxEntPresenter_test.py
   PeriodicTableModel_test.py
   PeriodicTablePresenter_test.py
   PlottingPresenter_test.py
   PlottingUtils_test.py
   PlottingView_test.py
   transformWidget_test.py
   utilities/muon_group_test.py
   utilities/muon_pair_test.py
   utilities/load_utils_test.py
   utilities/thread_model_test.py
   utilities/muon_workspace_wrapper_test.py
   utilities/muon_workspace_wrapper_directory_test.py
   utilities/muon_load_data_test.py
   utilities/muon_file_utils_test.py
   utilities/run_string_utils_operator_test.py
   utilities/run_string_utils_conversion_test.py
   subplotObject_test.py
   help_widget_presenter_test.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test name=PythonAlgorithms
set ( PYUNITTEST_QT_API pyqt ) # force to use qt4
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} python.MuonQt4 ${TEST_PY_FILES} )
unset ( PYUNITTEST_QT_API )
