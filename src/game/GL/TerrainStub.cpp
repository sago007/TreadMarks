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

// Stub for headless build

#include "../Terrain.h"
#include "../ResourceManager.h"

void Terrain::UndownloadTextures(){
}
int32_t Terrain::MakeTexturePalette(EcoSystem *eco, int32_t numeco){	//Makes a map-specific texture palette, for use with paletted terrain textures.
	return 1;
}
void Terrain::UsePalettedTextures(bool usepal){
}

bool Terrain::DownloadTextures(){
	return false;
}
int32_t Terrain::UpdateTextures(int32_t x1, int32_t y1, int32_t w, int32_t h){
	return 1;
}

int32_t Terrain::Redownload(int32_t px, int32_t py){
	return 0;
}



bool Terrain::MapLod(){
	return true;
}
bool Terrain::MapLod(int32_t x, int32_t y, int32_t w, int32_t h){
	return true;
}

bool ResourceManager::DownloadTextures(bool UpdateOnly){
	return false;
}