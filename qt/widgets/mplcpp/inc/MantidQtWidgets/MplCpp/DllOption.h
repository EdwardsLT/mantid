#ifndef MANTIDQT_WIDGETS_MPLCPP_DLLOPTION_H_
#define MANTIDQT_WIDGETS_MPLCPP_DLLOPTION_H_

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_MPLCPP
#define EXPORT_OPT_MANTIDQT_MPLCPP DLLExport
#define EXTERN_MANTIDQT_MPLCPP
#else
#define EXPORT_OPT_MANTIDQT_MPLCPP DLLImport
#define EXTERN_MANTIDQT_MPLCPP extern
#endif /* IN_MANTIDQT_MPLCPP */

#endif // MANTIDQT_WIDGETS_MPLCPP_DLLOPTION_H_