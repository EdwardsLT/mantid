# Create egg link to binary output directory
add_python_package ( workbench EXECUTABLE )

# Configure resources data in place for ease of development. The output
# file is added to the toplevel gitignore
set ( _qrc_file ${CMAKE_CURRENT_LIST_DIR}/resources.qrc )
set ( _output_res_py ${CMAKE_CURRENT_LIST_DIR}/workbench/app/resources.py )
add_custom_command ( OUTPUT ${_output_res_py}
  COMMAND ${PYRCC5_CMD} -o ${_output_res_py} ${_qrc_file}
  COMMENT "Generating workbench resources module"
  DEPENDS ${_qrc_file}
)
add_custom_target ( workbench_resources ALL DEPENDS ${_output_res_py} )

# Dependency chain
add_dependencies ( workbench workbench_resources mantidqt )

# ctest targets
set ( TEST_FILES
  workbench/config/test/test_user.py
  workbench/test/test_import.py

  workbench/plotting/test/test_figuremanager.py
  workbench/plotting/test/test_globalfiguremanager.py

  workbench/widgets/plotselector/test/test_plotselector_model.py
  workbench/widgets/plotselector/test/test_plotselector_presenter.py
  workbench/widgets/plotselector/test/test_plotselector_view.py
)

set ( WORKBENCH_TEST_PY_FILES
  workbench/test/test_fit_interactive_tool.py
)

set ( PYUNITTEST_RUN_SERIAL ON )
set ( PYUNITTEST_QT_API pyqt5)
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}
  workbench ${TEST_FILES}
)

if ( CMAKE_GENERATOR MATCHES "Visual Studio" OR CMAKE_GENERATOR MATCHES "Xcode" )
  set ( PYUNITTEST_RUNNER ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/workbench-script.pyw -x)
else()
  set ( PYUNITTEST_RUNNER ${CMAKE_BINARY_DIR}/bin/workbench -x)
endif()

if (NOT APPLE)
    # Tests fail/crash on OSX for unknown reasons. Skipping by the usual way
    # via @unittest.skipIf also doesn't work.
    pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}
      workbench_gui ${WORKBENCH_TEST_PY_FILES}
    )
endif()

# Install MantidWorkbench for OSX
if ( APPLE )

    configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/make_package.rb.in
        ${CMAKE_CURRENT_BINARY_DIR}/make_package.rb
        @ONLY )
    install ( FILES  ${CMAKE_CURRENT_BINARY_DIR}/make_package.rb DESTINATION MantidWorkbench.app/ )
    install ( CODE "
        set ( bundle \${CMAKE_INSTALL_PREFIX}/MantidWorkbench.app )
        execute_process(COMMAND chmod +x ./make_package.rb WORKING_DIRECTORY \${bundle})
        execute_process(COMMAND ./make_package.rb WORKING_DIRECTORY \${bundle} RESULT_VARIABLE install_name_tool_result)
        if(NOT install_name_tool_result EQUAL 0)
          message(FATAL_ERROR \"Package script failed!!!\n\")
        endif()
    ")

endif()
