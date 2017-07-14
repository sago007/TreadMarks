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

//
//These classes allocate no memory, they are only for working on existing memory blocks.
//

#include "BitPacking.h"

BitPackEngine::BitPackEngine(char *buffer, int32_t buflength){
	data = (unsigned char*)buffer;
	maxlen = buflength;
	bitlen = buflength * 8;
	pos = 0;
}
BitPackEngine::BitPackEngine(unsigned char *buffer, int32_t buflength){
	data = (unsigned char*)buffer;
	maxlen = buflength;
	bitlen = buflength * 8;
	pos = 0;
}
BitPackEngine::~BitPackEngine(){
	//
}
int32_t BitPackEngine::BytesUsed(){	//Returns number of bytes used in buffer.
	if(data && maxlen > 0) return (pos + 7) / 8;	//0 bits will be 0 bytes, 1 bit will be 1 byte, 8 bits will be 1 bytes, 9 bits 2 bytes, etc.
	return 0;
}
int32_t BitPackEngine::BitsUsed(){	//Returns number of bits used in buffer.
	if(data && maxlen > 0) return pos;
	return 0;
}
int32_t BitPackEngine::SkipBits(int32_t bits){
	if(pos + bits <= bitlen && pos + bits >= 0){
		pos += bits;
		return 1;
	}
	return 0;
}
void BitPackEngine::Reset(){
	pos = 0;
}
int32_t BitPackEngine::PackInt(int32_t val, int32_t bits){
	if(data && bitlen > 0 && bits <= bitlen - pos && bits <= 32){
		val <<= (BPW - bits);	//BitsPerWord - bits shifts meaningful data to "top" of variable.
		int32_t bytepos = pos >>3;
		int32_t bitpos = pos;	//Save working position.
		int32_t freebits = 8 - (bitpos & 7);	//Empty bits in current byte.
		pos += bits;	//Increment internal pos now for next call.
		if(freebits < 8){	//Partial byte free.
			unsigned char mask = (uint32_t)((1 << (freebits - 0)) - 1);	//Mask for lowe end of bit yet unused.
			data[bytepos] = (data[bytepos] & (~mask)) | ((unsigned char)(val >> (BPW - freebits)) & mask);	//Save good part of data, or with value.
			val <<= freebits;
			bits -= freebits;
			bytepos++;
		}
		while(bits > 0){
			data[bytepos++] = (unsigned char)(val >> (BPW - 8));
			val <<= 8;
			bits -= 8;
		}
		return 1;
	}
	return 0;
}
int32_t BitPackEngine::PackUInt(int32_t val, int32_t bits){
	return PackInt(val, bits);	//Signed vs. unsigned only matters when unpacking!
}
int32_t BitPackEngine::PackFloatInterval(float val, float min, float max, int32_t bits){
	if(bits > 0 && max > min){
	//	return PackUInt((uint32_t)(((val - min) / (max - min)) * (float)((1 << bits) - 1)), bits);
		uint32_t j;
		if(val <= min){
			j = 0;//val = min;
		}else{
			j = (uint32_t)(((val - min) / (max - min)) * (double)(1 << bits));
			if(j >= ((uint32_t)1 << bits)) j = (1 << bits) - 1;
		}
		return PackUInt(j, bits);
	}
	return 0;
}
int32_t BitPackEngine::PackFloat(float val, int32_t bits){
	//TODO:  Make good!
	return PackFloatInterval(val, -10000.0f, 10000.0f, bits);
}
int32_t BitPackEngine::PackString(const char *str, int32_t bits){
	if(str && bits <= 8 && bits >= 1){
		int32_t mask = (1 << bits) - 1;
		int32_t val = 1;
		int32_t ret = 1;
		while(val != 0){	//Make sure we check the _truncated_ string value for zero-ness.
			val = ((int32_t)*((const unsigned char*)str)) & mask;
			ret &= PackUInt(val, bits);
			str++;
		}
		return ret;
	}
	return 0;
}

