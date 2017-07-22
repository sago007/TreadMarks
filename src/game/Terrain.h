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
	Terrain Class - holds height and color map data for voxel rendering.
*/

//
// March 27th, 1999.
// With the software renderer looking like a sure fire no-go, I'm going
// to try turning the "Color Map" byte array into a Dark Map, which will
// contain the intensity of the color on that texel, _seperate_ from the
// lighting color, so by default all cmap voxels will be 255, and will
// only be decreased to add specifically dark areas such as craters.
// This will be used when doing dynamic terrain modification, so it is
// easy to re-light the terrain after morphing, and yet keep the correct
// blending of multiple crater scorches.  Possible dynamic lighting will
// be done seperately, with a re-texture-lighting pass to erase the
// dynamic light.  The downside is that completely arbitrary painting
// on terrain wouldn't work, unless there was a totally seperate paint
// color texture layer...  Or maybe if instead of a Dark Map, it was
// a color mapped detail addition map, with ranges of palette entries
// for different colors as well as darkness??  Hmm.
//

#ifndef TERRAIN_H
#define TERRAIN_H

#include "Image.h"
#include "IFF.h"
#include "Quantizer.h"

#define TFORM_FRACTAL 1
#define TFORM_SPLINE 2

#define NUM_SHADE 64
#define NUM_SHADED2 32
#define NUM_SHADE_SHIFT 6
#define NUM_SQRT 10000

#define TERRAIN_ENCODE_RAW 1
#define TERRAIN_ENCODE_QUADTREE 2

#define TEXID_ENCODE_RAW 1
#define TEXID_ENCODE_RLE 2

struct AvgRng{
	int32_t	a, r;
};

//Arrgh.  ::sigh::  Ok, we'll have the World Editable Values, and the internal chewed on
//values best for texturing with...  All actions go through public interface, except Terrain texturing.
class EcoSystem{
friend class Terrain;
private:
	double minA, minSlope1, maxSlope1;	//Altitude is a percentage from 0 to 100.
	double maxA, minSlope2, maxSlope2;	//Slope is an angle from 0 to 90.
public:
	double minAlt, minAngle1, maxAngle1;
	double maxAlt, minAngle2, maxAngle2;
	Image *tex;
	char name[256];
	EcoSystem();
	~EcoSystem();
	void Init(double, double, double, double, double, double, Image *t = NULL, char *n = NULL);
	double MinAlt(){ return minAlt; };
	double MaxAlt(){ return maxAlt; };
	double MinAngle1(){ return minAngle1; };
	double MinAngle2(){ return minAngle2; };
	double MaxAngle1(){ return maxAngle1; };
	double MaxAngle2(){ return maxAngle2; };
	void MinAlt(double a);
	void MaxAlt(double a);
	void MinAngle1(double a);
	void MinAngle2(double a);
	void MaxAngle1(double a);
	void MaxAngle2(double a);
	EcoSystem &operator=(EcoSystem &eco);
};

