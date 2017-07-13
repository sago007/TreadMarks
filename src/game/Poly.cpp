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

//Polygon renderer for voxel terrain integration.

/*
ToDo:
	? Load textures from LWO file.
		...Maybe not...
	+ Add cylindrical mapping function, and sub-project to output a bitmap with the unfolded
		polygons outlined on it for easy texture painting.
	- Optimize texture mapper, perhaps span based.
	- Try span rendering on voxels too, use two ybufs and two cbufs, and just swap
		pointers after each line of Z.  Render to new ybuf/cbuf, then fill in vertical lines
		between new ybuf/cbuf and old ybuf/cbuf.  Use tight ASM for both seperate loops.
	- Shadows!  Project models onto ground using temporary shadow buffer, then transparent/
		darken map that onto the terrain using a flat polygon rotated to the terrain's
		contours.
	- Either cylindrical projector or polygon transformation/rendering is flipped through the
		X axis...  Have a closer look with a specially created object later.
*/

//#define ASM_SPRITE

#include "Poly.h"
//#include <math.h>
//#include <stdio.h>
#include "IFF.h"
#include "CfgParse.h"
#include <cmath>
#include <cstdio>

using namespace std;

PolyRender::PolyRender(){
//	for(int i = 0; i < MAX_YSHEAR; i++){
//		YShear[i] = 0;
//	}
//	nObject = nTPoly = nTVertex = 0;
//	YBufUsed = 0;
	LODLimit = 0;
//	DisableParticles = 0;
	ParticleAtten = 1.0f;
	DisableFog = false;
	ShowPolyNormals = false;
	EnvMap1 = EnvMap2 = NULL;
	//
	AlphaTestOn = false;
}
PolyRender::~PolyRender(){
}
bool PolyRender::SetEnvMap1(Image *env1){
	if(env1){// && env1->id){//Data()){
		EnvMap1 = env1;
		return true;
	}
	return false;
}
bool PolyRender::SetEnvMap2(Image *env2){
	if(env2){// && env2->id){//Data()){
		EnvMap2 = env2;
		return true;
	}
	return false;
}
void PolyRender::UnlinkTextures(){
	EnvMap1 = EnvMap2 = 0;
}
bool PolyRender::LimitLOD(int level){
	if(level >= 0){
		LODLimit = level;
		return true;
	}else{
		return false;
	}
}
void PolyRender::Particles(float atten){//int yesno){
//	DisableParticles = !yesno;
	ParticleAtten = atten;
}
void PolyRender::Fog(bool yesno){
	DisableFog = !yesno;
}
void PolyRender::AlphaTest(bool yesno){
	AlphaTestOn = yesno;
}
void PolyRender::PolyNormals(bool yesno){
	ShowPolyNormals = yesno;
}
void PolyRender::SetLightVector(float lx, float ly, float lz, float amb){
	float l = sqrtf(lx * lx + ly * ly + lz * lz);
	if(l > 0.0f){
		LightX = lx / l;
		LightY = ly / l;
		LightZ = lz / l;
	}
	Ambient = amb;
}

bool PolyRender::InitRender(){
	//
	nSolidObject = 0;
	nTransObject = 0;
	nOrthoObject = 0;
	//
	nSecondaryObject = 0;
	//
	//Force state functions to re-set state on first call.
	GLResetStates();
	//
	return true;
}

bool PolyRender::SetupCamera(Camera *Cm){
	if(Cm) Cam = *Cm;
	//
	float bv[2] = {sin(Cam.b * DEG2RAD), cos(Cam.b * DEG2RAD)};
    float tv[2];
    tv[0] = -Cam.viewwidth / 2.0f;
    tv[1] = Cam.viewplane;
	float s = sqrtf(SQUARE(tv[0]) + SQUARE(tv[1]));
	s = std::max(0.001f, s);
	//
	//Plane equations (signed dist = x * a + y * b + c) for left, right, and z clipping planes.
	//
	lplane[0] = (tv[0] * (-bv[0]) + tv[1] * (bv[1])) / s;	//X coord of X basis vect, X coord of Y basis vect.
	lplane[1] = (tv[0] * (-bv[1]) + tv[1] * (-bv[0])) / s;	//Y coord of X basis vect, Y coord of Y basis vect.
	lplane[2] = -(lplane[0] * Cam.x + lplane[1] * Cam.z);
	//
	rplane[0] = ((-tv[0]) * (bv[0]) + tv[1] * (-bv[1])) / s;	//X coord of X basis vect, X coord of Y basis vect.
	rplane[1] = ((-tv[0]) * (bv[1]) + tv[1] * (bv[0])) / s;	//Y coord of X basis vect, Y coord of Y basis vect.
	rplane[2] = -(rplane[0] * Cam.x + rplane[1] * Cam.z);
	//
	zplane[0] = bv[0];
	zplane[1] = bv[1];
	zplane[2] = -(zplane[0] * Cam.x + zplane[1] * Cam.z);
	//
	//Create View matrix.
	Rot3 rot = {Cam.a * DEG2RAD, Cam.b * DEG2RAD, Cam.c * DEG2RAD};
	Rot3ToMat3(rot, view);
#define SWAP(a, b) (t = (a), (a) = (b), (b) = t)
	float t;
	SWAP(view[0][1], view[1][0]);
	SWAP(view[0][2], view[2][0]);	//Invert matrix after making with non-negative rots.
	SWAP(view[1][2], view[2][1]);	//Hah, it works!
	//
	Vec3 p = {-Cam.x, -Cam.y, -Cam.z};
	Vec3MulMat3(p, view, view[3]);	//To do the translate then rotate the view transform requires, must rotate translation amount by rotation amount.
	//
	return true;
}

