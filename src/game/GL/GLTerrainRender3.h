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

#ifndef GLTERRAINRENDER3_H
#define GLTERRAINRENDER3_H

#include "../Render.h"
#include "GLTerrain.h"

#define IHEIGHT_NORENDER -1

#define BINTRIPOOL 25000
#define BINTRISAFE 24000
#define BINTRIMAX  20000
//Should be even multiple of 4.

class BinTriPool;

class BinaryTriangleR3
{
public:
	BinaryTriangleR3 *LeftNeighbor, *RightNeighbor, *BottomNeighbor, *LeftChild, *RightChild;
	float SplitVertHeight;	//Split vertex height.
	union{
		float height;
		int32_t iheight;
	};
	uint32_t iIndex; // index into vertex array
	void Null(){
		LeftNeighbor = 0;
		RightNeighbor = 0;
		BottomNeighbor = 0;
		LeftChild = 0;
		RightChild = 0;
		SplitVertHeight = 0.5f;
		iheight = 0;
	};
private:
	int32_t Split2(BinTriPool *pool);	//Note, leaves, LeftChild->RightNeighbor and RightChild->LeftNeighbor hanging!
public:
	void Split(BinTriPool *pool);
	void TestSplit(LodTree *lod, int32_t variance, int32_t LimitLod, int32_t level, int32_t index, BinTriPool *pool);
	void TestSplitZ(int32_t level, int32_t LimitLod, int32_t index, float cz1, float cz2, float cz3, LodTree *lod, BinTriPool *pool);
	void TestSplitClip(int32_t level, float variance, int32_t LimitLod, int32_t index, float radius, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, float lclip[3], float rclip[3], float zclip[3], LodTree *lod, BinTriPool *pool);
};

class BinTriPool
{
private:
	BinaryTriangleR3	RealBinTriPool[BINTRIPOOL + 1];
	//BinaryTriangleR3	*AlignedBinTriPool;
	int					NextBinTriPool;

public:
	BinTriPool() {NextBinTriPool = 0; 	/*AlignedBinTriPool = (BinaryTriangleR3*)((((uint32_t)RealBinTriPool) + 31) & (~31));*/ }
	void ResetBinTriPool(){ NextBinTriPool = 0;}
	int32_t AvailBinTris(){ return BINTRIPOOL - NextBinTriPool;}
	int32_t ElectiveSplitSafe(){ return NextBinTriPool < BINTRISAFE;}
	BinaryTriangleR3 *AllocBinTri()
	{
		if(NextBinTriPool < BINTRIPOOL)
		{
			BinaryTriangleR3 *t = &RealBinTriPool[NextBinTriPool++];
			t->iheight = 0;
			return t;
		}
		return 0;
	}
	void FreeBinTri(BinaryTriangleR3 *tri){return;}
	int32_t GetNextBinTriPool() {return NextBinTriPool;}
};

//A section of world for rendering, may point32_t to wrapped terrain off-map.
struct MapPatch{
	int32_t x, y;	//Coordinates in patch grid.
	uint32_t id;
	BinaryTriangleR3 ul, dr;
	LodTree *lodul, *loddr;
	MapPatch() : x(0), y(0), id(0) {
		ul.Null(); ul.BottomNeighbor = &dr;	//Links component root bintris together at bottoms.
		dr.Null(); dr.BottomNeighbor = &ul;
	};
	MapPatch(int32_t X, int32_t Y, uint32_t ID) : x(X), y(Y), id(ID) {
		ul.Null(); ul.BottomNeighbor = &dr;	//Links component root bintris together at bottoms.
		dr.Null(); dr.BottomNeighbor = &ul;
	};
	void SetCoords(int32_t X, int32_t Y, uint32_t ID){
		x = X;
		y = Y;
		id = ID;
		lodul = LodMap.Tree(x, y, 0);
		loddr = LodMap.Tree(x, y, 1);
	};
	void LinkUp(MapPatch *p){	//Links with a map patch above.
		if(p){
			ul.RightNeighbor = &p->dr;
			p->dr.RightNeighbor = &ul;
		}
	};
	void LinkLeft(MapPatch *p){	//Ditto left.
		if(p){
			ul.LeftNeighbor = &p->dr;
			p->dr.LeftNeighbor = &ul;
		}
	};
	void Split(float variance, int32_t LimitLod, float lclip[3], float rclip[3], float zclip[3], BinTriPool *pool){
		int32_t x1 = x * TexSize, y1 = y * TexSize;
		int32_t x2 = x1 + TexSize, y2 = y1 + TexSize;
		float rad = sqrtf((float)(SQUARE(TexSize >>1) + SQUARE(TexSize >>1)));
		//
		ul.TestSplitClip(0, variance * 0.01f, LimitLod, 0, rad, x1, y2, x2, y1, x1, y1, lclip, rclip, zclip, lodul, pool);
		dr.TestSplitClip(0, variance * 0.01f, LimitLod, 0, rad, x2, y1, x1, y2, x2, y2, lclip, rclip, zclip, loddr, pool);
	};
};

class GLRenderEngine3 : public RenderEngine
{
private:
	BinTriPool	Pool;
	Terrain		*curmap;

	float		lclip[3];
	float		rclip[3];
	float		zclip[3];

	float		texoffx, texoffy;
	float		texscalex, texscaley, texaddx, texaddy;
	float		texoffx2, texoffy2, texscalex2, texscaley2;

	int32_t			PolyCount;

	int32_t			nFanStack;
	int32_t			FanCount;
	int32_t			UseMT;
	union
	{	int32_t FanStack[200];
		float FanStackf[200];
	};

	Timer		gltmr;
	int32_t			TerrainFrame;

	float		CurQuality;
	int32_t			CurBinTris;

	MapPatch	*patches;
	int32_t			npatches;

	void GLViewplane(float w, float h, float view, float n, float f);

	void BinTriVert(int32_t x, int32_t y);
	void RenderBinTri(BinaryTriangleR3 *btri, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3);
	void BinTriVert3(float x, float y, float h);
	void BinTriVert3MT(float x, float y, float h);
	void InitFanStack();
	void FlushFanStack();
	void AddFanPoint(int32_t x, int32_t y, float h);
	void AddFanPoint(int32_t x1, int32_t y1, float h1, int32_t x2, int32_t y2, float h2, int32_t x3, int32_t y3, float h3);
	void RenderBinTriFan(BinaryTriangleR3 *btri, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t sense, float h1, float h2, float h3);
public:
	GLRenderEngine3();

	bool GLTerrainRender(Terrain *map, Camera *cam, int32_t flags, float quality, int32_t ms = 0);	//Optional number of msecs for frame parameter.
	bool GLRenderWater(Terrain *map, Camera *cam, int32_t flags, float quality);
	const char *GLTerrainDriverName() {return "Tom's Experimental Terrain Driver";}
};

#endif

