#include "MantidQtWidgets/MplCpp/MplFigureCanvas.h"
#include "MantidQtWidgets/MplCpp/NDArray1D.h"
#include "MantidQtWidgets/MplCpp/PythonErrors.h"
#include "MantidQtWidgets/MplCpp/SipUtils.h"
#include "MantidQtWidgets/Common/PythonThreading.h"

#include <QVBoxLayout>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
//------------------------------------------------------------------------------
// Static constants/functions
//------------------------------------------------------------------------------
#if QT_VERSION >= QT_VERSION_CHECK(4, 0, 0) &&                                 \
    QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
// Define PyQt version and matplotlib backend
const char *PYQT_MODULE = "PyQt4";
const char *MPL_QT_BACKEND = "matplotlib.backends.backend_qt4agg";

#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) &&                               \
    QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
// Define PyQt version and matplotlib backend
const char *PYQT_MODULE = "PyQt5";
const char *MPL_QT_BACKEND = "matplotlib.backends.backend_qt5agg";

#else
#error "Unknown Qt version. Cannot determine matplotlib backend."
#endif

// Return static instance of figure type
// The GIL must be held to call this
const PythonObject &mplFigureType() {
  static PythonObject figureType;
  if (figureType.isNone()) {
    figureType = getAttrOnModule("matplotlib.figure", "Figure");
  }
  return figureType;
}

// Return static instance of figure canvas type
// The GIL must be held to call this
const PythonObject &mplFigureCanvasType() {
  static PythonObject figureCanvasType;
  if (figureCanvasType.isNone()) {
    // Importing PyQt version first helps matplotlib select the correct backend.
    // We should do this in some kind of initialisation routine
    importModule(PYQT_MODULE);
    figureCanvasType = getAttrOnModule(MPL_QT_BACKEND, "FigureCanvasQTAgg");
  }
  return figureCanvasType;
}
}

//------------------------------------------------------------------------------
// MplFigureCanvas::PyObjectHolder - Private implementation
//------------------------------------------------------------------------------
struct MplFigureCanvas::PyObjectHolder {
  // QtAgg canvas object
  PythonObject canvas;
  // A pointer to the C++ data contained within the Python object
  QWidget *canvasWidget;
  // List of lines on current plot
  std::vector<PythonObject> lines;

  // constructor
  PyObjectHolder(int subplotLayout) {
    ScopedPythonGIL gil;
    // Create a figure and attach it to a canvas object. This creates a
    // blank widget
    auto figure = PythonObject::fromNewRef(
        PyObject_CallObject(mplFigureType().get(), NULL));
    // tight layout
    detail::decref(PyObject_CallMethod(figure.get(),
                                       PYSTR_LITERAL("set_tight_layout"),
                                       PYSTR_LITERAL("(i)"), 1));
    detail::decref(PyObject_CallMethod(figure.get(),
                                       PYSTR_LITERAL("add_subplot"),
                                       PYSTR_LITERAL("i"), subplotLayout));
    auto instance = PyObject_CallFunction(mplFigureCanvasType().get(),
                                          PYSTR_LITERAL("(O)"), figure.get());
    if (!instance) {
      throw PythonError();
    }
    canvas = PythonObject::fromNewRef(instance);
    canvasWidget = static_cast<QWidget *>(sipUnwrap(canvas.get()));
    assert(canvasWidget);
  }

  /**
   * Return the Axes object that is currently active. Analogous to figure.gca()
   * @return matplotlib.axes.Axes object
   */
  PythonObject gca() {
    ScopedPythonGIL gil;
    auto figure = PythonObject::fromNewRef(
        PyObject_GetAttrString(canvas.get(), PYSTR_LITERAL("figure")));
    return PythonObject::fromNewRef(PyObject_CallMethod(
        figure.get(), PYSTR_LITERAL("gca"), PYSTR_LITERAL(""), nullptr));
  }
};

//------------------------------------------------------------------------------
// MplFigureCanvas
//------------------------------------------------------------------------------
/**
 * @brief Constructs an empty plot widget with the given subplot layout.
 *
 * @param subplotLayout The sublayout geometry defined in matplotlib's
 * convenience format: [Default=111]. See
 * https://matplotlib.org/api/pyplot_api.html#matplotlib.pyplot.subplot
 *
 * @param parent A pointer to the parent widget, can be nullptr
 */
MplFigureCanvas::MplFigureCanvas(int subplotLayout, QWidget *parent)
    : QWidget(parent), m_pydata(nullptr) {
  m_pydata = new PyObjectHolder(subplotLayout);
  setLayout(new QVBoxLayout);
  layout()->addWidget(m_pydata->canvasWidget);
  m_pydata->canvasWidget->setMouseTracking(false);
}

/**
 * @brief Destroys the object
 */
MplFigureCanvas::~MplFigureCanvas() { delete m_pydata; }

