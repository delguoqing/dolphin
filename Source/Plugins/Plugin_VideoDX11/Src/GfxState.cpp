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

#include "VideoConfig.h"
#include "GfxState.h"
#include "VertexShaderCache.h"

namespace D3D
{

StateManager* stateman;


template<typename T> AutoState<T>::AutoState(const T* object) : state(object)
{
	((IUnknown*)state)->AddRef();
}

template<typename T> AutoState<T>::AutoState(const AutoState<T> &source)
{
	state = source.GetPtr();
	((T*)state)->AddRef();
}

template<typename T> AutoState<T>::~AutoState()
{
	if(state) ((T*)state)->Release();
	state = NULL;
}

StateManager::StateManager() : cur_blendstate(NULL), cur_depthstate(NULL), cur_raststate(NULL) {}

void StateManager::PushBlendState(const ID3D11BlendState* state) { blendstates.push(AutoBlendState(state)); }
void StateManager::PushDepthState(const ID3D11DepthStencilState* state) { depthstates.push(AutoDepthStencilState(state)); }
void StateManager::PushRasterizerState(const ID3D11RasterizerState* state) { raststates.push(AutoRasterizerState(state)); }
void StateManager::PopBlendState() { blendstates.pop(); }
void StateManager::PopDepthState() { depthstates.pop(); }
void StateManager::PopRasterizerState() { raststates.pop(); }

void StateManager::Apply()
{
	if (!blendstates.empty())
	{
		if (cur_blendstate != blendstates.top().GetPtr())
		{
			cur_blendstate = (ID3D11BlendState*)blendstates.top().GetPtr();
			D3D::context->OMSetBlendState(cur_blendstate, NULL, 0xFFFFFFFF);
		}
	}
	else ERROR_LOG(VIDEO, "Tried to apply without blend state!");

	if (!depthstates.empty())
	{
		if (cur_depthstate != depthstates.top().GetPtr())
		{
			cur_depthstate = (ID3D11DepthStencilState*)depthstates.top().GetPtr();
			D3D::context->OMSetDepthStencilState(cur_depthstate, 0);
		}
	}
	else ERROR_LOG(VIDEO, "Tried to apply without depth state!");

	if (!raststates.empty())
	{
		if (cur_raststate != raststates.top().GetPtr())
		{
			cur_raststate = (ID3D11RasterizerState*)raststates.top().GetPtr();
			D3D::context->RSSetState(cur_raststate);
		}
	}
	else ERROR_LOG(VIDEO, "Tried to apply without rasterizer state!");
}

}  // namespace
