#version 330



uniform sampler2DRect sampler_world_colour;
uniform int showEdgeDetection;

//in vec2 textureCoords;

layout(location = 0) out vec3 reflected_light;


// Function prototypes
vec3 Inverted(vec3);
vec3 DiscreteConvolution(vec2);
vec3 BlurFilter(vec2);
float BlurFilterColour(float, vec2);
float luminance(vec3);
vec3 SobalEdgeDetection();
vec3 CannyEdgeDetection();
vec3 EdgeDetection();


vec2 SobalEdgeStrengthAt(vec2);

void main(void)
{
	vec3 result = texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy)).rgb;
	//result = BlurFilter();
	result = CannyEdgeDetection();
	//result = EdgeDetection();
	
	reflected_light = result;
}


vec3 EdgeDetection()
{

	// greyscale
	float middleLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy)).rgb);
	float leftLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(-1, 0))).rgb);
	float rightLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(1, 0))).rgb);

	float topLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(0, -1))).rgb);
	float topleftLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(-1, -1))).rgb);
	float toprightLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(1, -1))).rgb);

	float bottomLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(0, 1))).rgb);
	float bottomleftLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(-1, 1))).rgb);
	float bottomrightLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(1, 1))).rgb);

	// Gausian Blur (small)
	float topleftBlurLum = BlurFilterColour(topleftLum, vec2(0, 0));
	float topBlurLum = BlurFilterColour(topLum, vec2(1, 0));
	float toprightBlurLum = BlurFilterColour(toprightLum, vec2(2, 0));

	float leftBlurLum = BlurFilterColour(leftLum, vec2(0, 1));
	float middleBlurLum = BlurFilterColour(middleLum, vec2(1, 1));
	float rightBlurLum = BlurFilterColour(rightLum, vec2(2, 1));

	float bottomleftBlurLum = BlurFilterColour(bottomleftLum, vec2(0, 2));
	float bottomBlurLum = BlurFilterColour(bottomLum, vec2(1, 2));
	float bottomrightBlurLum = BlurFilterColour(bottomrightLum, vec2(2, 2));



	// Edge Detection
	// https://www.youtube.com/watch?v=uihBwtPIBxM
	float vertical = -1.0 * topleftBlurLum - 2.0 * topBlurLum - 1.0 * toprightBlurLum
		+ 0.0 * leftBlurLum + 0.0 * middleBlurLum + 0.0 * rightBlurLum
		+ 1.0 * bottomleftBlurLum + 2.0 * bottomBlurLum + 1.0 * bottomrightBlurLum;
	// unsure if values are right
	float horizontal = -1.0 * topleftBlurLum + 0.0 * topBlurLum + 1.0 * toprightBlurLum
		- 2.0 * leftBlurLum + 0.0 * middleBlurLum + 2.0 * rightBlurLum
		- 1.0 * bottomleftBlurLum + 0.0 * bottomBlurLum + 1.0 * bottomrightBlurLum;

	float edge = length(vec2(horizontal, vertical));
	edge = min(edge, 1.0);

	/*vec3 blend = vec3(	1.0 * topleftLum + 2.0 * topLum + 1.0 * toprightLum, 
						2.0 * leftLum + 4.0 * middleLum + 2.0 * rightLum,
						1.0 * bottomleftLum + 2.0 * bottomLum + 1.0 * bottomrightLum);
	blend /= 16.0;


	vec3 middle = texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy)).rgb;
	vec3 result = mix(middle, blend, edge);

	return result;*/

	
	if (edge < 1)
	{
		vec3 middle = texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy)).rgb;
		return middle;
	}
	else
	{
		vec3 middle = texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy)).rgb;
		return vec3(1.0, 1.0, 0.0) * BlurFilter(gl_FragCoord.xy);// *vec3(1.0, 1.0, 0.0);
		// I THINK THIS IS BECAUSE I NEED TO BLUR WHOLE IMAGE FIRST, not just surrounding pixels? Actually not much difference?
		
		
	}

	//return vec3(edge);
}

