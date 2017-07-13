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

//File manager implementation.  Seumas McNally, 1998.

#include "FileManager.h"
#include "CfgParse.h"

//#include <cstdlib>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <map>
#include <experimental/filesystem>

FileManager::FileManager(){
	f = NULL;
	SearchDirHead.Data = new CStr;	//Add empty entry to head for searching current dir.
	nFileStack = 0;
}
FileManager::~FileManager(){
//	Close();
	while(PopFile());	//Clear file stack.
	Close();
	LowerCaseToCaseSensitive.clear();
	ClearSearchDirs();
}
bool FileManager::PushFile(){
	if(nFileStack < FILE_STACK_SIZE){
		FileStack[nFileStack] = f;
		f = NULL;
		FileNameStack[nFileStack] = FileName;
		FileName = "";
		FileOffsetStack[nFileStack] = FileOffset;
		FileOffset = 0;
		nFileStack++;
		return true;
	}
	return false;
}
bool FileManager::PopFile(){
	if(nFileStack > 0){
		Close();	//Close active file before re-instating saved file.
		nFileStack--;
		f = FileStack[nFileStack];
		FileName = FileNameStack[nFileStack];
		FileOffset = FileOffsetStack[nFileStack];
		return true;
	}
	return false;
}
void FileManager::ClearSearchDirs(){	//Empties the search dir and packed file list.
	LowerCaseToCaseSensitive.clear();
	SearchDirHead.DeleteList();
	PackedFileHead.DeleteList();
}

static void toLowerCase (std::string& inout) {
	for (size_t i = 0; i< inout.length(); ++i) {
		inout[i] = tolower(inout[i]);
	}
}
int FileManager::AddSearchDir(const char *dir){	//Adds a directory to the search path.
	if(dir && strlen(dir) > 0){
		CStr t;
		char end = dir[strlen(dir) - 1];
		if(end != '/' && end != '\\') t = CStr(dir) + "/";
		else t = dir;
		try {
			for(auto& p: std::experimental::filesystem::recursive_directory_iterator(t.get())) {
				std::string thePath = p.path().c_str();
				std::size_t pos = thePath.find("/");
				thePath = thePath.substr(pos+1);
				std::string lowercase = thePath;
				toLowerCase(lowercase);
				LowerCaseToCaseSensitive[lowercase] = thePath;
				//fprintf(stderr, "added: %s - %s\n", thePath.c_str(), lowercase.c_str());
			}
		} catch (...) {
			//As filesystem is still experimental it is hard to know the exact exception that will be thrown if not existsing.
		}
		fprintf(stderr, "Scanned: %s\n", t.get());
		if(SearchDirHead.AddItem(new CStr(t))) return 1;
	}
	return 0;
}
int FileManager::FindPackedFiles(){	//Scans for packed files in current search directories and makes note of them for further searches.
	//NOT IMPLEMENTED YET!!!
	return 0;
}
FILE *FileManager::Open(const char *in_name){	//The returned pointer is for convenience only!  DO NOT fclose the file stream!
	Close();
	CStrList *cl = &SearchDirHead;
	std::string org_name = in_name? in_name : "";
	std::string name = org_name;
	toLowerCase(name);
	name = LowerCaseToCaseSensitive[name];
	//fprintf(stderr, "Looking for: %s, %s\n",name.c_str(), org_name.c_str());
	if (name.length() == 0 && org_name.length()> 0) {
		//If the folder has not been scanned then try the original name
		name = org_name;
	}
	if(name.length() > 0){
		while((cl != NULL) && (cl->Data != NULL) && (NULL == (f = ::fopen(*cl->Data + name.c_str(), "rb")))){
			//std::string s = std::string(*cl->Data) + name;
			cl = cl->NextLink();
			//fprintf(stderr, "Failed to find: %s\n",s.c_str());
		}
		if(f){
			FileName = *cl->Data + name.c_str();
			FileOffset = 0;
		}
	}
	if(f == NULL && name.length() > 0){
		OutputDebugLog("File Not Found in search path: \"" + CStr(name.c_str()) + "\"\n");
		fprintf(stderr, "File Not Found in search path: %s\n", name.c_str());
	}
	return f;
}
FILE *FileManager::OpenWildcard(const char *wild, char *nameret, int namelen, bool recursive, bool scanbasedir){	//Opens a file by wildcard.
	if(!wild) return NULL;
	//Set up find object.
	find.Clear();
	findIndex = 0;
	CStrList *cl = SearchDirHead.NextLink();
	while(cl && cl->Data){
		find.AddSearch(*cl->Data + wild, recursive, FilePathOnly(*cl->Data + wild));
		cl = cl->NextLink();
	}
	if(scanbasedir) find.AddSearch(wild, recursive, FilePathOnly(wild));	//Then search base dir.
	//Later, do other stuff for packed file searching.
	if(find.Items() <= 0) OutputDebugLog("Wildcard had no matches: " + CStr(wild) + "\n");
	if(find.Items() <= 0) fprintf(stderr, "Wildcard had no matches: %s\n", wild);
	return NextWildcard(nameret, namelen);
}
FILE *FileManager::NextWildcard(char *nameret, uint32_t namelen){	//Continues searching by previous wildcard.
	const char *name = find[findIndex++];
	if(name && nameret && namelen > 0){
		memcpy(nameret, name, std::min(namelen - 1, static_cast<uint32_t>(strlen(name)) + 1));
		nameret[namelen - 1] = '\0';	//Add closing null in case we hit end.
	}
	return Open(name);	//Open will handle a null or boffed pointer.
}
FILE *FileManager::GetFile(){
	return f;
}
CStr FileManager::GetFileName(){
	return FileName;
}
uint32_t FileManager::GetFileOffset(){
	return FileOffset;
}
void FileManager::Close(){
	if(f) fclose(f);
	f = NULL;
	FileName = "";
	FileOffset = 0;
}
//These work on the currently opened file, either physical or in pack.
size_t FileManager::fread(void *buf, size_t size, size_t count){
	if(f && buf) return ::fread(buf, size, count, f);
	return 0;
}
int32_t FileManager::fseek(long offset, int origin){
	if(f) return ::fseek(f, offset, origin);
	return 0;
}
int32_t FileManager::ftell(){
	if(f) return ::ftell(f);
	return 0;
}
int32_t FileManager::length(){
	int32_t pos, len = 0;
	if(f){
		if(false){
			//Do special case for packed file here.
		}else{
			pos = ::ftell(f);
			::fseek(f, 0, SEEK_END);
			len = ::ftell(f);
			::fseek(f, pos, SEEK_SET);
		}
	}
	return len;
}
int32_t FileManager::ReadLong(){
	int32_t l = 0;
	if(f) ::fread(&l, sizeof(l), 1, f);
	return l;
}
char FileManager::ReadByte(){
	char c = 0;
	if(f) ::fread(&c, sizeof(c), 1, f);
	return c;
}