bool PolyRender::AddSolidObject(Object3D *obj){
	if(nSolidObject < MAX_RENDEROBJ && obj){
		//
		float l = (obj->Pos[0] * lplane[0] + obj->Pos[2] * lplane[1] + lplane[2]) + obj->BndRad;
		float r = (obj->Pos[0] * rplane[0] + obj->Pos[2] * rplane[1] + rplane[2]) + obj->BndRad;
		float z = (obj->Pos[0] * zplane[0] + obj->Pos[2] * zplane[1] + zplane[2]) - obj->BndRad;
		//
		if(l > 0.0f && r > 0.0f && z < Cam.farplane){
			SolidObjectP[nSolidObject++] = obj;
			return true;
		}
		//
	}
	return false;
}
bool PolyRender::AddSecondaryObject(Object3D *obj){
	if(nSecondaryObject < MAX_RENDEROBJ && obj){
		//
	//	float l = (obj->Pos[0] * lplane[0] + obj->Pos[2] * lplane[1] + lplane[2]) + obj->BndRad;
	//	float r = (obj->Pos[0] * rplane[0] + obj->Pos[2] * rplane[1] + rplane[2]) + obj->BndRad;
	//	float z = (obj->Pos[0] * zplane[0] + obj->Pos[2] * zplane[1] + zplane[2]) - obj->BndRad;
		//
	//	if(l > 0.0f && r > 0.0f && z < Cam.farplane){
		SecondaryObjectP[nSecondaryObject++] = obj;
		return true;
	//	}
		//
	}
	return false;
}
bool PolyRender::AddTransObject(Object3D *obj){
	if(nTransObject < MAX_RENDEROBJ && obj){
		//
		//Perform coarse culling here.
		float l = (obj->Pos[0] * lplane[0] + obj->Pos[2] * lplane[1] + lplane[2]) + obj->BndRad;
		float r = (obj->Pos[0] * rplane[0] + obj->Pos[2] * rplane[1] + rplane[2]) + obj->BndRad;
		float z = (obj->Pos[0] * zplane[0] + obj->Pos[2] * zplane[1] + zplane[2]) - obj->BndRad;
		//
		if(l > 0.0f && r > 0.0f && z < Cam.farplane){
			obj->SortKey = FastFtoL(z);
			TransObjectP[nTransObject++] = obj;
			return true;
		}
		//
	}
	return false;
}
bool PolyRender::AddOrthoObject(ObjectOrtho *obj){
	if(nOrthoObject < MAX_RENDEROBJ && obj){
		OrthoObjectP[nOrthoObject++] = obj;
		return true;
	}
	return false;
}

