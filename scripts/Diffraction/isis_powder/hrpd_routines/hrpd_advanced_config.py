from __future__ import (absolute_import, division, print_function)

from isis_powder.hrpd_routines.hrpd_enums import HRPD_TOF_WINDOWS

absorption_correction_params = {
    "cylinder_sample_height": 2.0,
    "cylinder_sample_radius": 0.3,
    "cylinder_position": [0., 0., 0.],
    "chemical_formula": "V"
}

# Default cropping values are 5% off each end

window_10_110_params = {
    "vanadium_tof_cropping": (1e4, 1.2e5),
    "focused_cropping_values": [
        (1.5e4, 1.08e5),  # Bank 1
        (1.5e4, 1.12e5),  # Bank 2
        (1.5e4, 1e5)   # Bank 3
    ]
}

window_30_130_params = {
    "vanadium_tof_cropping": (3e4, 1.4e5),
    "focused_cropping_values": [
        (3.5e4, 1.3e5),  # Bank 1
        (3.4e4, 1.4e5),  # Bank 2
        (3.3e4, 1.3e5)   # Bank 3
    ]
}

window_100_200_params = {
    "vanadium_tof_cropping": (1e5, 2.15e5),
    "focused_cropping_values": [
        (1e5, 2e5),      # Bank 1
        (8.7e4, 2.1e5),  # Bank 2
        (9.9e4, 2.1e5)   # Bank 3
    ]
}

file_names = {
    "grouping_file_name": "hrpd_new_072_01_corr.cal"
}

general_params = {
    "spline_coefficient": 70,
    "focused_bin_widths": [
        -0.0005,  # Bank 1
        -0.0005,  # Bank 2
        -0.001    # Bank 3
    ],
    "mode": "coupled"
}


def get_all_adv_variables(tof_window=HRPD_TOF_WINDOWS.window_10_110):
    advanced_config_dict = {}
    advanced_config_dict.update(file_names)
    advanced_config_dict.update(general_params)
    advanced_config_dict.update(get_tof_window_dict(tof_window=tof_window))
    return advanced_config_dict


def get_tof_window_dict(tof_window):
    if tof_window == HRPD_TOF_WINDOWS.window_10_110:
        return window_10_110_params
    if tof_window == HRPD_TOF_WINDOWS.window_30_130:
        return window_30_130_params
    if tof_window == HRPD_TOF_WINDOWS.window_100_200:
        return window_100_200_params
    raise RuntimeError("Invalid time-of-flight window: {}".format(tof_window))