//OpenGL (well, general polygon rendering) related stuff.
#define MAX_LOD_BUMP 1000
#define TEXIDSIZE 32
struct ByteRect{
	unsigned char x, y, w, h;
	ByteRect() : x(0), y(0), w(0), h(0) {};
	ByteRect(unsigned char X, unsigned char Y, unsigned char W, unsigned char H) : x(X), y(Y), w(W), h(H) {};
};
//
class Terrain{
friend class RenderEngine;
public:	//OpenGL functions.
	bool MapLod();
	bool MapLod(int32_t x, int32_t y, int32_t w, int32_t h);
		//Flags areas of high irregularity for increased LOD at distance.
	bool DownloadTextures();//int32_t paltextures = 0);	//Set to 1 to attempt to use palettized textures.
		//Downloads texture segments for terrain to OpenGL.  Must call TextureLight32 first!
	void UndownloadTextures();
	int32_t UpdateTextures(int32_t x1, int32_t y1, int32_t w, int32_t h);
	int32_t MakeTexturePalette(EcoSystem *eco, int32_t numeco);	//Makes a map-specific texture palette, for use with paletted terrain textures.
	void UsePalettedTextures(bool usepal);
public:	//ick...  public...
	uint32_t TexIDs[TEXIDSIZE][TEXIDSIZE];
	ByteRect TexDirty[TEXIDSIZE][TEXIDSIZE];
	int32_t Redownload(int32_t tx, int32_t ty);
	//
protected:
#define INVPALBITS 6
	InversePal TerrainInv;
	Quantizer TerrainQuant;
	int32_t PalTextures;
protected:
//	unsigned char	*hdata;		//Height data
//	unsigned char	*cdata;		//Color data
	unsigned short	*data;	//Storage for height and color data interleaved.
	unsigned char	*texid;	//TextureID number map.
	int32_t		width;
	int32_t		height;
	int32_t		widthmask;
	int32_t		heightmask;
	int32_t		widthpow;
	int32_t		heightpow;
	unsigned char	ShadeLookup[256][NUM_SHADE];
	unsigned char	ShadeLookup32[256][NUM_SHADE];
	float	LightX, LightY, LightZ;	//Vector TOWARDS global sun, in +Z = forwards coords.
	float	Ambient;
	int32_t		ScorchTex;
public:
	uint32_t	*data32;
public:
	void RotateEdges();	//Rotates edges into center or vice versa for painting wrappability.
	Terrain();
	~Terrain();
	bool Init(int32_t x, int32_t y);
	bool Clear();
	bool Free();
	bool Load(IFF *iff, int32_t *numecoptr, EcoSystem *eco, int32_t maxeco, bool mirror = false);
	//Use IFF taking versions of Load and Save to tack other data on end, such as Entities.
	bool Load(const char *name, int32_t *numecoptr, EcoSystem *eco, int32_t maxeco, bool mirror = false);
	bool Save(IFF *iff, EcoSystem *eco, int32_t numeco);
	bool Save(const char *name, EcoSystem *eco, int32_t numeco);
	//
	void SetLightVector(float lx, float ly, float lz, float amb = 0.25f);
	int32_t ClearCMap(unsigned char val);
	void SetScorchEco(int32_t eco = -1);	//Sets an eco system to use as the Scorch texture, instead of black.
	int32_t GetScorchEco();
	//
	//bool FractalForm(int32_t level, int32_t min, int32_t max, int32_t Form1, int32_t Form2, EcoSystem *eco, int32_t numeco, void (*Stat)(Terrain*const, const char*, int32_t) );
	bool InitTextureID();
	int32_t EcoTexture(EcoSystem *eco, int32_t numeco, int32_t x, int32_t y);	//EcoSystems the TexID for a specific point.
	bool Texture(EcoSystem *eco, int32_t numeco, bool UseIDMap = false, int32_t x1 = 0, int32_t y1 = 0, int32_t x2 = 0, int32_t y2 = 0);
	bool RemapTexID(unsigned char *remap);	//256 entry remapping table.
	bool Lightsource(int32_t x1 = 0, int32_t y1 = 0, int32_t x2 = 0, int32_t y2 = 0);
	int32_t Blow(unsigned char *dest, int32_t dw, int32_t dh, int32_t dp,
		int32_t sx = 0, int32_t sy = 0, int32_t sx2 = 0, int32_t sy2 = 0, int32_t flag = 0, int32_t destbpp = 8, PaletteEntry pe[256] = NULL);
		//sx, sy, sx2, sy2 offsets are used to blit a specific rectangle of the map data to
		//the dest surface.  THE TOP LEFT OF THE MAP IS STILL EQUAL TO THE TOP LEFT OF THE
		//dest POINTER REGION!  This is designed for a dest region that is 1 to 1 with the
		//map in size.  flag specifies 1 for height or 0 for color map data.
	float GetSlope(int32_t x, int32_t y, float lx, float ly, float lz);
	int32_t Width(){ return width; };
	int32_t Height(){ return height; };
	bool MakeShadeLookup(InversePal *inv);//PALETTEENTRY *pe);
	//float lx = -0.7071, float ly = 0.7071, float lz = 0,
	bool TextureLight32(EcoSystem *eco, int32_t numeco, int32_t x1 = 0, int32_t y1 = 0, int32_t x2 = 0, int32_t y2 = 0, bool UseShadeMap = true);
	int32_t WrapX(int32_t x) {return x & widthmask;}
	int32_t WrapY(int32_t y) {return y & heightmask;}
	//
#define CALCIDX(x, y) (((x) & widthmask) + (((y) & heightmask) << widthpow))
	int32_t GetRGB(int32_t x, int32_t y, unsigned char *r, unsigned char *g, unsigned char *b){
		if(data32){
			unsigned char *t = (unsigned char*)(data32 + CALCIDX(x, y));
			*r = t[0];
			*g = t[1];
			*b = t[2];
			return 1;
		}
		return 0;
	}
	void PutRGB(int32_t x, int32_t y, unsigned char r, unsigned char g, unsigned char b){
		if(data32){
			unsigned char *t = (unsigned char*)(data32 + CALCIDX(x, y));
			t[0] = r;
			t[1] = g;
			t[2] = b;
		}
	}
	unsigned char GetT(int32_t x, int32_t y){
		if(!texid) return 0;
		return *(texid + CALCIDX(x,y));
	};
	unsigned char GetTraw(int32_t x, int32_t y){
		return *(texid + x + (y <<widthpow)); };
	//	return *(texid + x + y * width); };
	unsigned char GetTwrap(int32_t x, int32_t y){
		if(!texid) return 0;
		return *(texid + CALCIDX(x,y));
	};
	unsigned char GetH(int32_t x, int32_t y){
		if(!data) return 0;
		return *(data + CALCIDX(x,y)) >>8;
	};
		//Oops, already had a seperate wrapping GetH...  Hmm...
		//This'll need to be straightened out at some point.
	unsigned char GetHraw(int32_t x, int32_t y){
		return *((unsigned char*)data + ((x + (y <<widthpow)) <<1) + 1); };
	//	return *(data + x + y * width) >> 8; };
	unsigned char GetHwrap(int32_t x, int32_t y){
	//	return *(data + (x & widthmask) + ((y & heightmask) <<widthpow)) >> 8; };
		if(!data) return 0;
		return *((unsigned char*)data + (CALCIDX(x, y) <<1) + 1); };
	unsigned short GetBigH(int32_t x, int32_t y);
	unsigned char GetC(int32_t x, int32_t y){
		if(data == NULL || x < 0 || y < 0 || x >= width || y >= height) return 0;
		return *(data + x + y * width) & 0xff; };
	unsigned char GetCraw(int32_t x, int32_t y){
		return *((unsigned char*)data + ((x + (y <<widthpow)) <<1)); };
	unsigned char GetCwrap(int32_t x, int32_t y){
		if(!data) return 0;
		return *((unsigned char*)data + (CALCIDX(x, y) <<1)); 
        };
	//SetH now bounds checks the input value, and takes an int instead of a char.
	bool SetT(int32_t x, int32_t y, int32_t v){
		if(texid == NULL || x < 0 || y < 0 || x >= width || y >= height) return false;
		*(texid + x + y * width) = v;
		return true; };
	void SetTraw(int32_t x, int32_t y, int32_t v){
		*(texid + x + y * width) = v; };
	bool SetH(int32_t x, int32_t y, int32_t v){
		if(data == NULL || x < 0 || y < 0 || x >= width || y >= height) return false;
		*(data + x + y * width) = GetCraw(x, y) | ((v > 255 ? 255 : (v < 0 ? 0 : v)) << 8);
		return true; };
	void SetHraw(int32_t x, int32_t y, int32_t v){
		*(data + x + y * width) = GetCraw(x, y) | ((v > 255 ? 255 : (v < 0 ? 0 : v)) << 8); };
	void SetHwrap(int32_t x, int32_t y, int32_t v){
		if(!data) return;
	//	*(data + CALCIDX(x, y)) = GetCwrap(x, y) | ((v > 255 ? 255 : (v < 0 ? 0 : v)) << 8); };
		*((unsigned char*)data + (CALCIDX(x, y) <<1) + 1) = ((v > 255 ? 255 : (v < 0 ? 0 : v))); };
		//
	bool SetBigH(int32_t x, int32_t y, unsigned short v);
	bool SetC(int32_t x, int32_t y, unsigned char v){
		if(data == NULL || x < 0 || y < 0 || x >= width || y >= height) return false;
		*(data + x + y * width) = (GetH(x, y) << 8) | v;  return true; };
	bool SetCraw(int32_t x, int32_t y, unsigned char v){
		*(data + x + y * width) = (GetHraw(x, y) << 8) | v;  return true; };
	void SetCwrap(int32_t x, int32_t y, unsigned char v){
		if(!data) return;
		*((unsigned char*)data + (CALCIDX(x, y) <<1)) = v; };
		//
	int32_t GetI(int32_t x, int32_t y);	//Get an interpolated height, passing in FP x,y and getting back VP sized voxel height data.
	float FGetI(float x, float y);	//Get an interpolated height, passing in FP x,y and getting back VP sized voxel height data.
protected:
	AvgRng Average(int32_t x, int32_t y, int32_t p);
	int32_t FastSqrt(int32_t x, int32_t y, int32_t z);
};

#endif