BitUnpackEngine::BitUnpackEngine(const char *buffer, int32_t buflength){
	data = (unsigned char*)buffer;
	maxlen = buflength;
	bitlen = buflength * 8;
	pos = 0;
}
BitUnpackEngine::BitUnpackEngine(const unsigned char *buffer, int32_t buflength){
	data = (unsigned char*)buffer;
	maxlen = buflength;
	bitlen = buflength * 8;
	pos = 0;
}
BitUnpackEngine::~BitUnpackEngine(){
	//
}
int32_t BitUnpackEngine::BytesUsed(){	//Returns number of bytes used in buffer.
	if(data && maxlen > 0) return (pos + 7) / 8;	//0 bits will be 0 bytes, 1 bit will be 1 byte, 8 bits will be 1 bytes, 9 bits 2 bytes, etc.
	return 0;
}
int32_t BitUnpackEngine::BitsUsed(){	//Returns number of bits used in buffer.
	if(data && maxlen > 0) return pos;
	return 0;
}
int32_t BitUnpackEngine::SkipBits(int32_t bits){
	if(pos + bits <= bitlen && pos + bits >= 0){
		pos += bits;
		return 1;
	}
	return 0;
}

int32_t BitUnpackEngine::UnpackInt(int32_t &outval, int32_t bits){
	if(data && bitlen > 0 && bits <= bitlen - pos && bits <= 32){
		int32_t bitpos = pos;
		int32_t bytepos = bitpos >>3;
		int32_t endbitpos = bitpos + bits;
		int32_t endbytepos = (endbitpos - 1) >>3;
		pos += bits;	//Increment internal pos now for next call.
		//
		int32_t val = 0;
		while(endbytepos > bytepos){
			val >>= 8;
			val &= ((1 << (BPW - 8)) - 1);	//Mask.
			val |= (int32_t)data[endbytepos] << (BPW - 8);
			endbytepos--;
		}
		//
		int32_t usedbits = 8 - (bitpos & 7);
		val >>= usedbits;
		val &= (((uint32_t)1 << (BPW - usedbits)) - 1);
		val |= (((int32_t)data[bytepos]) << (BPW - usedbits));
		val >>= (BPW - bits);	//Scoot right.
		outval = val;
		return 1;
	}
	return 0;
}
int32_t BitUnpackEngine::UnpackUInt(uint32_t &val, int32_t bits){
	int32_t v;
	if(UnpackInt(v, bits)){
		if(bits < 32) v &= (((uint32_t)1 << bits) - 1);	//Mask off extraneous bits.
		val = (uint32_t)v;
		return 1;
	}
	return 0;
}
int32_t BitUnpackEngine::UnpackFloatInterval(float &val, float min, float max, int32_t bits){
	if(bits > 0 && max > min){
		uint32_t v;
		if(UnpackUInt(v, bits)){
			double size = (max - min) / (double)(bits >= 32 ? -1 : 1 << bits);
			val = (float)(min + size * ((double)v + 0.5));
			return 1;
		}
	}
	return 0;
}
int32_t BitUnpackEngine::UnpackFloat(float &val, int32_t bits){
	return UnpackFloatInterval(val, -10000.0f, 10000.0f, bits);
}
int32_t BitUnpackEngine::UnpackString(CStr &strout, int32_t bits){
	if(bits <= 8 && bits >= 1){
		char c[MAX_STRING] = {0};
	//	int32_t mask = (1 << bits) - 1;
		int32_t val = 1;
		int32_t ret = 1;
		int32_t n = 0;
		while(val != 0){	//Make sure we check the _truncated_ string value for zero-ness.
			val = 0;
			ret &= UnpackUInt(val, bits);
			if(n < MAX_STRING) c[n] = (unsigned char)val;
			n++;
		//	val = (int32_t)*((const unsigned char*)str++) & mask;
		}
		c[MAX_STRING - 1] = 0;
		strout = c;
		return ret;
	}
	return 0;
}

