#include <string>
#include <sf2d.h>
#include <sftd.h>
#include "Common.h"
#include "Slider.h"

extern sftd_font *font;

Slider::Slider()
{
	value = 0.0f;
	min = 0.0f;
	max = 1.0f;
}

Slider::Slider(float min, float max)
{
	value = min;
	this->min = min;
	this->max = max;
}

void Slider::TouchingAnywhere(int x, int y)
{
	value = Interpolate((float)x, 1.0f, (float)(width - 2), min, max);
	if (value < min) value = min;
	else if (value > max) value = max;
}

void Slider::Draw(int x, int y, int w, int h)
{
	width = w;
	int fillWidth = (int)Interpolate(value, min, max, 0.0f, (float)(w - 2));
	sf2d_draw_rectangle(x+1, y+1, w-2, h-2, RGBA8(0xFF, 0xFF, 0xFF, 0xFF));
	sf2d_draw_rectangle(x+1, y+1, fillWidth, h-2, RGBA8(0x00, 0xCC, 0xFF, 0xFF));
	sftd_draw_textf(font, x + 8, y + h/2 - 14, RGBA8(0x00, 0x00, 0x00, 0xFF), 20, "%.5f", value);
}

void Slider::TouchingInside(int x, int y)
{
	Control::TouchingInside(x, y);
	TouchingAnywhere(x, y);
}

void Slider::TouchingOutside(int x, int y)
{
	Control::TouchingOutside(x, y);
	TouchingAnywhere(x, y);
}

void Slider::SetRange(float min, float max)
{
	if (min > max) std::swap(min, max);
	this->min = min;
	this->max = max;
}

float Slider::GetMinimum() const
{
	return min;
}

float Slider::GetMaximum() const
{
	return max;
}