/*
Copyright (C) 2016 pelorat

This file is part of Vectrexia.

Vectrexia is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Vectrexia is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Vectrexia.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef WIN32_H
#define WIN32_H

#if _MSC_VER >= 1910 && !__INTEL_COMPILER
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0600
#include <SDKDDKVer.h>
#include <windows.h>
#endif

#endif // WIN32_H