/**
 * Get the "real" canvas widget. This is generally only required
 * when needing to install event filters to capture mouse events as
 * it's not possible to override the methods in Python from C++
 * @return The real canvas object
 */
QWidget *MplFigureCanvas::canvasWidget() const {
  return m_pydata->canvasWidget;
}

/**
 * Retrieve information about the subplot geometry
 * @return A SubPlotSpec object defining the geometry
 */
SubPlotSpec MplFigureCanvas::getGeometry() const {
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  auto geometry = PythonObject::fromNewRef(PyObject_CallMethod(
      axes.get(), PYSTR_LITERAL("get_geometry"), PYSTR_LITERAL(""), nullptr));

  return SubPlotSpec(TO_LONG(PyTuple_GET_ITEM(geometry.get(), 0)),
                     TO_LONG(PyTuple_GET_ITEM(geometry.get(), 1)));
}

/**
 * @return The number of Line2Ds on the canvas
 */
size_t MplFigureCanvas::nlines() const {
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  auto lines = PythonObject::fromNewRef(PyObject_CallMethod(
      axes.get(), PYSTR_LITERAL("get_lines"), PYSTR_LITERAL(""), nullptr));
  return static_cast<size_t>(PyList_Size(lines.get()));
}

/**
 * Get a label from the canvas
 * @param type The label type
 * @return The label on the requested axis
 */
QString MplFigureCanvas::getLabel(const Axes::Label type) const {
  const char *method;
  if (type == Axes::Label::X)
    method = "get_xlabel";
  else if (type == Axes::Label::Y)
    method = "get_ylabel";
  else if (type == Axes::Label::Title)
    method = "get_title";
  else
    throw std::logic_error("MplFigureCanvas::getLabel() - Unknown label type.");

  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  auto label = PythonObject::fromNewRef(PyObject_CallMethod(
      axes.get(), PYSTR_LITERAL(method), PYSTR_LITERAL(""), nullptr));
  return QString::fromAscii(TO_CSTRING(label.get()));
}

/**
 * Return the scale type on the given axis
 * @param type An enumeration giving the axis type
 * @return A string defining the scale type
 */
QString MplFigureCanvas::getScale(const Axes::Scale type) {
  const char *method;
  if (type == Axes::Scale::X)
    method = "get_xscale";
  else if (type == Axes::Scale::Y)
    method = "get_yscale";
  else
    throw std::logic_error(
        "MplFigureCanvas::getScale() - Scale type must be X or Y");
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  auto scale = PythonObject::fromNewRef(PyObject_CallMethod(
      axes.get(), PYSTR_LITERAL(method), PYSTR_LITERAL(""), nullptr));
  return QString::fromAscii(TO_CSTRING(scale.get()));
}

/**
 * @brief MplFigureCanvas::draw
 */
void MplFigureCanvas::draw() {
  ScopedPythonGIL gil;
  drawNoGIL();
}

/**
 * Equivalent of Figure.add_subplot. If the subplot already exists then
 * it simply sets that plot number to be active
 * @param subplotLayout Subplot geometry in matplotlib convenience format,
 * e.g 2,1,2 would stack 2 plots on top of each other and set the second
 * to active
 */
void MplFigureCanvas::addSubPlot(int subplotLayout) {
  ScopedPythonGIL gil;
  auto figure = PythonObject::fromNewRef(
      PyObject_GetAttrString(m_pydata->canvas.get(), PYSTR_LITERAL("figure")));
  auto result = PyObject_CallMethod(figure.get(), PYSTR_LITERAL("add_subplot"),
                                    PYSTR_LITERAL("(i)"), subplotLayout);
  if (!result)
    throw PythonError();
  detail::decref(result);
}

/**
 * Set the color of the given line
 * @param index The index of an existing line. If it does not exist then
 * this is a no-op.
 * @param color A string indicating a matplotlib color
 */
void MplFigureCanvas::setLineColor(const size_t index, const char *color) {
  auto &lines = m_pydata->lines;
  if (lines.empty() || index >= lines.size())
    return;
  ScopedPythonGIL gil;
  auto &line = lines[index];
  detail::decref(PyObject_CallMethod(line.get(), PYSTR_LITERAL("set_color"),
                                     PYSTR_LITERAL("(s)"), color));
}

/**
 * Plot lines to the current axis
 * @param x A container of X points. Requires support for forward iteration.
 * @param y A container of Y points. Requires support for forward iteration.
 * @param format A format string for the line/markers
 */
template <typename XArrayType, typename YArrayType>
void MplFigureCanvas::plotLine(const XArrayType &x, const YArrayType &y,
                               const char *format) {
  ScopedPythonGIL gil;
  NDArray1D xnp(x), ynp(y);
  auto axes = m_pydata->gca();
  // This will return a list of lines but we know we are only plotting 1
  auto lines =
      PyObject_CallMethod(axes.get(), PYSTR_LITERAL("plot"),
                          PYSTR_LITERAL("(OOs)"), xnp.get(), ynp.get(), format);
  if (!lines) {
    throw PythonError();
  }
  m_pydata->lines.emplace_back(
      PythonObject::fromBorrowedRef(PyList_GET_ITEM(lines, 0)));
  detail::decref(lines);
}

