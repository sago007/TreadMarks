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


#ifndef GLPOLYRENDER_H
#define GLPOLYRENDER_H

#include "../Poly.h"

class GLPolyRender : public PolyRender {
public:
	bool GLDoRender() override;	//Renders using OpenGL.
	bool GLDoRenderTrans() override;	//Renders transparents, MUST BE CALLED AFTER ABOVE CALL!
	bool GLDoRenderOrtho() override;
	bool GLDoRenderSecondary() override;	//Renders using OpenGL.

	void GLResetStates() override;
	void GLBlendMode(int32_t mode) override;	//Will not re-set already set modes.
	void GLBindTexture(uint32_t name) override;	//Will not re-bind already bound textures.

	//Russ
	void GLRenderMeshObject(MeshObject *thismesh, PolyRender *PR) override;
	void GLRenderSpriteObject(SpriteObject *thissprite, PolyRender *PR) override;
	void GLRenderParticleCloudObject(ParticleCloudObject *thispc, PolyRender *PR) override;
	void GLRenderStringObject(StringObject *thisstring, PolyRender *PR) override;
	void GLRenderLineMapObject(LineMapObject *thislinemap, PolyRender *PR) override;
	void GLRenderTilingTextureObject(TilingTextureObject *thistile, PolyRender *PR) override;
	void GLRenderChamfered2DBoxObject(Chamfered2DBoxObject *thisbox, PolyRender *PR) override;
};

#endif

