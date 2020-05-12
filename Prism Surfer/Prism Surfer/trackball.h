#ifndef __TRACKBALL_H__
#define __TRACKBALL_H__
#include "cgmath.h"

struct trackball
{
	bool	b_tracking = false;
	bool	b_pan_tracking = false;
	bool	b_zoom_tracking = false;
	float	scale;			// controls how much rotation is applied
	float	scale_pan;
	float	scale_zoom;
	mat4	view_matrix0;	// initial view matrix
	mat4	view_matrix0_pan;
	mat4	view_matrix0_zoom;
	vec2	m0;				// the last mouse position
	vec2	m0_pan;
	float	m0_zoom;

	trackball(float rot_scale = 1.0f, float pan_scale = 30.0f, float zoom_scale = 1.0f) : scale(rot_scale), scale_pan(pan_scale), scale_zoom(zoom_scale) {}
	bool is_tracking() const { return b_tracking; }
	bool is_trackingpan() const { return b_pan_tracking; }
	bool is_trackingzoom() const { return b_zoom_tracking; }
	void begin(const mat4& view_matrix, vec2 m);
	void beginpan(const mat4& view_matrix, vec2 m);
	void beginzoom(const mat4& view_matrix, float y);
	void end() { b_tracking = false; }
	void endpan() { b_pan_tracking = false; }
	void endzoom() { b_zoom_tracking = false; }
	mat4 update(vec2 m) const;
	mat4 pan(vec2 m) const;
	mat4 zoom(float y) const;
};

inline void trackball::begin(const mat4& view_matrix, vec2 m)
{
	b_tracking = true;			// enable trackball tracking
	m0 = m;						// save current mouse position
	view_matrix0 = view_matrix;	// save current view matrix
}

inline void trackball::beginpan(const mat4& view_matrix, vec2 m)
{
	b_pan_tracking = true;			// enable trackball tracking
	m0_pan = m;						// save current mouse position
	view_matrix0_pan = view_matrix;	// save current view matrix
}

inline void trackball::beginzoom(const mat4& view_matrix, float y)
{
	b_zoom_tracking = true;			// enable trackball tracking
	m0_zoom = y;						// save current mouse position
	view_matrix0_zoom = view_matrix;	// save current view matrix
}

inline mat4 trackball::update(vec2 m) const
{
	// project a 2D mouse position to a unit sphere
	static const vec3 p0 = vec3(0, 0, 1.0f);	// reference position on sphere
	vec3 p1 = vec3(m - m0, 0);					// displacement
	if (!b_tracking || length(p1) < 0.0001f) return view_matrix0;		// ignore subtle movement
	p1 *= scale;														// apply rotation scale
	p1 = vec3(p1.x, p1.y, sqrtf(max(0, 1.0f - length2(p1)))).normalize();	// back-project z=0 onto the unit sphere

	// find rotation axis and angle in world space
	// - trackball self-rotation should be done at first in the world space
	// - mat3(view_matrix0): rotation-only view matrix
	// - mat3(view_matrix0).transpose(): inverse view-to-world matrix
	vec3 v = mat3(view_matrix0).transpose() * p0.cross(p1);
	float theta = asin(min(v.length(), 1.0f));

	// resulting view matrix, which first applies
	// trackball rotation in the world space
	return view_matrix0 * mat4::rotate(v.normalize(), theta);
}

inline mat4 trackball::pan(vec2 m) const
{
	vec3 p1 = vec3(m - m0_pan, 0);
	if (!b_pan_tracking || length(p1) < 0.0001f)return view_matrix0_pan;
	p1 *= scale_pan;
	return view_matrix0_pan * mat4::translate(p1);
}

inline mat4 trackball::zoom(float y) const
{
	float p1 = m0_zoom - y;
	if (!b_zoom_tracking || abs(p1) < 0.0001f)return view_matrix0_zoom;
	p1 *= scale_zoom;
	if (p1 >= 1.0f) p1 = 0.999f;
	return view_matrix0_zoom * mat4::scale(vec3(1.0f - p1, 1.0f - p1, 1.0f - p1));
}

// utility function
inline vec2 cursor_to_ndc(dvec2 cursor, ivec2 window_size)
{
	// normalize window pos to [0,1]^2
	vec2 npos = vec2(float(cursor.x) / float(window_size.x - 1),
		float(cursor.y) / float(window_size.y - 1));

	// normalize window pos to [-1,1]^2 with vertical flipping
	// vertical flipping: window coordinate system defines y from
	// top to bottom, while the trackball from bottom to top
	return vec2(npos.x * 2.0f - 1.0f, 1.0f - npos.y * 2.0f);
}

#endif // __TRACKBALL_H__
