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

#include "../Web.h"
#include <string>

void OpenWebLink(const char* link)
{
	std::string cmd = "xdg-open \"";
	cmd += link;
	cmd += "\"";
	int ret = system(cmd.c_str());
	if (ret != 0) {
		fprintf(stderr, "Failed to execute: %s (Return code: %d)\n", cmd.c_str(), ret);
	}
}
