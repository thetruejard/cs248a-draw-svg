#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CS248 {


  // Added as a static variable because software_renderer cannot be modified
  // or else precompiled binary (SoftwareRendererRef) breaks
  static unsigned char* ssaa_buffer = nullptr;
  
  // Diddo ^
  static std::vector<Matrix3x3> transform_stack = { Matrix3x3::identity() };


// Implements SoftwareRenderer //

// fill a sample location with color
void SoftwareRendererImp::fill_sample(int sx, int sy, const Color &color) {
  // Task 2: implement this function
  unsigned char* buffer = ssaa_buffer ? ssaa_buffer : pixel_buffer;
  int width = this->width * sample_rate;
  int height = this->height * sample_rate;

  // check bounds
  if (sx < 0 || sx >= width) return;
  if (sy < 0 || sy >= height) return;

  Color pixel_color;
  float inv255 = 1.0f / 255.0f;
  pixel_color.r = buffer[4 * (sx + sy * width) + 0] * inv255;
  pixel_color.g = buffer[4 * (sx + sy * width) + 1] * inv255;
  pixel_color.b = buffer[4 * (sx + sy * width) + 2] * inv255;
  pixel_color.a = buffer[4 * (sx + sy * width) + 3] * inv255;

  //pixel_color = ref->alpha_blending_helper(pixel_color, color);
  pixel_color = alpha_blending(pixel_color, color);

  buffer[4 * (sx + sy * width) + 0] = (uint8_t)(pixel_color.r * 255);
  buffer[4 * (sx + sy * width) + 1] = (uint8_t)(pixel_color.g * 255);
  buffer[4 * (sx + sy * width) + 2] = (uint8_t)(pixel_color.b * 255);
  buffer[4 * (sx + sy * width) + 3] = (uint8_t)(pixel_color.a * 255);
}

// fill samples in the entire pixel specified by pixel coordinates
void SoftwareRendererImp::fill_pixel(int x, int y, const Color &color) {

  // Task 2: Re-implement this function

  for (int j = y * sample_rate; j < (y + 1) * sample_rate; j++) {
	for (int i = x * sample_rate; i < (x + 1) * sample_rate; i++) {
	  fill_sample(i, j, color);
	}
  }

}

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  // set top level transformation
  transformation = canvas_to_screen;

  // canvas outline
  Vector2D a = transform(Vector2D(0, 0)); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width, 0)); b.x++; b.y--;
  Vector2D c = transform(Vector2D(0, svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width, svg.height)); d.x++; d.y++;

  svg_bbox_top_left = Vector2D(a.x+1, a.y+1);
  svg_bbox_bottom_right = Vector2D(d.x-1, d.y-1);

  // draw all elements
  for (size_t i = 0; i < svg.elements.size(); ++i) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to pixel buffer
  resolve();

}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  // Task 2: 
  // You may want to modify this for supersampling support
  if (this->sample_rate == sample_rate)
	return;
  this->sample_rate = sample_rate;
  if (ssaa_buffer) {
	delete[] ssaa_buffer;
	ssaa_buffer = nullptr;
  }
  if (sample_rate > 1) {
	ssaa_buffer = new unsigned char[width * height * sample_rate * sample_rate * 4];
	memset(ssaa_buffer, 255, width * height * sample_rate * sample_rate * 4);
  }

}

