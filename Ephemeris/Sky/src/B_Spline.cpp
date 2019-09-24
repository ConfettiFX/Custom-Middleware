#include "B_Spline.h"

void B_Spline::interpolate(eastl::vector<float> &time, uint degree, eastl::vector<vec3> &points, eastl::vector<float> &knots, eastl::vector<float> &weights, eastl::vector<vec3> &result)
{
  //var i, j, s, l;                // function-scoped iteration variables
  uint32_t numberOfPoints = (uint32_t)points.size();    // points count
  uint32_t dimension = 3; // point dimensionality

  /*
  if (degree < 1)
  {
    ASSERT(false);
  }

  if (degree > (n - 1))
  {
    ASSERT(false);
  }
  */

  /*
  if ((uint32_t)weights.size() == 0)
  {
    // build weight vector of length [n]       
    for (uint32_t i = 0; i < numberOfPoints; i++)
    {
      weights.push_back(1);
    }
  }
  */

  if ((uint32_t)knots.size() == 0)
  {
    // build knot vector of length [n + degree + 1]       
    for (uint32_t i = 0; i < numberOfPoints + degree + 1; i++)
    {
      knots.push_back((float)i);
    }
  }
  else
  {
    //ASSERT((uint32_t)knots.size() == (numberOfPoints + degree + 1));
  }
  

  // var domain = [ degree, knots.length - 1 - degree];
  eastl::vector<uint32_t> domain;
  domain.push_back(degree);
  domain.push_back((uint32_t)knots.size() - 1 - degree);

  

  // convert points to homogeneous coordinates
  //var v = [];
  eastl::vector<eastl::vector<float>> v;

  v.resize(numberOfPoints);

  for (uint32_t i = 0; i < numberOfPoints; i++)
  {
    //v[i] = [];
    v[i].resize(dimension + 1);
    for (uint32_t j = 0; j < dimension; j++)
    {
      v[i][j] = points[i][j];// * weights[i];
    }
    v[i][dimension] = 1.0f;//weights[i];
  }

  // remap t to the domain where the spline is defined
  float low = knots[domain[0]];
  float high = knots[domain[1]];  

  for (uint32_t i = 0; i < (uint32_t)time.size(); i++)
  {

    float  t = time[i];

    t = t * (high - low) + low;

    if (t < low || t > high)
    {
      //ASSERT(false);
    }

    uint s;

    // find s (the spline segment) for the [t] value provided
    for (s = domain[0]; s < domain[1]; s++)
    {
      if (t >= knots[s] && t <= knots[s + 1])
      {
        break;
      }
    }   

    eastl::vector<eastl::vector<float>> v2 = v;

    uint degreePlusOne = degree + 1;
    uint dimensionPlusOne = dimension + 1;

    // l (level) goes from 1 to the curve degree + 1
    float alpha;
    for (uint32_t l = 1; l <= degreePlusOne; l++)
    {
      uint PyramidIndex = s - degree - 1 + l;
      // build level l of the pyramid
      for (uint32_t i = s; i > PyramidIndex; i--)
      {
        uint denomKnots = i + degreePlusOne - l;
        alpha = (t - knots[i]) / (knots[denomKnots] - knots[i]);

        // interpolate each component
        for (uint32_t j = 0; j < dimensionPlusOne; j++)
        {
          v2[i][j] = (1 - alpha) * v2[i - 1][j] + alpha * v2[i][j];
        }
      }
    }

    // convert back to cartesian and return
    result.push_back(vec3(v2[s][0], v2[s][1], v2[s][2]) / v2[s][3]);
  }
}