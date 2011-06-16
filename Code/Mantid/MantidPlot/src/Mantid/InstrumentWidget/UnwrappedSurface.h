#ifndef UNWRAPPEDSURFACE_H
#define UNWRAPPEDSURFACE_H

#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"
#include "MantidGeometry/IComponent.h"
#include "InstrumentActor.h"
#include "ProjectionSurface.h"
#include <boost/shared_ptr.hpp>

#include <QImage>
#include <QList>
#include <QStack>
#include <QSet>
#include <QMap>

namespace Mantid{
  namespace Geometry{
    class IDetector;
  }
}

class GLColor;
class QGLWidget;
class GL3DWidget;

/**
\class UnwrappedDetector
\brief Class helper for drawing detectors on unwraped surfaces
\date 15 Nov 2010
\author Roman Tolchenov, Tessella plc

This class keeps information used to draw a detector on an unwrapped cylindrical surface.

*/
class UnwrappedDetector
{
public:
  UnwrappedDetector(const unsigned char* c,
                       boost::shared_ptr<const Mantid::Geometry::IDetector> det
                       );
  unsigned char color[3]; ///< red, green, blue colour components (0 - 255)
  double u;      ///< horizontal "unwrapped" coordinate
  double v;      ///< vertical "unwrapped" coordinate
  double width;  ///< detector width in units of u
  double height; ///< detector height in units of v
  double uscale; ///< scaling factor in u direction
  double vscale; ///< scaling factor in v direction
  boost::shared_ptr<const Mantid::Geometry::IDetector> detector;
//  Mantid::Kernel::V3D minPoint,maxPoint;
};

/**
  * @class UnwrappedSurface
  * @brief Performs projection of an instrument onto a 2D surface and unwrapping it into a plane. Draws the resulting image
  *        on the screen.
  * @author Roman Tolchenov, Tessella plc
  * @date 18 Nov 2010
  */

class UnwrappedSurface: /*public DetectorCallback,*/ public ProjectionSurface
{
  //Q_OBJECT
public:
  UnwrappedSurface(const InstrumentActor* rootActor,const Mantid::Kernel::V3D& origin,const Mantid::Kernel::V3D& axis);
  ~UnwrappedSurface();
  void componentSelected(Mantid::Geometry::ComponentID = NULL);
  void getSelectedDetectors(QList<int>& dets);
  virtual QString getInfoText()const;

protected:
  virtual void drawSurface(MantidGLWidget* widget,bool picking = false)const;
  virtual void changeColorMap();

  virtual void mousePressEventMove(QMouseEvent*);
  virtual void mouseMoveEventMove(QMouseEvent*);
  virtual void mouseReleaseEventMove(QMouseEvent*);
  virtual void wheelEventMove(QWheelEvent*);

  /// calculate and assign udet.u and udet.v
  virtual void calcUV(UnwrappedDetector& udet) = 0;
  /// calculate rotation R for a udet
  virtual void calcRot(const UnwrappedDetector& udet, Mantid::Kernel::Quat& R)const = 0;
  virtual double uPeriod()const{return 0.0;}

  void init();
  void calcSize(UnwrappedDetector& udet,const Mantid::Kernel::V3D& X,
                const Mantid::Kernel::V3D& Y);
  //void callback(boost::shared_ptr<const Mantid::Geometry::IDetector> det,const DetectorCallbackData& data);
  void setColor(int index,bool picking)const;
  void showPickedDetector();
  void calcAssemblies(boost::shared_ptr<const Mantid::Geometry::IComponent> comp,const QRectF& compRect);
  void findAndCorrectUGap();

  const InstrumentActor* m_instrActor;
  double m_u_min;                      ///< Minimum u
  double m_u_max;                      ///< Maximum u
  double m_v_min;                      ///< Minimum v
  double m_v_max;                      ///< Maximum v
  double m_height_max;  ///< Maximum detector height
  double m_width_max;   ///< Maximum detector width
  QList<UnwrappedDetector> m_unwrappedDetectors;  ///< info needed to draw detectors onto unwrapped image
  QMap<Mantid::Geometry::ComponentID,QRectF> m_assemblies;

};

#endif // UNWRAPPEDSURFACE_H