void SoftwareRendererImp::set_pixel_buffer( unsigned char* pixel_buffer,
                                             size_t width, size_t height ) {

  // Task 2: 
  // You may want to modify this for supersampling support
  this->pixel_buffer = pixel_buffer;
  this->width = width;
  this->height = height;

  if (ssaa_buffer) {
	delete[] ssaa_buffer;
	ssaa_buffer = nullptr;
  }
  if (sample_rate > 1) {
	ssaa_buffer = new unsigned char[width * height * sample_rate * sample_rate * 4];
	memset(ssaa_buffer, 255, width * height * sample_rate * sample_rate * 4);
  }

}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

	// Task 3 (part 1):
	// Modify this to implement the transformation stack

	Matrix3x3 model = transform_stack.back() * element->transform;
	transform_stack.push_back(model);
	transformation = canvas_to_screen * model;

	switch (element->type) {
	case POINT:
		draw_point(static_cast<Point&>(*element));
		break;
	case LINE:
		draw_line(static_cast<Line&>(*element));
		break;
	case POLYLINE:
		draw_polyline(static_cast<Polyline&>(*element));
		break;
	case RECT:
		draw_rect(static_cast<Rect&>(*element));
		break;
	case POLYGON:
		draw_polygon(static_cast<Polygon&>(*element));
		break;
	case ELLIPSE:
		draw_ellipse(static_cast<Ellipse&>(*element));
		break;
	case IMAGE:
		draw_image(static_cast<Image&>(*element));
		break;
	case GROUP:
		draw_group(static_cast<Group&>(*element));
		break;
	default:
		break;
	}

	transform_stack.pop_back();

}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  
  // Advanced Task
  // Implement ellipse rasterization
  Vector2D center = transform(ellipse.center) * float(sample_rate);
  // Transform dir only (no translation)
  Vector3D a3d(ellipse.radius.x, 0.0, 0.0);
  Vector3D b3d(0.0, ellipse.radius.y, 0.0);
  a3d = transformation * a3d * float(sample_rate);
  b3d = transformation * b3d * float(sample_rate);
  Vector2D a = Vector2D(a3d.x, a3d.y);
  Vector2D b = Vector2D(b3d.x, b3d.y);

  Vector2D b0 = center + a + b;
  Vector2D b1 = center + a - b;
  Vector2D b2 = center - a + b;
  Vector2D b3 = center - a - b;
  int minx = max(0, int(min(min(b0.x, b1.x), min(b2.x, b3.x))));
  int miny = max(0, int(min(min(b0.y, b1.y), min(b2.y, b3.y))));
  int maxx = min(int(width*sample_rate)-1,  int(max(max(b0.x, b1.x), max(b2.x, b3.x))));
  int maxy = min(int(height*sample_rate)-1, int(max(max(b0.y, b1.y), max(b2.y, b3.y))));
  
  float an2 = a.x * a.x + a.y * a.y;
  float bn2 = b.x * b.x + b.y * b.y;

  for (int y = miny; y <= maxy; y++) {
	for (int x = minx; x <= maxx; x++) {
	  float xf = float(x) + 0.5f - center.x;
	  float yf = float(y) + 0.5f - center.y;
	  // DOES NOT WORK FOR SHEARED TRANSFORMS
	  float af = (a.x * xf + a.y * yf) / an2;
	  float bf = (b.x * xf + b.y * yf) / bn2;
	  if (af * af + bf * bf <= 1.0f) {
		fill_sample(x, y, ellipse.style.fillColor);
	  }
	}
  }

}

void SoftwareRendererImp::draw_image( Image& image ) {

  // Advanced Task
  // Render image element with rotation

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {

  // fill in the nearest pixel
  int sx = (int)floor(x);
  int sy = (int)floor(y);

  // check bounds
  if (sx < 0 || sx >= width) return;
  if (sy < 0 || sy >= height) return;

  // fill sample - NOT doing alpha blending!
  // TODO: Call fill_pixel here to run alpha blending
  fill_pixel(sx, sy, color);

}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {

  // Task 0: 
  // Implement Bresenham's algorithm (delete the line below and implement your own)

  int start, end;
  // true if x should increment, false if y
  bool xbase = abs(x1 - x0) > abs(y1 - y0);
  if (xbase) {
	start = (int)floor(x0);
	end = (int)floor(x1);
  }
  else {
	start = (int)floor(y0);
	end = (int)floor(y1);
  }
  int stride = (start < end) ? 1 : -1;
  float slope = (float)stride * (xbase ? (y1 - y0) / (x1 - x0) : (x1 - x0) / (y1 - y0));
  float w2 = xbase ? y0 : x0;

  for (int w1 = start; true; w1 += stride) {
	int sx = xbase ? w1 : (int)floor(w2);
	int sy = xbase ? (int)floor(w2) : w1;
	if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
	  fill_pixel(sx, sy, color);
	}
	w2 += slope;
	if (w1 == end)
	  break;
  }

  // Advanced Task
  // Drawing Smooth Lines with Line Width
}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 1: 
  // Implement triangle rasterization
  x0 = x0 * sample_rate - 0.5f;
  y0 = y0 * sample_rate - 0.5f;
  x1 = x1 * sample_rate - 0.5f;
  y1 = y1 * sample_rate - 0.5f;
  x2 = x2 * sample_rate - 0.5f;
  y2 = y2 * sample_rate - 0.5f;
  size_t width = this->width * sample_rate;
  size_t height = this->height * sample_rate;
  float minx = max(0.0f, min(x0, min(x1, x2)));
  float miny = max(0.0f, min(y0, min(y1, y2)));
  float maxx = min((float)width - 0.5f,  max(x0, max(x1, x2)));
  float maxy = min((float)height - 0.5f, max(y0, max(y1, y2)));

  auto cross = [](float x0, float y0, float x1, float y1) {
	return x0 * y1 - x1 * y0;
  };
  bool ccw = cross(x1 - x0, y1 - y0, x2 - x0, y2 - y0) > 0.0f;

  for (int sx = int(minx); sx <= int(maxx); sx++) {
	for (int sy = int(miny); sy <= int(maxy); sy++) {
	  float x_ = float(sx);
	  float y_ = float(sy);
	  // triangle test
	  float d1 =  cross(x_ - x0, x1 - x0, y_ - y0, y1 - y0);
	  float d2 = -cross(x_ - x0, x2 - x0, y_ - y0, y2 - y0);
	  float d3 =  cross(x_ - x1, x2 - x1, y_ - y1, y2 - y1);
	  constexpr bool use_edge_rules = true;	// USE EDGE RULES
	  bool top_edge = use_edge_rules && (
		(d1 == 0.0f && int(y0) == int(y1) && int(y1) == int(miny)) ||
		(d2 == 0.0f && int(y0) == int(y2) && int(y2) == int(miny)) ||
		(d3 == 0.0f && int(y1) == int(y2) && int(y2) == int(miny))
	  );
	  bool left_edge = use_edge_rules && (top_edge ||  // short circuit as optimization
		(d1 == 0.0f && ccw == (int(y0) < int(y1))) ||
		(d2 == 0.0f && ccw == (int(y0) < int(y2))) ||
		(d3 == 0.0f && ccw == (int(y2) < int(y1)))
	  );
	  if (
		(d1 < 0.0f && d2 < 0.0f && d3 < 0.0f) ||
		(d1 > 0.0f && d2 > 0.0f && d3 > 0.0f) ||
		(top_edge || left_edge)
	  ) {
		fill_sample(sx, sy, color);
	  }
	}
  }

  // Advanced Task
  // Implementing Triangle Edge Rules

}


