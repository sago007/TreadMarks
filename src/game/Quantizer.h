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
	Color quantization class.  Compresses multiple 256 color palettes into one.
*/

#ifndef QUANTIZER_H
#define QUANTIZER_H

#include <cstdint>

struct PaletteEntry
{
	uint32_t peRed = 0;
	uint32_t peGreen = 0;
	uint32_t peBlue = 0;
	uint32_t peFlags = 0;
};

class InversePal{
private:
	int32_t r_center, g_center, b_center;
	bool r_skip1, r_skip2, g_skip1, g_skip2, b_skip1, b_skip2;
	unsigned char *inv_pal;
	int32_t red_max, green_max, blue_max;
	int32_t gb_pow2, red_pow2, green_pow2, blue_pow2;
	int32_t r_quant, g_quant, b_quant;
	unsigned char cur_color;
	void red_init();
	void green_init();
	void blue_init();
	bool red_loop(int32_t *dbuf, unsigned char *cbuf, int32_t center, int32_t dist);
	bool green_loop(int32_t *dbuf, unsigned char *cbuf, int32_t center, int32_t dist);
	bool blue_loop(int32_t *dbuf, unsigned char *cbuf, int32_t center, int32_t dist);
public:
	PaletteEntry pe[256];
	InversePal();
	InversePal(int32_t rbits, int32_t gbits, int32_t bbits);
	~InversePal();
	bool Init(int32_t rbits, int32_t gbits, int32_t bbits);
	bool Make(PaletteEntry *pe, int32_t ncols = 256);
	void Free();
	unsigned char Lookup(unsigned char r, unsigned char g, unsigned char b) const{
		if(inv_pal){
			return *(inv_pal + ((r >>r_quant) <<gb_pow2) + ((g >>g_quant) <<blue_pow2) + (b >>b_quant));
		}
		return 0;
	};
};


//Storage and access abstraction for color mixing table.
#define MIX25 3
#define MIX50 2
#define MIX75 1
#define MIX100 0
//#define INVPALBITS 4
//#define INVPALSIZE (1 << INVPALBITS)
//#define INVPALSHIFT (8 - INVPALBITS)
//#define INVPALSHIFT2 (INVPALBITS - INVPALSHIFT)
//Shift 8bit value down this much to get inverse palette index.
#define LIGHT_SHADES 16
class MixTable{
friend class RenderEngine;
friend class PolyRender;
private:
	unsigned char Mix4[257][256][4];
	unsigned char Add1[257][256];
	unsigned char Sub1[257][256];
	unsigned char Light1[LIGHT_SHADES][256];
//	unsigned char InvPal[INVPALSIZE][INVPALSIZE][INVPALSIZE];
	InversePal InvPal;
public:
	int32_t DifferentColors;
	int32_t CachedColors;
public:
	unsigned char Mix25(unsigned char a, unsigned char b){ return Mix4[a][b][MIX25]; };
	unsigned char Mix50(unsigned char a, unsigned char b){ return Mix4[a][b][MIX50]; };
	unsigned char Mix75(unsigned char a, unsigned char b){ return Mix4[a][b][MIX75]; };
	unsigned char Mix(unsigned char a, unsigned char b, unsigned char c){
		return Mix4[a][b][c]; };
	unsigned char Add(unsigned char src, unsigned char dst){ return Add1[src][dst]; };	//Saturation arithmetic addition and subtraction.
	unsigned char Sub(unsigned char src, unsigned char dst){ return Sub1[src][dst]; };
	unsigned char Light(unsigned char src, unsigned char shade){ return Light1[shade][src]; };	//Assumes you will usually light a whole area to one shade, better cache coherency.
	bool MakeLookup(PaletteEntry *pe, bool disk = false);
	void BestColorInit(PaletteEntry *pe);
	unsigned char BestColor(PaletteEntry *pe, unsigned char r, unsigned char g, unsigned char b);
		//Set disk to true to load in old table (if palette matches) and save out table.
};

//Note because of static variable, only one Reduce(int32_t) or GetPalette() call can be going on at a time, anywhere!
//Ok, for sucking out the palette after reduction, you iterate through and copy leaves'
//colors (RtGtBt / Count) to next pep stop, and divide RtGtBt by Count and set it to 1.
//Then (after external sorting, say) to reassociate the octree to the sorted palette
//entries, feed them back in...
//This is silly!  Sort and remap AFTER making the 256 color image!  Ack!  Ok, so just
//store the index in the node (and all sub-nodes) as the palette is being pulled out.
class ColorOctree{
private:
	static int32_t ColorCount, ColorIndex;
	static PaletteEntry *pep;
	ColorOctree *Prev;
	ColorOctree *Next[8];
	PaletteEntry Split;	//The splitting plane for this node.  Root is 128,128,128.
	int32_t Size;	//The size of this node's children.  Root should have a Size of 128, the final level should have a break of 1.  Break / 2 is how much to add to the node's center when spawning shildren.
	int32_t Rt, Gt, Bt;	//Color value totals for this node, combination of all children that were reduced into this node.
	int32_t Count;	//Count of children concatenated, or 1 for a first time leaf node.  If 0, we are not a leaf.
	int32_t Index;	//Color table index for inverse palette use.
	int32_t Octant(PaletteEntry col);
	PaletteEntry SplitPlane(int32_t octant);
	void Reduce(int32_t size, int32_t cols);	//Recursively reduces only those nodes with sizes smaller than size.
	void GetPalette(int32_t index);
	void Clear();
public:
	ColorOctree();	//Creates a root node, 128,128,128.
	ColorOctree(ColorOctree *prev, PaletteEntry split, int32_t size);	//Creates a child node with splitting plane split, and child size of size.
	~ColorOctree();
	void DeleteTree();	//Deletes all children recursively, DOES NOT TOUCH this node.
	ColorOctree *Add(PaletteEntry col);	//Adds color col to tree.  If split and col match, becomes leaf node, otherwise passes on to children or makes new kids.
	int32_t CountLeaves();	//Recursively counts the color leaves in the tree, NOT all the total nodes.
	int32_t Reduce(int32_t cols);	//Only call on root node, reduces tree to only cols leaves.
	bool GetPalette(PaletteEntry *pe, int32_t cols = 256);
	int32_t LookupIndex(PaletteEntry pe);	//You can only look up exact colors that were put in the tree previously!
};

//#define MAXCOLS 10240

class Quantizer{
private:
//	PALETTEENTRY realpal1[MAXCOLS];
//	PALETTEENTRY realpal2[MAXCOLS];
//	PALETTEENTRY *pal1;
//	PALETTEENTRY *pal2;
//	int32_t npal1, npal2;
	ColorOctree octree;
public:
	Quantizer();
	~Quantizer();
	bool ClearPalette();
	bool AddPalette(PaletteEntry *pe, int32_t NumCols, int32_t NumShades = 0);
	bool GetCompressedPalette(PaletteEntry *pe, int32_t NumCols);//, int32_t level);
//	bool RemapRawBMP(RawBMP *bmp, PALETTEENTRY *pe);
};

#endif
