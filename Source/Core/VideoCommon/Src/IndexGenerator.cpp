// Copyright (C) 2003 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/

#include <cstddef>

#include "Common.h"
#include "VideoConfig.h"
#include "IndexGenerator.h"

//Init
u16 *IndexGenerator::Tptr;
u16 *IndexGenerator::BASETptr;
u16 *IndexGenerator::Lptr;
u16 *IndexGenerator::BASELptr;
u16 *IndexGenerator::Pptr;
u16 *IndexGenerator::BASEPptr;
u32 IndexGenerator::numT;
u32 IndexGenerator::numL;
u32 IndexGenerator::numP;
u32 IndexGenerator::index;

void IndexGenerator::Start(u16* Triangleptr, u16* Lineptr, u16* Pointptr)
{
	Tptr = Triangleptr;
	Lptr = Lineptr;
	Pptr = Pointptr;
	BASETptr = Triangleptr;
	BASELptr = Lineptr;
	BASEPptr = Pointptr;
	index = 0;
	numT = 0;
	numL = 0;
	numP = 0;
}

void IndexGenerator::AddIndices(int primitive, u32 numVerts)
{
	//switch (primitive)
	//{
	//case GX_DRAW_QUADS:          IndexGenerator::AddQuads(numVerts);		break;
	//case GX_DRAW_TRIANGLES:      IndexGenerator::AddList(numVerts);		break;
	//case GX_DRAW_TRIANGLE_STRIP: IndexGenerator::AddStrip(numVerts);		break;
	//case GX_DRAW_TRIANGLE_FAN:   IndexGenerator::AddFan(numVerts);		break;
	//case GX_DRAW_LINES:          IndexGenerator::AddLineList(numVerts);	break;
	//case GX_DRAW_LINE_STRIP:     IndexGenerator::AddLineStrip(numVerts);	break;
	//case GX_DRAW_POINTS:         IndexGenerator::AddPoints(numVerts);		break;
	//}

	static void (*const primitive_table[])(u32) =
	{
		IndexGenerator::AddQuads,
		NULL,
		IndexGenerator::AddList,
		IndexGenerator::AddStrip,
		IndexGenerator::AddFan,
		IndexGenerator::AddLineList,
		IndexGenerator::AddLineStrip,
		IndexGenerator::AddPoints,
	};

	primitive_table[primitive](numVerts);
	index += numVerts;
}

// Triangles
__forceinline void IndexGenerator::WriteTriangle(u32 index1, u32 index2, u32 index3)
{
	*Tptr++ = index1;
	*Tptr++ = index2;
	*Tptr++ = index3;
	if(g_Config.backend_info.bSupportsPrimitiveRestart)
		*Tptr++ = 65535;
	
	++numT;
}

void IndexGenerator::AddList(u32 const numVerts)
{
	auto const numTris = numVerts / 3;
	for (u32 i = 0; i != numTris; ++i)
	{
		WriteTriangle(index + i * 3, index + i * 3 + 1, index + i * 3 + 2);
	}
}

void IndexGenerator::AddStrip(u32 const numVerts)
{
	if(g_Config.backend_info.bSupportsPrimitiveRestart) {
		for (u32 i = 0; i < numVerts; ++i)
		{
			*Tptr++ = index + i;
		}
		*Tptr++ = 65535;
		numT += numVerts - 2;
		
	} else {
		bool wind = false;
		for (u32 i = 2; i < numVerts; ++i)
		{
			WriteTriangle(
				index + i - 2,
				index + i - !wind,
				index + i - wind);

			wind ^= true;
		}
	}
}

/**
 * FAN simulator:
 * 
 *   2---3
 *  / \ / \
 * 1---0---4
 * 
 * would generate this triangles:
 * 012, 023, 034
 * 
 * rotated (for better striping):
 * 120, 302, 034
 * 
 * as odd ones have to winded, following strip is fine:
 * 12034
 * 
 * so we use 6 indices for 3 triangles
 */

void IndexGenerator::AddFan(u32 numVerts)
{
	ERROR_LOG(VIDEO, "addfan: %d vertices", numVerts);
	u32 i = 2;
	
	if(g_Config.backend_info.bSupportsPrimitiveRestart) {
		for(; i<=numVerts-3; i+=3) {
			*Tptr++ = index + i - 1;
			*Tptr++ = index + i + 0;
			*Tptr++ = index;
			*Tptr++ = index + i + 1;
			*Tptr++ = index + i + 2;
			*Tptr++ = 65535;
			numT += 3;
		}
		
		for(; i<=numVerts-2; i+=2) {
			*Tptr++ = index + i - 1;
			*Tptr++ = index + i + 0;
			*Tptr++ = index;
			*Tptr++ = index + i + 1;
			*Tptr++ = 65535;
			numT += 2;
		}
	}
	
	for (; i < numVerts; ++i)
	{
		WriteTriangle(index, index + i - 1, index + i);
	}
}

/*
 * QUAD simulator
 * 
 * 0---1   4---5
 * |\  |   |\  |
 * | \ |   | \ |
 * |  \|   |  \|
 * 3---2   7---6
 * 
 * 012,023, 456,467 ...
 * or 120,302, 564,746
 * or as strip: 1203, 5647
 */
void IndexGenerator::AddQuads(u32 numVerts)
{
	auto const numQuads = numVerts / 4;
	for (u32 i = 0; i != numQuads; ++i)
	{
		if(g_Config.backend_info.bSupportsPrimitiveRestart) {
			*Tptr++ = index + i * 4 + 1;
			*Tptr++ = index + i * 4 + 2;
			*Tptr++ = index + i * 4 + 0;
			*Tptr++ = index + i * 4 + 3;
			*Tptr++ = 65535;
			numT += 2;
		} else {
			WriteTriangle(index + i * 4, index + i * 4 + 1, index + i * 4 + 2);
			WriteTriangle(index + i * 4, index + i * 4 + 2, index + i * 4 + 3);
		}
	}
}

// Lines
void IndexGenerator::AddLineList(u32 numVerts)
{
	auto const numLines = numVerts / 2;
	for (u32 i = 0; i != numLines; ++i)
	{
		*Lptr++ = index + i * 2;
		*Lptr++ = index + i * 2 + 1;
		++numL;
	}
}

void IndexGenerator::AddLineStrip(u32 numVerts)
{
	for (u32 i = 1; i < numVerts; ++i)
	{
		*Lptr++ = index + i - 1;
		*Lptr++ = index + i;
		++numL;
	}
}

// Points
void IndexGenerator::AddPoints(u32 numVerts)
{
	for (u32 i = 0; i != numVerts; ++i)
	{
		*Pptr++ = index + i;
		++numP;
	}
}


u32 IndexGenerator::GetRemainingIndices()
{
	u32 max_index = 65535;
	return max_index - index;
}