/**
 * Remove a line from the canvas based on the index
 * @param index The index of the line to remove. If it does not exist then
 * this is a no-op.
 */
void MplFigureCanvas::removeLine(const size_t index) {
  auto &lines = m_pydata->lines;
  if (lines.empty() || index >= lines.size())
    return;
  auto posIter = std::next(std::begin(lines), index);
  auto line = *posIter;
  // .erase will cause a decrement of the reference count so we should
  // really hold the GIL at that point
  ScopedPythonGIL gil;
  lines.erase(posIter);
  detail::decref(PyObject_CallMethod(line.get(), PYSTR_LITERAL("remove"),
                                     PYSTR_LITERAL(""), nullptr));
}

/**
 * Clear the current axes of artists
 */
void MplFigureCanvas::clearLines() {
  if (m_pydata->lines.empty())
    return;
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  detail::decref(PyObject_CallMethod(axes.get(), PYSTR_LITERAL("cla"),
                                     PYSTR_LITERAL(""), nullptr));
  m_pydata->lines.clear();
}

/**
 * Set a label on the requested axis
 * @param type Type of label
 * @param label Label for the axis
 */
void MplFigureCanvas::setLabel(const Axes::Label type, const char *label) {
  const char *method;
  if (type == Axes::Label::X)
    method = "set_xlabel";
  else if (type == Axes::Label::Y)
    method = "set_ylabel";
  else if (type == Axes::Label::Title)
    method = "set_title";
  else
    throw std::logic_error("MplFigureCanvas::setLabel() - Unknown label type.");

  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  auto result = PyObject_CallMethod(axes.get(), PYSTR_LITERAL(method),
                                    PYSTR_LITERAL("(s)"), label);
  if (!result)
    throw PythonError();
  detail::decref(result);
}

/**
 * Set the scale type on an axis. Redraw is called if requested
 * @param axis Enumeration defining the axis
 * @param scaleType The type of scale
 * @param redraw If true then call Axes.draw to repaint the canvas
 */
void MplFigureCanvas::setScale(const Axes::Scale axis, const char *scaleType,
                               bool redraw) {
  auto scaleSetter =
      [](const PythonObject &axes, const char *method, const char *value) {
        auto result = PyObject_CallMethod(axes.get(), PYSTR_LITERAL(method),
                                          PYSTR_LITERAL("(s)"), value);
        if (!result)
          throw PythonError();
        detail::decref(result);
      };
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  if (axis == Axes::Scale::Both || axis == Axes::Scale::X)
    scaleSetter(axes, "set_xscale", scaleType);
  if (axis == Axes::Scale::Both || axis == Axes::Scale::Y)
    scaleSetter(axes, "set_yscale", scaleType);

  if (redraw)
    drawNoGIL();
}

/**
 * Rescale the axis limits to the data
 * @param axis Choose the axis to rescale
 * @param redraw If true then call canvas->draw() to repaint the canvas
 */
void MplFigureCanvas::rescaleToData(const Axes::Scale axis, bool redraw) {
  int scaleX(0), scaleY(0);
  switch (axis) {
  case Axes::Scale::Both:
    scaleX = 1;
    scaleY = 1;
    break;
  case Axes::Scale::X:
    scaleX = 1;
    break;
  case Axes::Scale::Y:
    scaleY = 1;
    break;
  default:
    throw std::logic_error(
        "MplFigureCanvas::rescaleToData() - Unknown Axis type.");
  };
  ScopedPythonGIL gil;
  auto axes = m_pydata->gca();
  detail::decref(PyObject_CallMethod(axes.get(), PYSTR_LITERAL("relim"),
                                     PYSTR_LITERAL(""), nullptr));
  detail::decref(
      PyObject_CallMethod(axes.get(), PYSTR_LITERAL("autoscale_view"),
                          PYSTR_LITERAL("(iii)"), 1, scaleX, scaleY));
  if (redraw)
    drawNoGIL();
}

/**
 * The implementation of draw that does NOT lock the GIL. Use with caution.
 */
void MplFigureCanvas::drawNoGIL() {
  detail::decref(PyObject_CallMethod(m_pydata->canvas.get(),
                                     PYSTR_LITERAL("draw"), PYSTR_LITERAL(""),
                                     nullptr));
}

//------------------------------------------------------------------------------
// Explicit template instantations
//------------------------------------------------------------------------------
using VectorDouble = std::vector<double>;
template EXPORT_OPT_MANTIDQT_MPLCPP void
MplFigureCanvas::plotLine<VectorDouble, VectorDouble>(const VectorDouble &,
                                                      const VectorDouble &,
                                                      const char *);
}
}
}