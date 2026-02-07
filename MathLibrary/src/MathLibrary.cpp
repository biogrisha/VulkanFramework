#include "MathLibrary.h"

float Math::PointsToInches(float Points)
{
	return Points / 72.f;
}

float Math::PointsToPixels(float Points, float Dpi)
{
	return PointsToInches(Points) * Dpi;
}
