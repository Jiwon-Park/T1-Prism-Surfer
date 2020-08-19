#ifndef __PARTICLE_H__
#define __PARTICLE_H__
#pragma once

#include "cgmath.h"
#include "cgut.h"

inline float random_range(float min, float max) { return mix(min, max, rand() / float(RAND_MAX)); }

struct particle_t
{
	static constexpr int MAX_PARTICLES = 50;

	vec2 pos;
	vec4 color;
	vec2 velocity;
	float scale;
	float life;

	//optional
	float elapsed_time;
	float time_interval;

	particle_t() { reset(); }
	void reset();
	void update();
};

inline void particle_t::reset()
{
	pos = vec2(random_range(-1.0f, 1.0f), random_range(-1.0f, 1.0f));
	color = vec4(random_range(0, 1.0f), random_range(0, 1.0f), random_range(0, 1.0f), 1);
	scale = random_range(0.02f, 0.2f);
	life = random_range(0.01f, 1.0f);
	velocity = vec2(random_range(-1.0f, 1.0f), random_range(-1.0f, 1.0f)) * 0.003f;
	elapsed_time = 0.0f;
	time_interval = random_range(200.0f, 600.0f);
}

inline void particle_t::update()
{
	const float dwTime = (float)glfwGetTime();
	elapsed_time += dwTime;

	if (elapsed_time > time_interval)
	{
		const float theta = random_range(0, 1.0f) * PI * 2.0f;
		constexpr float velocity_factor = 0.003f;
		velocity = vec2(cos(theta), sin(theta)) * velocity_factor;

		elapsed_time = 0.0f;
	}

	pos += velocity;

	constexpr float life_factor = 0.001f;
	life -= life_factor * dwTime;

	// disappear
	if (life < 0.0f)
	{
		constexpr float alpha_factor = 0.001f;
		color.a -= alpha_factor * dwTime;
	}

	// dead
	if (color.a < 0.0f) reset();
}

#endif