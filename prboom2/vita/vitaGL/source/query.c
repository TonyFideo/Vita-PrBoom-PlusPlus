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
 * query.c:
 * Partial implementation of GL1.5 query objects
 */

#include <stdint.h>
#include <string.h>

#include "shared.h"

#define VIS_NUM_QUERIES 16
#define VIS_CORE_STRIDE (VIS_NUM_QUERIES * sizeof(uint32_t))

static uint32_t *vis_buffer;
static uint32_t vis_buffer_size;

static struct VisQuery {
	GLboolean active;
	SceGxmNotification notify;
	GLboolean wait;
} vis_queries[VIS_NUM_QUERIES];

static struct GLQuery {
	GLboolean active;
	GLboolean allocated;
	GLenum target;
	uint64_t data;
	uint64_t result;
} gl_queries[QUERIES_NUM];

enum {
	Q_SAMPLES_PASSED,
	Q_ANY_SAMPLES_PASSED,
	Q_TIME_ELAPSED,
	Q_MAX_TARGETS
};

static uint32_t query_targets[Q_MAX_TARGETS];

static inline int qtarget_index(GLenum target) {
	switch (target) {
		case GL_SAMPLES_PASSED: return Q_SAMPLES_PASSED;
		case GL_ANY_SAMPLES_PASSED: return Q_ANY_SAMPLES_PASSED;
		case GL_TIME_ELAPSED: return Q_TIME_ELAPSED;
		default: return -1;
	}
}

GLboolean initQueries(void) {
	// 4 GPU cores, 1 buffer, there has to be at least 16 bytes of stride between
	// regions for each GPU core, even though counters themselves are 4 bytes each
	const size_t bufsize = 4 * 1 * VIS_CORE_STRIDE;
	vglMemType bufmem = VGL_MEM_RAM;
	vis_buffer = gpu_alloc_mapped(bufsize, &bufmem);
	if (!vis_buffer) return GL_FALSE;

	// alloc invalid query (0)
	gl_queries[0].allocated = GL_TRUE;

	// set visibility query buffer
	sceGxmSetVisibilityBuffer(gxm_context, vis_buffer, VIS_CORE_STRIDE);

	// allocate notifications
	for (uint32_t i = 0; i < VIS_NUM_QUERIES; ++i) {
		vis_queries[i].notify.value = 0;
		vis_queries[i].notify.address = sceGxmGetNotificationRegion() + i;
		*vis_queries[i].notify.address = 0;
	}

	memset(vis_buffer, 0, bufsize);
	vis_buffer_size = bufsize;

	return GL_TRUE;
}

void glGenQueries(GLsizei n, GLuint *ids) {
	for (GLsizei count = 0; count < n; ++count) {
		GLuint i;
		// try to find an unused query slot
		for (i = 1; i < QUERIES_NUM; ++i)
			if (!gl_queries[i].allocated) break;
		// everything is taken up, die
		if (i >= QUERIES_NUM) {
			SET_GL_ERROR(GL_OUT_OF_MEMORY)
		}
		// clear the query object we found and return it into the index array
		memset(&gl_queries[i], 0, sizeof(*gl_queries));
		gl_queries[i].allocated = GL_TRUE;
		ids[count] = i;
	}
}

void glDeleteQueries(GLsizei n, const GLuint *ids) {
	for (GLsizei i = 0; i < n; ++i) {
		const GLuint id = ids[i];
		if (id == 0 || id >= QUERIES_NUM || !gl_queries[id].allocated)
			continue;
		if (gl_queries[id].active) {
			glEndQuery(gl_queries[id].target);
			if (gl_queries[id].target == GL_SAMPLES_PASSED || gl_queries[id].target == GL_ANY_SAMPLES_PASSED) {
				vis_queries[gl_queries[id].target].active = GL_FALSE;
			}
		}
		gl_queries[id].allocated = GL_FALSE;
	}
}

