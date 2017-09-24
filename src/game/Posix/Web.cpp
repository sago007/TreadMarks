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
#include "CStr.h"

void OpenWebLink(const char* link)
{
	CStr cmd = "xdg-open \"";
	cmd.cat(link);
	cmd.cat("\"");
	int ret = system(cmd.get());
	if (ret != 0) {
		fprintf(stderr, "Failed to execute: %s (Return code: %d)\n", cmd.get(), ret);
	}
}
