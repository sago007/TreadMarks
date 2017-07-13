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

// This class used to wrap the Windows registry calls, but in the name of 
// portability, I've ripped all the Windows-specific stuff out, and now I'm just
// using a simple ini-style file as the backend.
// Seumas' original comment is below. -Rick-
//
//Windows Registry wrapper class, by Seumas McNally, 1998.
//
//Usage:  Construct one object with Company Name and Product Name as parameters.
//Call OpenKey, write or read data, and CloseKey.  Or use Save and RestoreWindowPos.
//Note, they will open and close the key internally!
//Save/RestoreWindowPos will append the letters X, Y, W, and H onto the name given
//when writing the four size/position values.
//
//Actually, I modified it so the data reading/writing functions open and close the
//key internally themselves.  Call me paranoid, and a little inefficient.  But it
//seems to be fast enough, and Windows should cache registry accesses pretty damn
//heavily.

#ifndef REG_H
#define REG_H

#include "CStr.h"

#include <string>
#include <unordered_map>

class Registry
{
	Registry(bool global = false);	//Set global to true to for global settings instead of per-user settings.

public:
	static Registry& Get();

	~Registry();

	void WriteDword(const char *name, uint32_t val) { WriteValue(name, val); }
	void ReadDword(const char *name, uint32_t *val) const { ReadValue(name, *val); }
	void ReadDword(const char *name, int32_t *val) const { ReadValue(name, *val); }
	void ReadDword(const char *name, bool *val) const { ReadValue(name, *val); }
	void WriteFloat(const char *name, float val) { WriteValue(name, val); }
	void ReadFloat(const char *name, float *val) const { ReadValue(name, *val); }
	void WriteString(const char *name, const char *val);
	void ReadString(const char *name, char *val, int maxlen) const;
	void ReadString(const char *name, CStr *str) const;

	template <class T>
	void ReadValue(const char *name, T& val) const
	{
		ValueMap::const_iterator it = aValues.find(name);
		if(it != aValues.end())
			Cast(it->second, val);
	}

	void Cast(const std::string& in, uint32_t& out) const { out = std::stoul(in); }
	void Cast(const std::string& in, int32_t& out) const { out = std::stoi(in); }
	void Cast(const std::string& in, bool& out) const { out = (!in.empty() && in[0] == 't') || in[0] == 'T' || in[0] == '1'; }
	void Cast(const std::string& in, float& out) const { out = std::stof(in); }

	template <class T>
	void WriteValue(const char *name, T& val)
	{
		aValues[name] = std::to_string(val);
		Write();
	}

private:
	std::string sFile;

	typedef std::unordered_map<std::string, std::string> ValueMap;
	ValueMap aValues;

	void Read();
	void Write();
};

#endif