void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 4: 
  // Implement image rasterization

  x0 *= sample_rate;
  x1 *= sample_rate;
  y0 *= sample_rate;
  y1 *= sample_rate;
  float minxf = min(x0, x1);
  float maxxf = max(x0, x1);
  float minyf = min(y0, y1);
  float maxyf = max(y0, y1);
  int minx = max(0, min(int(width * sample_rate) - 1, int(minxf + 0.5f)));
  int maxx = max(0, min(int(width * sample_rate) - 1, int(maxxf + 0.5f)));
  int miny = max(0, min(int(height * sample_rate) - 1, int(minyf + 0.5f)));
  int maxy = max(0, min(int(height * sample_rate) - 1, int(maxyf + 0.5f)));

  for (int y = miny; y <= maxy; y++) {
	for (int x = minx; x <= maxx; x++) {
	  float u = abs(float(x) + 0.5f - x0) / (maxxf - minxf);
	  float v = abs(float(y) + 0.5f - y0) / (maxyf - minyf);
	  fill_sample(x, y, sampler->sample_bilinear(tex, u, v, 0));
	}
  }

}

// resolve samples to pixel buffer
void SoftwareRendererImp::resolve( void ) {

  // Task 2: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 2".

  if (!ssaa_buffer) return;

  for (int y = 0; y < height; y++) {
	for (int x = 0; x < width; x++) {
	  uint16_t s[4] = { 0, 0, 0, 0 };
	  for (int j = y * sample_rate; j < (y + 1) * sample_rate; j++) {
		for (int i = x * sample_rate; i < (x + 1) * sample_rate; i++) {
		  for (int c = 0; c < 4; c++) {
			s[c] += ssaa_buffer[4 * (i + j * width * sample_rate) + c];
		  }
		}
	  }
	  for (int c = 0; c < 4; c++) {
		pixel_buffer[4 * (x + y * width) + c] = s[c] / (sample_rate * sample_rate);
	  }
	}
  }

}

Color SoftwareRendererImp::alpha_blending(Color pixel_color, Color color)
{
  // Task 5
  // Implement alpha compositing
  Color c;
  c.a = 1.0f - (1.0f - color.a) * (1.0f - pixel_color.a);
  if (c.a == 0.0f) {
	return Color(0.0f, 0.0f, 0.0f, 0.0f);
  }
  c.r = (1.0f - color.a) * pixel_color.r + color.r;
  c.g = (1.0f - color.a) * pixel_color.g + color.g;
  c.b = (1.0f - color.a) * pixel_color.b + color.b;
  return c;
}


} // namespace CS248