//Ok, now ALL objects allocated off the farm will have pointers added to the ObjectP
//list, child/linked objects will be connected back to the most recent top level object
//added, as well as having the most previous farm object (whatever it is) linked to
//it.  So any child can reference back to the parent, and from there the child links
//can be walked to descend all children.  Thus all objects can be clipped and ybuffered
//independantly, yet as soon as any children come up for rendering the whole set will
//be rendered.
/*
inline Object *PolyRender::AddFarmObj(int type, bool link){
	if(ObjectFarmUsed < MAX_FARMOBJ){
		Object *obj = &ObjectFarm[ObjectFarmUsed];
		obj->Type = type;
	//	obj->M = mesh;
		if(link && ObjectFarmUsed > 0){
			ObjectFarm[ObjectFarmUsed - 1].Link = obj;
			obj->Parent = ParentPtr;
			obj->Link = NULL;
		}else{
			obj->Link = obj->Parent = NULL;
			ParentPtr = obj;
		}
		ObjectFarmUsed++;
		if(nObject < MAX_RENDEROBJ){	//Add to pointer list.
			ObjectP[nObject++] = obj;
			return obj;
		}
	}
	return NULL;
}
*/
/*
bool PolyRender::AddMesh(Mesh *mesh, Vec3 pos, Rot3 rot, bool link, int flags, float opacity){
	Object *obj;
	if(mesh && pos && rot && (obj = AddFarmObj(OBJECT_POLY, link))){
		obj->Flags = flags;
		obj->M = mesh;
		obj->Opacity = opacity;
		Rot3ToMat3(rot, obj->model);
		CopyVec3(pos, obj->model[3]);
		return true;
	}
	return false;
}
bool PolyRender::AddMeshMat(Mesh *mesh, Mat43 mat, bool link, int flags, float opacity){
	Object *obj;
	if(mesh && mat && (obj = AddFarmObj(OBJECT_POLY, link))){
		obj->Flags = flags;
		obj->M = mesh;
		obj->Opacity = opacity;
		memcpy(obj->model, mat, sizeof(Mat43));
		return true;
	}
	return false;
}
bool PolyRender::AddSprite(Image *img, Float w, Float h, Vec3 pos, bool link, int flags, float opacity){
	Object *obj;
	if(img && pos && (obj = AddFarmObj(OBJECT_SPRITE, link))){
		obj->Flags = flags;
		obj->Sprite = img;
		obj->SpriteWidth = w;
		obj->SpriteHeight = h;
		obj->Opacity = opacity;
		IdentityMat43(obj->model);
		CopyVec3(pos, obj->model[3]);
		return true;
	}
	return false;
}
bool PolyRender::AddParticle(unsigned char r, unsigned char g, unsigned char b, unsigned char seed,
		Vec3 pos, float size, int flags, float opacity){
	Object *obj;
	if(pos && (obj = AddFarmObj(OBJECT_PARTICLE, false))){
		obj->Flags = flags;
		obj->Radius = size;
		obj->Red = r;
		obj->Green = g;
		obj->Blue = b;
		obj->Seed = seed;
		obj->Opacity = opacity;
		IdentityMat43(obj->model);
		CopyVec3(pos, obj->model[3]);
		return true;
	}
	return false;
}
bool PolyRender::AddText(Image *img, int charsx, int charsy, Float x, Float y, Float w, Float h,
		unsigned const char *glyphs, int count, float opac,
		unsigned char r, unsigned char g, unsigned char b){
	if(img && glyphs && nTextObject < MAX_TEXTOBJECT && count > 0 && count < MAX_TEXTLEN){
		memcpy(Text[nTextObject].Glyph, glyphs, count);
		Text[nTextObject].nGlyph = count;
		Text[nTextObject].Bmp = img;
		Text[nTextObject].W = w;
		Text[nTextObject].H = h;
		Text[nTextObject].x = x;
		Text[nTextObject].y = y;
		Text[nTextObject].Red = r;
		Text[nTextObject].Green = g;
		Text[nTextObject].Blue = b;
		Text[nTextObject].CharsX = charsx;
		Text[nTextObject].CharsY = charsy;
		Text[nTextObject].Opacity = opac;
		nTextObject++;
		return true;
	}
	return false;
}
*/

int CDECL CompareObject3DKeys(const void *obj1, const void *obj2){
	if((*(Object3D**)obj1)->SortKey < (*(Object3D**)obj2)->SortKey) return 1;
	if((*(Object3D**)obj1)->SortKey > (*(Object3D**)obj2)->SortKey) return -1;
	return 0;
}
int CDECL CompareObject2DKeys(const void *obj1, const void *obj2){
	if((*(ObjectOrtho**)obj1)->Z < (*(ObjectOrtho**)obj2)->Z) return 1;
	if((*(ObjectOrtho**)obj1)->Z > (*(ObjectOrtho**)obj2)->Z) return -1;
	return 0;
}

bool LineMapObject::AddLine(float X1, float Y1, float X2, float Y2, float R, float G, float B){
	if(nLines < MAX_LINEMAP){
		Lines[nLines].x1 = X1; Lines[nLines].x2 = X2; Lines[nLines].y1 = Y1; Lines[nLines].y2 = Y2;
		Lines[nLines].r = R; Lines[nLines].g = G; Lines[nLines].b = B;
		nLines++;
		return true;
	}
	return false;
}
bool LineMapObject::AddPoint(float X, float Y, float Size, float R, float G, float B, float Ramp){
	if(nPoints < MAX_LINEMAP){
		Points[nPoints].x = X; Points[nPoints].y = Y; Points[nPoints].size = Size;
		Points[nPoints].r = R; Points[nPoints].g = G; Points[nPoints].b = B; Points[nPoints].ramp = Ramp;
		nPoints++;
		return true;
	}
	return false;
}


void MeshObject::Render(PolyRender *PR){
	PR->GLRenderMeshObject(this, PR);
}
void SpriteObject::Render(PolyRender *PR){
	PR->GLRenderSpriteObject(this, PR);
}
void ParticleCloudObject::Render(PolyRender *PR){
	PR->GLRenderParticleCloudObject(this, PR);
}
void StringObject::Render(PolyRender *PR){
	PR->GLRenderStringObject(this, PR);
}
void LineMapObject::Render(PolyRender *PR){
	PR->GLRenderLineMapObject(this, PR);
}
void TilingTextureObject::Render(PolyRender *PR){
	PR->GLRenderTilingTextureObject(this, PR);
}
void Chamfered2DBoxObject::Render(PolyRender *PR){
	PR->GLRenderChamfered2DBoxObject(this, PR);
}
