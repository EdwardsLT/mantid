#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGINGFORMATSCONVERTQTWIDGET_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGINGFORMATSCONVERTQTWIDGET_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Tomography/IImagingFormatsConvertPresenter.h"
#include "MantidQtCustomInterfaces/Tomography/IImagingFormatsConvertView.h"

#include "ui_ImgFormatsConversion.h"

#include <QWidget>

// forward declarations for Qt
class QComboBox;

namespace MantidQt {
namespace CustomInterfaces {

/**
Qt view of a widget to convert images and stacks of images between
different formats.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_CUSTOMINTERFACES_DLL ImagingFormatsConvertQtWidget
    : public QWidget,
      public IImagingFormatsConvertView {
  Q_OBJECT

public:
  ImagingFormatsConvertQtWidget(QWidget *parent = 0);

  ~ImagingFormatsConvertQtWidget() override{};

  void userWarning(const std::string &err,
                   const std::string &description) override;

  void userError(const std::string &err,
                 const std::string &description) override;

  void setFormats(const std::vector<std::string> &fmts,
                  const std::vector<bool> &enable) override;

  std::string inputPath() const override;
  std::string inputFormatName() const override;

  std::string outputPath() const override;
  std::string outputFormatName() const override;

  bool compressHint() const override;

  size_t maxSearchDepth() const override;

  void convert(const std::string &inputName,
               const std::string &outputName) const override;

protected:
  void initLayout();

  void saveSettings() const override;
  void setup();

private slots:
  void browseImgInputConvertClicked();
  void browseImgOutputConvertClicked();
  void convertClicked();

private:
  void setFormatsCombo(QComboBox *cbox, const std::vector<std::string> &fmts,
                       const std::vector<bool> &enable);

  std::string
  grabUserBrowseDir(QLineEdit *le,
                    const std::string &userMsg = "Open directory/folder",
                    bool remember = true);
  std::string askImgOrStackPath();

  void closeEvent(QCloseEvent *event) override;

  Ui::ImgFormatsConversion m_ui;

  // path name for persistent settings
  const static std::string m_settingsGroup;

  // presenter as in the model-view-presenter
  std::unique_ptr<IImagingFormatsConvertPresenter> m_presenter;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_IMAGINGFORMATSCONVERTQTWIDGET_H_
