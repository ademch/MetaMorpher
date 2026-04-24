#include "stdafx.h"
#include "MorphFBOprocessor.h"
#include "GLSL_Pipeline.h"

extern TextureBank  texBank;

void MorphFBOprocessor::GenMeshToListEmu(float fX_bottom, float fY_bottom, float fWidth, float fHeight, float fZ, int iHorSlices, int iVertSlices)
{
	mesh_listEmu.clear();
	for (int j = 0; j < iVertSlices; j++) {
		for (int i = 0; i <= iHorSlices; i++)
		{
			Vec2 u01 = Vecc2(fX_bottom + fWidth * (i / float(iHorSlices)), fY_bottom + fHeight * ((j + 1) / float(iVertSlices)));
			Vec2 u00 = Vecc2(fX_bottom + fWidth * (i / float(iHorSlices)), fY_bottom + fHeight * (j / float(iVertSlices)));

			//glVertex3f(u01.X, u01.Y, fZ);
			mesh_listEmu.push_back(Vecc2(u01.X, u01.Y));

			//glVertex3f(u00.X, u00.Y, fZ);
			mesh_listEmu.push_back(Vecc2(u00.X, u00.Y));
		}
	}
}

void MorphFBOprocessor::RenderFromMeshListEmu(unsigned int tex, float fX_bottom, float fY_bottom, float fWidth, float fHeight, float fZ, int iHorSlices, int iVertSlices)
{
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, tex);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glColor3f(0.0, 1.0, 0.0);

	for (int j = 0; j < iVertSlices; j++) {
		glBegin(GL_TRIANGLE_STRIP);
		for (int i = 0; i <= iHorSlices; i++)
		{
			Vec2 u01 = Vecc2(fX_bottom + fWidth * (i / float(iHorSlices)), fY_bottom + fHeight * ((j + 1) / float(iVertSlices)));
			Vec2 u00 = Vecc2(fX_bottom + fWidth * (i / float(iHorSlices)), fY_bottom + fHeight * (j / float(iVertSlices)));

			glTexCoord2f((j + 1) / float(iVertSlices), i / float(iHorSlices));
			Vec2 item = mesh_listEmu.front();
			mesh_listEmu.pop_front();
			glVertex3f(item.X, item.Y, fZ);

			glTexCoord2f(j / float(iVertSlices), i / float(iHorSlices));
			item = mesh_listEmu.front();
			mesh_listEmu.pop_front();
			glVertex3f(item.X, item.Y, fZ);
		}
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);
}

void MorphFBOprocessor::ShaderEmulate()
{
	int iWidth  = texBank[TEXTURE_FLOAT_BUFFER]->m_width;
	int iHeight = texBank[TEXTURE_FLOAT_BUFFER]->m_height;
	int nrChannels = 2;
	Vec2* dataSrc = (Vec2 *)malloc(iWidth*iHeight * sizeof(float)*nrChannels);
	Vec2* dataDst = dataSrc + iWidth;

	glBindTexture(GL_TEXTURE_2D, texBank[TEXTURE_FLOAT_BUFFER]->m_uiTextureID);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, dataSrc);

	const float FLOAT_EPS  = 0.001f;
	const float P_INFINITY = 1000000.0f;

	float fVotingRadius = 100;
	float fMorphRadius = m_ParamsSubWindow->fMorphRadius;
	float fMorphPower  = m_ParamsSubWindow->fMorphPower;
	float fMorphRatio  = m_ParamsSubWindow->fMorphRatio;

	for (auto &vPosition : mesh_listEmu)
	{
		int iMorphPointsCount = texBank[TEXTURE_FLOAT_BUFFER]->m_width;
		const float fMorphParameterA = 0.01;
		const float fMorphParameterB = 2.0f;

		float fTotalWeight = 0.0f;
		Vec2 vShift = Vecc2();
		for (int i = 0; i < iMorphPointsCount; i++)
		{
			Vec2 ptSrc  = dataSrc[i];
			Vec2 ptDst  = dataDst[i];
			Vec2 ptLerp = VecMix(ptSrc, ptDst, fMorphRatio / 100.0);

			float dist = PointDist(ptSrc, vPosition);
			float weight = 0;
			if (dist < fVotingRadius)
				weight = pow(cos(dist / fVotingRadius * PI) / 2.0f + 0.5f, fMorphPower);//pow((fMorphParameterA + dist), -fMorphParameterB);
			fTotalWeight += weight;
			vShift = vShift + weight * (ptLerp - ptSrc);
		}
		if (fTotalWeight > 0.001) {
			vShift = (1.0 / fTotalWeight)*vShift;
		}

		// 2. During the second step it is decided the fullness of travel to the voted location
		float distMin = P_INFINITY;
		for (int i = 0; i < iMorphPointsCount; i++)
		{
			Vec2 ptSrc = dataSrc[i];

			float dist = PointDist(ptSrc, vPosition);
			if (dist < distMin)
				distMin = dist;
		}

		if (distMin < fMorphRadius)
		{
			float weight = pow(cos(distMin / fMorphRadius * PI)*0.5 + 0.5, 1.0);
			vShift = weight * vShift;
		}
		else
			vShift = Vecc2();

		vPosition = vPosition + vShift;

	}
	free(dataSrc);

}