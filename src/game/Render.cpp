// This file is part of Tread Marks
// 
// Tread Marks is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Tread Marks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Tread Marks.  If not, see <http://www.gnu.org/licenses/>.

/*
	RenderEngine - voxel terrain rendering engine class.
	By Seumas McNally
*/

//#define ASM_SMOOTH
//#define ASM_FLAT
//#define ASM_INTERPOLATE

#include "VoxelAfx.h"
#include <cmath>
#include <cstdio>
#include "Image.h"
#include "Terrain.h"
#include "Render.h"
#include "CamRbd.h"

using namespace std;

RenderEngine::RenderEngine(){
	Sky = NULL;
	Detail = NULL;
	DetailMT = NULL;
	Water = NULL;
	WaterAlpha = 0.75f;
	WaterReflect = 0.5f;
	FogR = FogG = FogB = 0.6f;
	msecs = 0;
	UseSkyBox = false;
	for(int32_t i = 0; i < 6; i++) SkyBox[i] = NULL;
	skyrotatespeed = 0.0f;
	detailrotrad = 0.0f;
	detailrotspeed = 0.0f;

//	LastZlVp = 0;
//	Clookup[0] = FPVAL;
//	for(int32_t c = 1; c < 1024; c++){	//Voxel column-width-scalar lookup table.
//		Clookup[c] = FPVAL / c;
//	}
//	for(int32_t m = 2; m < 1024; m++){
//		Mlookup[m] = 256 / m;
//		SMlookup[m] = std::min((256 * 4) / m, 255);
//	}
//	Mlookup[0] = Mlookup[1] = 255;
//	SMlookup[0] = SMlookup[1] = 255;

//	for(int32_t z = 0; z < MAX_Z; z++){
//		Zoffset[z] = 0;
//	}

	RedMask = RedMaskLen = RedMaskOff = 0;
	GreenMask = GreenMaskLen = GreenMaskOff = 0;
	BlueMask = BlueMaskLen = BlueMaskOff = 0;
#ifdef MARKMIXTABLE
	for(int32_t a = 0; a < 256; a++){
		for(int32_t b = 0; b < 256; b++){
			MarkMix[a][b] = 0;
		}
	}
#endif
}
RenderEngine::~RenderEngine(){
}
void RenderEngine::UnlinkTextures(){
	SkyENV = NULL;
	Sky = NULL;
	for(int32_t i = 0; i < 6; i++) SkyBox[i] = NULL;
	DetailMT = NULL;
	Detail = NULL;
	Water = NULL;
	UseSkyBox = false;
}
bool RenderEngine::SetSkyRotate(float rotspeed){
	skyrotatespeed = rotspeed;
	return true;
}
bool RenderEngine::SetDetailRot(float rotspeed, float rotrad){
	detailrotspeed = rotspeed;
	detailrotrad = rotrad;
	return true;
}
bool RenderEngine::SetSkyTexture(Image *sky, Image *skyenv){
	if(skyenv){// && skyenv->Data()){
		SkyENV = skyenv;
	}else{
		SkyENV = NULL;
	}
	if(sky){// && sky->Data()){
		Sky = sky;
		UseSkyBox = false;
		return true;
	}else{
		Sky = NULL;
		return false;
	}
}
bool RenderEngine::SetSkyBoxTexture(int32_t face, Image *tex){
	if(face >= 0 && face < 6 && tex){
		SkyBox[face] = tex;
		UseSkyBox = true;
		return true;
	}
	return false;
}

bool RenderEngine::SetDetailTexture(Image *detail, Image *detailmt){
	if(detailmt){// && detailmt->Data()){
		DetailMT = detailmt;
	}else{
		DetailMT = NULL;
	}
	if(detail){//->Data()){
		Detail = detail;
		return true;
	}else{
		Detail = NULL;
		return false;
	}
}
bool RenderEngine::SetWaterTexture(Image *water, float wateralpha, float waterscale, float waterenv){
	WaterAlpha = wateralpha;
	WaterScale = waterscale;
	WaterReflect = waterenv;
	if(water){// && water->Data()){
		Water = water;
		return true;
	}else{
		Water = NULL;
		return false;
	}
}
bool RenderEngine::SetFogColor(float r, float g, float b){
	FogR = r;
	FogG = g;
	FogB = b;
	return true;
}

bool RenderEngine::Init16BitTables(int32_t rmask, int32_t gmask, int32_t bmask, PaletteEntry *pe, unsigned short *C816){
	RedMask = (rmask &= 0xffff);
	GreenMask = (gmask &= 0xffff);
	BlueMask = (bmask &= 0xffff);
	do{
		if(rmask & 1){ RedMaskLen++; }else{ RedMaskOff++; }
		rmask = rmask >>1;
	}while(rmask > 0);
	do{
		if(gmask & 1){ GreenMaskLen++; }else{ GreenMaskOff++; }
		gmask = gmask >>1;
	}while(gmask > 0);
	do{
		if(bmask & 1){ BlueMaskLen++; }else{ BlueMaskOff++; }
		bmask = bmask >>1;
	}while(bmask > 0);
	RedMaskLen2 = RedMaskLen * 2;
	GreenMaskLen2 = GreenMaskLen * 2;
	BlueMaskLen2 = BlueMaskLen * 2;
	RedMaskOff2 = RedMaskOff + RedMaskLen;
	GreenMaskOff2 = GreenMaskOff + GreenMaskLen;
	BlueMaskOff2 = BlueMaskOff + BlueMaskLen;
	RedMask2 = (RedMask << RedMaskOff2) | (RedMask << RedMaskOff);
	GreenMask2 = (GreenMask << GreenMaskOff2) | (GreenMask << GreenMaskOff);
	BlueMask2 = (BlueMask << BlueMaskOff2) | (BlueMask << BlueMaskOff);
	int32_t col;
	for(int32_t c = 0; c < 256; c++){
		Color8to16[c] = col = ((pe[c].peRed >> (8 - RedMaskLen)) << RedMaskOff) |
								((pe[c].peGreen >> (8 - GreenMaskLen)) << GreenMaskOff) |
								((pe[c].peBlue >> (8 - BlueMaskLen)) << BlueMaskOff);
		Color8to16x[c] = ((col & RedMask) << RedMaskOff2) |
							((col & GreenMask) << GreenMaskOff2) |
							((col & BlueMask) << BlueMaskOff2);
		//This lets the caller get a copy of the lookup table in their own array.
		if(C816) C816[c] = col;
	}
	return true;
}


#define plot(x,y,v) (*((UCHAR*)rbd->data + (x) + (y) * lPitch) = v)

//		x2 = (x * cos(a)) - (y * sin(a));
//		y2 = (x * sin(a)) + (y * cos(a));
void RotateIntVect(int32_t x, int32_t y, int32_t *xd, int32_t *yd, float a){
	int32_t sn = (int32_t)(sin(a * (float)DEG2RAD) * (float)FPVAL);
	int32_t cs = (int32_t)(cos(a * (float)DEG2RAD) * (float)FPVAL);
	*xd = (x * cs - y * sn) >>FP;
	*yd = (y * cs + x * sn) >>FP;
}