static inline GLboolean begin_vis_query(struct GLQuery *query) {
	// find available vis query
	uint32_t i;
	for (i = 0; i < VIS_NUM_QUERIES; ++i) {
		if (vis_queries[i].active == GL_FALSE)
			break;
	}

	if (i >= VIS_NUM_QUERIES) return GL_FALSE;

	sceGxmSetFrontVisibilityTestEnable(gxm_context, SCE_GXM_VISIBILITY_TEST_ENABLED);
	sceGxmSetFrontVisibilityTestIndex(gxm_context, i);
	// FIXME: GL_SAMPLES_PASSED requires TEST_OP_INCREMENT, but it only works with shaders
	//		that don't have discard in them and don't override depth
	sceGxmSetFrontVisibilityTestOp(gxm_context, SCE_GXM_VISIBILITY_TEST_OP_SET);

	sceGxmSetBackVisibilityTestEnable(gxm_context, SCE_GXM_VISIBILITY_TEST_ENABLED);
	sceGxmSetBackVisibilityTestIndex(gxm_context, i);
	// FIXME: GL_SAMPLES_PASSED requires TEST_OP_INCREMENT, but it only works with shaders
	//		that don't have discard in them and don't override depth
	sceGxmSetBackVisibilityTestOp(gxm_context, SCE_GXM_VISIBILITY_TEST_OP_SET);

	vis_queries[i].wait = GL_FALSE;

	query->data = i;

	return GL_TRUE;
}

void glBeginQuery(GLenum target, GLuint id) {
	const int tidx = qtarget_index(target);

	if (tidx < 0) {
		SET_GL_ERROR(GL_INVALID_ENUM)
	}

	if (query_targets[tidx] != 0) {
		// already querying this target
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}

	if (id == 0 || id >= QUERIES_NUM || !gl_queries[id].allocated || gl_queries[id].active) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}

	struct GLQuery *query = gl_queries + id;

	switch (target) {
		case GL_SAMPLES_PASSED:
		case GL_ANY_SAMPLES_PASSED:
			if (!begin_vis_query(query)) {
				SET_GL_ERROR(GL_INVALID_OPERATION)
			}
			break;
		case GL_TIME_ELAPSED:
			query->data = sceKernelGetProcessTimeWide();
			break;
		default:
			SET_GL_ERROR(GL_INVALID_ENUM)
	}

	query->target = target;
	query->active = GL_TRUE;
	query->result = 0;
	query_targets[qtarget_index(target)] = id;
}

static inline GLboolean end_vis_query(struct GLQuery *query) {
	sceGxmSetFrontVisibilityTestEnable(gxm_context, SCE_GXM_VISIBILITY_TEST_DISABLED);
	sceGxmSetBackVisibilityTestEnable(gxm_context, SCE_GXM_VISIBILITY_TEST_DISABLED);

	// if we were in a scene, end it, notifying the appropriate visquery
	const GLboolean was_in_scene = vgl_in_scene;
	if (was_in_scene) {
		endGxmScene(&vis_queries[query->data].notify);
		// flag this visquery as pending collection
		vis_queries[query->data].wait = GL_TRUE;
	}

	// if we were in a scene before, start new scene
	if (was_in_scene) beginGxmScene();

	return GL_TRUE;
}


void glEndQuery(GLenum target) {
	const int tidx = qtarget_index(target);

	if (tidx < 0) {
		SET_GL_ERROR(GL_INVALID_ENUM)
	}

	if (query_targets[tidx] == 0) {
		// no query pending on this target
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}

	struct GLQuery *query = gl_queries + query_targets[tidx];
	if (!query->active) {
		// how did this happen
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}

	switch (target) {
		case GL_SAMPLES_PASSED:
		case GL_ANY_SAMPLES_PASSED:
			if (!end_vis_query(query)) {
				SET_GL_ERROR(GL_INVALID_OPERATION)
			}
			break;

		case GL_TIME_ELAPSED:
			// TODO: do the same end scene / sync thing as before, can't be arsed
			query->result = sceKernelGetProcessTimeWide() - query->data;
			break;

		default:
			SET_GL_ERROR(GL_INVALID_ENUM)
	}

	query->active = GL_FALSE;
	query_targets[tidx] = 0;
}

