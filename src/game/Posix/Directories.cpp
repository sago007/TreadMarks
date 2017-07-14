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

#include "../Directories.h"
#include "../FileManager.h"
#include <experimental/filesystem>

CStr GetAppDataDir()
{
	CStr the_path =  getenv("HOME");
	the_path = the_path + CStr("/.local/share/Tread Marks/");
	/*std::error_code e;
	std::experimental::filesystem::create_directories(the_path.get(), e);
	if (e) {
		fprintf(stderr, "Failed to create \"%s\". Error: %s", the_path.get(), e.message().c_str());
	}*/
	CreateDirectoriesRecursivly(the_path.get());
	return the_path;
}

CStr GetCommonAppDataDir()
{
	return CStr("/usr/local/share/Tread Marks/");
}