vec2 SobalEdgeStrengthAt(vec2 fragMiddle)
{
	// SOBAL
	// greyscale
	float middleLum = luminance(texelFetch(sampler_world_colour, ivec2(fragMiddle.xy)).rgb);
	float leftLum = luminance(texelFetch(sampler_world_colour, ivec2(fragMiddle.xy + vec2(-1, 0))).rgb);
	float rightLum = luminance(texelFetch(sampler_world_colour, ivec2(fragMiddle.xy + vec2(1, 0))).rgb);

	float topLum = luminance(texelFetch(sampler_world_colour, ivec2(fragMiddle.xy + vec2(0, -1))).rgb);
	float topleftLum = luminance(texelFetch(sampler_world_colour, ivec2(fragMiddle.xy + vec2(-1, -1))).rgb);
	float toprightLum = luminance(texelFetch(sampler_world_colour, ivec2(fragMiddle.xy + vec2(1, -1))).rgb);

	float bottomLum = luminance(texelFetch(sampler_world_colour, ivec2(fragMiddle.xy + vec2(0, 1))).rgb);
	float bottomleftLum = luminance(texelFetch(sampler_world_colour, ivec2(fragMiddle.xy + vec2(-1, 1))).rgb);
	float bottomrightLum = luminance(texelFetch(sampler_world_colour, ivec2(fragMiddle.xy + vec2(1, 1))).rgb);

	// Gausian Blur (small)
	float topleftBlurLum = BlurFilterColour(topleftLum, vec2(0, 0));
	float topBlurLum = BlurFilterColour(topLum, vec2(1, 0));
	float toprightBlurLum = BlurFilterColour(toprightLum, vec2(2, 0));

	float leftBlurLum = BlurFilterColour(leftLum, vec2(0, 1));
	float middleBlurLum = BlurFilterColour(middleLum, vec2(1, 1));
	float rightBlurLum = BlurFilterColour(rightLum, vec2(2, 1));

	float bottomleftBlurLum = BlurFilterColour(bottomleftLum, vec2(0, 2));
	float bottomBlurLum = BlurFilterColour(bottomLum, vec2(1, 2));
	float bottomrightBlurLum = BlurFilterColour(bottomrightLum, vec2(2, 2));

	// Edge Detection
	// https://www.youtube.com/watch?v=uihBwtPIBxM
	float vertical = -1.0 * topleftBlurLum - 2.0 * topBlurLum - 1.0 * toprightBlurLum
		+ 0.0 * leftBlurLum + 0.0 * middleBlurLum + 0.0 * rightBlurLum
		+ 1.0 * bottomleftBlurLum + 2.0 * bottomBlurLum + 1.0 * bottomrightBlurLum;
	// unsure if values are right
	float horizontal = -1.0 * topleftBlurLum + 0.0 * topBlurLum + 1.0 * toprightBlurLum
		- 2.0 * leftBlurLum + 0.0 * middleBlurLum + 2.0 * rightBlurLum
		- 1.0 * bottomleftBlurLum + 0.0 * bottomBlurLum + 1.0 * bottomrightBlurLum;

	float edge = length(vec2(horizontal, vertical));
	edge = min(edge, 1.0);


	return vec2(edge, (pow(atan(horizontal, vertical), 2.0) / 3.14159) * 180.0);
}