static inline void get_vis_query_result(struct GLQuery *query) {
	struct VisQuery *visq = vis_queries + query->data;

	// if needed, wait for the notification
	if (visq->wait) {
		sceGxmNotificationWait(&visq->notify);
		visq->wait = GL_FALSE;
	}

	// sum visibility data over GPU cores and reset query values
	query->result = 0;
	for (uint32_t i = 0; i < 4; ++i) {
		query->result += vis_buffer[query->data + i * VIS_NUM_QUERIES];
		vis_buffer[query->data + i * VIS_NUM_QUERIES] = 0;
	}

	visq->active = GL_FALSE;
}

static inline GLboolean get_vis_query_ready(struct GLQuery *query) {
	struct VisQuery *visq = vis_queries + query->data;
	if (!visq->wait) // either already waited or there's no need to wait
		return GL_TRUE;
	else // notification is triggered if value == *address
		return (visq->notify.value == *visq->notify.address);
}

void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint * params) {
	if (id == 0 || id >= QUERIES_NUM || !gl_queries[id].allocated) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}

	struct GLQuery *query = gl_queries + id;

	if (pname == GL_QUERY_RESULT) {
		switch (query->target) {
			case GL_SAMPLES_PASSED:
			case GL_ANY_SAMPLES_PASSED:
				get_vis_query_result(query);
				*params = query->result;
				break;
			case GL_TIME_ELAPSED:
				*params = query->result;
				break;
			default:
				SET_GL_ERROR(GL_INVALID_ENUM)
		}
	} else if (pname == GL_QUERY_RESULT_AVAILABLE) {
		switch (query->target) {
			case GL_SAMPLES_PASSED:
			case GL_ANY_SAMPLES_PASSED:
				*params = get_vis_query_ready(query);
				break;
			case GL_TIME_ELAPSED:
				*params = GL_TRUE; // always available
				break;
			default:
				SET_GL_ERROR(GL_INVALID_ENUM)
		}
	} else {
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glGetQueryObjectiv(GLuint id, GLenum pname, GLint * params) {
	if (id == 0 || id >= QUERIES_NUM || !gl_queries[id].allocated) {
		SET_GL_ERROR(GL_INVALID_OPERATION)
	}

	struct GLQuery *query = gl_queries + id;

	if (pname == GL_QUERY_RESULT) {
		switch (query->target) {
			case GL_SAMPLES_PASSED:
			case GL_ANY_SAMPLES_PASSED:
				get_vis_query_result(query);
				*params = query->result;
				break;
			case GL_TIME_ELAPSED:
				*params = query->result;
				break;
			default:
				SET_GL_ERROR(GL_INVALID_ENUM)
		}
	} else if (pname == GL_QUERY_RESULT_AVAILABLE) {
		switch (query->target) {
			case GL_SAMPLES_PASSED:
			case GL_ANY_SAMPLES_PASSED:
				*params = get_vis_query_ready(query);
				break;
			case GL_TIME_ELAPSED:
				*params = GL_TRUE; // always available
				break;
			default:
				SET_GL_ERROR(GL_INVALID_ENUM)
		}
	} else {
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

void glGetQueryiv(GLenum target, GLenum pname, GLint * params) {
	if (pname == GL_QUERY_COUNTER_BITS) {
		if (target == GL_TIMESTAMP || target == GL_TIME_ELAPSED) {
			*params = 64; // timers are uint64
		} else if (pname == GL_ANY_SAMPLES_PASSED || pname == GL_SAMPLES_PASSED) {
			*params = 32; // everything else is uint32
		} else {
			SET_GL_ERROR(GL_INVALID_ENUM)
		}
	} else if (pname == GL_CURRENT_QUERY) {
		const int index = qtarget_index(target);
		if (index < 0) {
			SET_GL_ERROR(GL_INVALID_ENUM)
		}
		*params = query_targets[index];
	} else {
		SET_GL_ERROR(GL_INVALID_ENUM)
	}
}

GLboolean glIsQuery(GLuint id) {
	return (id != 0 && id < QUERIES_NUM && gl_queries[id].allocated);
}
