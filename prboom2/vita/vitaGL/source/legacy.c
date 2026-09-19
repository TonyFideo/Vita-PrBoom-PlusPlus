/*
 * This file is part of vitaGL
 * Copyright 2017, 2018, 2019, 2020 Rinnegatamante
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * legacy.c:
 * Implementation for legacy openGL 1.0 rendering method
 */

#include "shared.h"

#define DEFAULT_VTX_COUNT 16384

static GLuint vtx_max_vertices = 0;
static unsigned short *vtx_idx, *vtx_idx_quads;
static GLfloat *vtx_pos, *vtx_posptr, *vtx_posstart;
static GLfloat *vtx_tex, *vtx_texptr, *vtx_texstart;
static GLfloat *vtx_col, *vtx_colptr, *vtx_colstart;
static int vtx_num = 0;
static GLenum vtx_curprim = 0;

vector4f current_color = { 1.0f, 1.0f, 1.0f, 1.0f }; // Current in use color

void vglIndexPointerImmediate(void) {
	if (vtx_pos == NULL) {
		vglSetImmediateBufferSize(DEFAULT_VTX_COUNT);
	}
	vglIndexPointerMapped(vtx_idx);
}

void vglResetImmediateBuffer(void) {
	if (vtx_curprim != 0) {
		// don't reset in the middle of a model
		return;
	}
	vtx_posptr = vtx_posstart = vtx_pos;
	vtx_texptr = vtx_texstart = vtx_tex;
	vtx_colptr = vtx_colstart = vtx_col;
}

GLboolean vglSetImmediateBufferSize(const GLuint numverts) {
	if (numverts > 0xFFFF || numverts == 0) {
		vgl_error = GL_INVALID_VALUE;
		return GL_FALSE;
	}

	if (vtx_curprim != 0) {
		// don't realloc in the middle of a model
		vgl_error = GL_INVALID_OPERATION;
		return GL_FALSE;
	}

	vtx_pos = realloc(vtx_pos, sizeof(GLfloat) * 3 * numverts);
	vtx_tex = realloc(vtx_tex, sizeof(GLfloat) * 2 * numverts);
	vtx_col = realloc(vtx_col, sizeof(GLfloat) * 4 * numverts);
	vtx_idx = realloc(vtx_idx, sizeof(GLshort) * 1 * numverts);
	vtx_idx_quads = realloc(vtx_idx_quads, sizeof(GLshort) * 1 * numverts);

	if (!vtx_pos || !vtx_tex || !vtx_col || !vtx_idx || !vtx_idx_quads) {
		vgl_error = GL_OUT_OF_MEMORY;
		return GL_FALSE;
	}

	for (GLuint i = 0; i < numverts; ++i)
		vtx_idx[i] = i; // indices are always sequential here

	for (GLuint i = 0; i < (numverts / 6); ++i) {
		// prepare indices for GL_QUADS
		vtx_idx_quads[i * 6 + 0] = i * 4 + 0;
		vtx_idx_quads[i * 6 + 1] = i * 4 + 1;
		vtx_idx_quads[i * 6 + 2] = i * 4 + 3;
		vtx_idx_quads[i * 6 + 3] = i * 4 + 1;
		vtx_idx_quads[i * 6 + 4] = i * 4 + 2;
		vtx_idx_quads[i * 6 + 5] = i * 4 + 3;
	}

	vtx_max_vertices = numverts;

	vglResetImmediateBuffer();

	return GL_TRUE;
}

void glBegin(GLenum prim) {
	if (vtx_pos == NULL) {
		vglSetImmediateBufferSize(DEFAULT_VTX_COUNT);
	}

	// already in glBegin ... glEnd
	if (vtx_curprim != 0) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}

	phase = MODEL_CREATION;
	vtx_num = 0;
	vtx_curprim = prim;
}

void glVertex2f(GLfloat x, GLfloat y) {
	glVertex3f(x, y, 0.f);
}

void glVertex2i(GLint x, GLint y) {
	glVertex3f(x, y, 0.f);
}

void glVertex3fv(const GLfloat *v) {
	glVertex3f(v[0], v[1], v[2]);
}