vec3 CannyEdgeDetection()
{
	vec2 middleSobal = SobalEdgeStrengthAt(gl_FragCoord.xy);
	// END SOBAL
	

	// orientation
	//float orientation = atan(horizontal / vertical); // radians/degrees

	//http://www.pages.drexel.edu/~nk752/cannyTut2.html
	float thisAngle = middleSobal.y;	// Calculate actual direction of edge
	float newAngle = 0;
	/* Convert actual edge direction to approximate value */
	if (((thisAngle < 22.5) && (thisAngle > -22.5)) || (thisAngle > 157.5) || (thisAngle < -157.5))
		newAngle = 0;
	if (((thisAngle > 22.5) && (thisAngle < 67.5)) || ((thisAngle < -112.5) && (thisAngle > -157.5)))
		newAngle = 45;
	if (((thisAngle > 67.5) && (thisAngle < 112.5)) || ((thisAngle < -67.5) && (thisAngle > -112.5)))
		newAngle = 90;
	if (((thisAngle > 112.5) && (thisAngle < 157.5)) || ((thisAngle < -22.5) && (thisAngle > -67.5)))
		newAngle = 135;


	vec3 result = texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy)).rgb;
	// given orientation, is it bigger than its neighbour
	//if (middleSobal.x >= 0.8)
	{
	
		vec3 returnVal = vec3(BlurFilter(gl_FragCoord.xy));// result;// vec3(1, 1, 0);// *vec3(BlurFilter(gl_FragCoord.xy));//vec3(BlurFilter(gl_FragCoord.xy));// vec3(middleSobal.x);
		if(showEdgeDetection == 1)
			returnVal = vec3(1, 1, 0);

		middleSobal.x = 0.95;
		if (newAngle == 0) // horizontal
		{
			if (SobalEdgeStrengthAt(gl_FragCoord.xy + vec2(0, -1)).x > middleSobal.x
				&& SobalEdgeStrengthAt(gl_FragCoord.xy + vec2(0, 1)).x > middleSobal.x)
			{
				return returnVal;
			}
		}
		else if (newAngle == 45)
		{
			if ( SobalEdgeStrengthAt(gl_FragCoord.xy + vec2(-1, 1)).x > middleSobal.x
				&&  SobalEdgeStrengthAt(gl_FragCoord.xy + vec2(1, -1)).x > middleSobal.x)
			{
				return returnVal;
			}
		}
		else if (newAngle == 90)
		{
			if ( SobalEdgeStrengthAt(gl_FragCoord.xy + vec2(-1, 0)).x > middleSobal.x
				&&  SobalEdgeStrengthAt(gl_FragCoord.xy + vec2(1, 0)).x > middleSobal.x)
			{
				return returnVal;
			}
		}
		else if (newAngle == 135)
		{
			if ( SobalEdgeStrengthAt(gl_FragCoord.xy + vec2(-1, -1)).x > middleSobal.x
				&&  SobalEdgeStrengthAt(gl_FragCoord.xy + vec2(1, 1)).x > middleSobal.x)
			{
				return returnVal;
			}
		}
	}



	//return texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy)).rgb;
	return result;// vec3(0, 0, 0);
}

vec3 SobalEdgeDetection()
{

	// greyscale
	float middleLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy)).rgb);
	float leftLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(-1, 0))).rgb);
	float rightLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(1, 0))).rgb);
	
	float topLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(0, -1))).rgb);
	float topleftLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(-1, -1))).rgb);
	float toprightLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(1, -1))).rgb);
	
	float bottomLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(0, 1))).rgb);
	float bottomleftLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(-1, 1))).rgb);
	float bottomrightLum = luminance(texelFetch(sampler_world_colour, ivec2(gl_FragCoord.xy + vec2(1, 1))).rgb);

	// Gausian Blur (small)
	float topleftBlurLum = BlurFilterColour(topleftLum, vec2(0, 0));
	float topBlurLum = BlurFilterColour(topLum, vec2(1, 0));
	float toprightBlurLum = BlurFilterColour(toprightLum, vec2(2, 0));

	float leftBlurLum = BlurFilterColour(leftLum, vec2(0, 1));
	float middleBlurLum = BlurFilterColour(middleLum, vec2(1, 1));
	float rightBlurLum = BlurFilterColour(rightLum, vec2(2, 1));
	
	float bottomleftBlurLum = BlurFilterColour(bottomleftLum, vec2(0, 2));
	float bottomBlurLum = BlurFilterColour(bottomLum, vec2(1, 2));
	float bottomrightBlurLum = BlurFilterColour(bottomrightLum, vec2(2, 2));



	// Edge Detection
	// https://www.youtube.com/watch?v=uihBwtPIBxM
	float vertical =  - 1.0 * topleftBlurLum - 2.0 * topBlurLum - 1.0 * toprightBlurLum
					 + 0.0 * leftBlurLum + 0.0 * middleBlurLum + 0.0 * rightBlurLum
					 + 1.0 * bottomleftBlurLum + 2.0 * bottomBlurLum + 1.0 * bottomrightBlurLum;
	// unsure if values are right
	float horizontal =	 -1.0 * topleftBlurLum + 0.0 * topBlurLum + 1.0 * toprightBlurLum
						- 2.0 * leftBlurLum + 0.0 * middleBlurLum + 2.0 * rightBlurLum
						- 1.0 * bottomleftBlurLum + 0.0 * bottomBlurLum + 1.0 * bottomrightBlurLum;

	float edge = length(vec2(horizontal, vertical));
	edge = min(edge, 1.0);

	return vec3(edge);
}

