#include "viewport.h"

#include "CS248.h"

namespace CS248 {

void ViewportImp::set_viewbox( float x, float y, float span ) {

  // Task 3 (part 2): 
  // Set svg to normalized device coordinate transformation. 
  // Your input arguments are defined as SVG canvans coordinates.

  this->x = x;
  this->y = y;
  this->span = span;

  Matrix3x3 ctn;
  // i = 0.5 + (0.5/span) *  (i - x)
  // i = i * 0.5/span + (0.5 - x*0.5/span)
  ctn[0] = Vector3D(0.5/span, 0.0, 0.0);
  ctn[1] = Vector3D(0.0, 0.5 / span, 0.0);
  ctn[2] = Vector3D((0.5 - x * 0.5 / span), (0.5 - y * 0.5 / span), 1.0);
  set_canvas_to_norm(ctn);

}

void ViewportImp::update_viewbox( float dx, float dy, float scale ) { 
  
  this->x -= dx;
  this->y -= dy;
  this->span *= scale;
  set_viewbox( x, y, span );
}

} // namespace CS248
