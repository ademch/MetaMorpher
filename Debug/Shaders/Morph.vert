R"(#version 140

uniform sampler2D tex1;	// src/dst morphing lists

uniform float fMorphRadius;
uniform float fMorphPower;
uniform float fMorphRatio;

const float PI = 3.14159265359;
const float FLOAT_EPS = 0.001;
const float P_INFINITY = 1000000.0;

void main(void)
{
	vec4 vPosition = gl_Vertex;
	gl_FrontColor  = vec4(gl_Color.rgb, 1.0);
	gl_TexCoord[0] = gl_MultiTexCoord0;

	int iMorphPointsCount = textureSize(tex1, 0).s;

	// 1. At the first step find a weighted combination of !directional shifts!
    //    making a single vector of full translation we believe in (consequently final destination point).
	//    The distance to the pair only decides how much to believe in it and has no
	//    meaning of how strong to travel, the travelling is full.
	//    100% of votes are spread between each !directional shift!:
	//    E.g. 0.1*Pair0 + 11*Pair1 + 100*Pair2 + 2*Pair3 => Total votes: 113.1 =>
	//    0.088% voted for full travel offered by Pair0, 9.7% for Pair1, 88% for Pair2, 1.7% for Pair3
	float fTotalWeight = 0.0;
    vec2 vShift = vec2(0.0);
    for (int i = 0; i < iMorphPointsCount; i++)
	{
		vec2 ptSrc  = texelFetch(tex1, ivec2(i, 0), 0).st;
		vec2 ptDst  = texelFetch(tex1, ivec2(i, 1), 0).st;
		vec2 ptLerp = mix(ptSrc, ptDst, fMorphRatio/100.0);

        float dist = distance(ptSrc, vPosition.xy);
        float weight = 0;
		if (dist < fMorphRadius)
			weight = pow(cos(dist/fMorphRadius*PI)/2.0 + 0.5, fMorphPower);
        fTotalWeight += weight;
		vShift += weight * (ptLerp - ptSrc);
    }
	if (fTotalWeight > FLOAT_EPS) {
		vShift *= 1.0/fTotalWeight;
    }

	// 2. During the second step it is decided the fullness of travel to the decided location
	float distMin = P_INFINITY;
	for (int i = 0; i < iMorphPointsCount; ++i)
	{
		vec2 ptSrc = texelFetch(tex1, ivec2(i, 0), 0).st;

		float dist = distance(ptSrc, vPosition.xy);
		if (dist < distMin)
			distMin = dist;
	}
       
	if (distMin < fMorphRadius)
	{
		float weight = cos(distMin/fMorphRadius * PI)*0.5 + 0.5;
		vShift *= weight;
	}
    else
		vShift = vec2(0.0, 0.0);
   
	vPosition += vec4(vShift, 0, 0);
	gl_Position = gl_ModelViewProjectionMatrix*vPosition;
}
)"