float luminance(vec3 pixel)
{
	// https://en.wikipedia.org/wiki/Grayscale
	// for Linear RGB
	return dot(pixel, vec3(0.02126, 07152, 0.0722));
}

float BlurFilterColour(float pixel, vec2 pos)
{
	/*const mat3 kernel = mat3(0.05, 0.1, 0.05,
		0.1, 0.4, 0.1,
		0.05, 0.1, 0.05);*/
	//http://dev.theomader.com/gaussian-kernel-calculator/
	const mat3 kernel = mat3(0.077847, 0.123317, 0.077847,
		0.123317, 0.195346, 0.123317,
		0.077847, 0.123317, 0.077847);

	return (kernel[int(pos.x)][int(pos.y)]) * pixel; //* 0.04
}

vec3 BlurFilter(vec2 middleCoord)
{
	/*const mat3 kernel = mat3(0.05, 0.1, 0.05,
									0.1, 0.4, 0.1,
									0.05, 0.1, 0.05);*/
	//http://dev.theomader.com/gaussian-kernel-calculator/
	const mat3 kernel = mat3(0.077847, 0.123317, 0.077847,
							0.123317, 0.195346, 0.123317,
							0.077847, 0.123317, 0.077847);

	// maybe use equation https://en.wikipedia.org/wiki/Gaussian_blur?


	vec3 middle = texelFetch(sampler_world_colour, ivec2(middleCoord.xy)).rgb;
	vec3 left = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(-1, 0))).rgb;
	vec3 right = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(1, 0))).rgb;
	vec3 top = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(0, -1))).rgb;
	vec3 topleft = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(-1, -1))).rgb;
	vec3 topright = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(1, -1))).rgb;
	vec3 bottom = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(0, 1))).rgb;
	vec3 bottomleft = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(-1, 1))).rgb;
	vec3 bottomright = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(1, 1))).rgb;


	return	
		(0.077847 * topleft
		+ 0.123317 * left
		+ 0.077847 * bottomleft

		+ 0.123317 * top
		+ 0.195346 * middle
		+ 0.123317 * bottom

		+ 0.077847 * topright
		+ 0.123317 * right
		+ 0.077847 * bottomright); 
}

vec3 DiscreteConvolution(vec2 middleCoord)
{
	const mat3 kernel = mat3(0.11, 0.11, 0.11,
		0.11, 0.11, 0.11,
		0.11, 0.11, 0.11);

	vec3 middle = texelFetch(sampler_world_colour, ivec2(middleCoord.xy)).rgb;
	vec3 left = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(-1, 0))).rgb;
	vec3 right = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(1, 0))).rgb;
	vec3 top = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(0, -1))).rgb;
	vec3 topleft = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(-1, -1))).rgb;
	vec3 topright = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(1, -1))).rgb;
	vec3 bottom = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(0, 1))).rgb;
	vec3 bottomleft = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(-1, 1))).rgb;
	vec3 bottomright = texelFetch(sampler_world_colour, ivec2(middleCoord.xy + vec2(1, 1))).rgb;


	vec3 final = kernel[1][1] * middle
				+ kernel[0][1] * left
				+ kernel[2][1] * right
				+ kernel[1][0] * top
				+ kernel[0][0] * topleft
				+ kernel[2][0] * topright
				+ kernel[1][2] * bottom
				+ kernel[0][2] * bottomleft
				+ kernel[2][2] * bottomright;

	return final;
}

vec3 Inverted(vec3 pixel)
{
	return vec3(1, 1, 1) - pixel;
}