void glVertex3i(GLint x, GLint y, GLint z) {
	glVertex3f(x, y, z);
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
#ifndef SKIP_ERROR_HANDLING
	if (vtx_curprim == 0) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	*(vtx_colptr++) = current_color.r;
	*(vtx_colptr++) = current_color.g;
	*(vtx_colptr++) = current_color.b;
	*(vtx_colptr++) = current_color.a;
	*(vtx_posptr++) = x;
	*(vtx_posptr++) = y;
	*(vtx_posptr++) = z;
	vtx_num++;
}

void glTexCoord2i(GLint u, GLint v) {
	glTexCoord2f(u, v);
}

void glTexCoord2iv(const GLint *v) {
	glTexCoord2f(v[0], v[1]);
}

void glTexCoord2fv(const GLfloat *v) {
	glTexCoord2f(v[0], v[1]);
}

void glTexCoord2f(GLfloat u, GLfloat v) {
#ifndef SKIP_ERROR_HANDLING
	// this is technically incorrect, but we don't have a "current texture coordinate"
	// and instead just fill the buffer immediately
	if (vtx_curprim == 0) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}
#endif
	*(vtx_texptr++) = u;
	*(vtx_texptr++) = v;
}

void glColor3ub(GLubyte r, GLubyte g, GLubyte b) {
	glColor4f(r / 255.f, g / 255.f, b / 255.f, 1.f);
}

void glColor3ubv(const GLubyte *v) {
	glColor4f(v[0] / 255.f, v[1] / 255.f, v[2] / 255.f, 1.f);
}

void glColor3f(GLfloat r, GLfloat g, GLfloat b) {
	glColor4f(r, g, b, 1.f);
}

void glColor3fv(const GLfloat *v) {
	glColor4f(v[0], v[1], v[2], 1.f);
}

void glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) {
	glColor4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
}

void glColor4ubv(const GLubyte *v) {
	glColor4f(v[0] / 255.f, v[1] / 255.f, v[2] / 255.f, v[3] / 255.f);
}

void glColor4fv(const GLfloat *v) {
	glColor4f(v[0], v[1], v[2], v[3]);
}

void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
	current_color.r = r;
	current_color.g = g;
	current_color.b = b;
	current_color.a = a;
}

void glEnd(void) {
	if (vtx_curprim == 0 || vtx_num == 0) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}

	// save current array states
	texture_unit *tex_unit = &texture_units[client_texture_unit];
	const GLboolean varr_state = tex_unit->vertex_array_state;
	const GLboolean carr_state = tex_unit->color_array_state;
	const GLenum carr_type = tex_unit->color_object_type;
	const GLboolean tarr_state = tex_unit->texture_array_state;

	tex_unit->vertex_array_state = GL_TRUE;
	tex_unit->color_array_state = GL_TRUE;

	if (vtx_curprim == GL_QUADS) {
		// substitute indices for triangulated quads
		vtx_curprim = GL_TRIANGLES;
		vtx_num = (vtx_num / 4) * 6;
		vglIndexPointerMapped(vtx_idx_quads);
	} else {
		vglIndexPointerMapped(vtx_idx);
	}

	vglVertexPointerMapped(vtx_posstart);
	vglColorPointerMapped(GL_FLOAT, vtx_colstart);
	if (vtx_texptr != vtx_texstart) {
		tex_unit->texture_array_state = GL_TRUE;
		vglTexCoordPointerMapped(vtx_texstart);
	} else {
		tex_unit->texture_array_state = GL_FALSE;
	}

	vglDrawObjects(vtx_curprim, vtx_num, GL_TRUE);

	// restore array state
	tex_unit->vertex_array_state = varr_state;
	tex_unit->color_array_state = carr_state;
	tex_unit->color_object_type = carr_type;
	tex_unit->texture_array_state = tarr_state;

	// TODO: check if we need to restore these properly
	tex_unit->vertex_object = NULL;
	tex_unit->color_object = NULL;
	tex_unit->texture_object = NULL;

	vtx_curprim = 0;
	vtx_num = 0;
	vtx_posstart = vtx_posptr;
	vtx_colstart = vtx_colptr;
	vtx_texstart = vtx_texptr;
	phase = NONE;
}
