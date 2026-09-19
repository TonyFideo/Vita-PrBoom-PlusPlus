/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *
 *  memory manager for GL data
 *---------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doomtype.h"
#include "gl_intern.h"
#include "lprintf.h"

#ifdef PRBOOM_VITA_DIAGNOSTICS
#include "vita_log.h"
#endif

GLDrawInfo gld_drawinfo;

#ifdef PRBOOM_VITA_DIAGNOSTICS
static unsigned int vita_diag_drawinfo_alloc_failures;

static void vita_diag_drawinfo_alloc_failure(const char *stage, size_t size)
{
  if (!VitaLog_IsEnabled(VITA_LOG_RENDER))
    return;

  vita_diag_drawinfo_alloc_failures++;
  lprintf(LO_DEBUG | LO_VITA_RENDER,
    "[PRBOOM-VITA-DIAG] drawinfo allocation failed: stage=%s bytes=%lu failures=%u\n",
    stage, (unsigned long)size, vita_diag_drawinfo_alloc_failures);
}
#endif

//
// gld_FreeDrawInfo
//
void gld_FreeDrawInfo(void)
{
  int i;

  for (i = 0; i < gld_drawinfo.maxsize; i++)
  {
    if (gld_drawinfo.data[i].data)
    {
      free(gld_drawinfo.data[i].data);
      gld_drawinfo.data[i].data = 0;
    }
  }
  free(gld_drawinfo.data);
  gld_drawinfo.data = 0;

  for (i = 0; i < GLDIT_TYPES; i++)
  {
    if (gld_drawinfo.items[i])
    {
      free(gld_drawinfo.items[i]);
      gld_drawinfo.items[i] = 0;
    }
  }

  memset(&gld_drawinfo, 0, sizeof(GLDrawInfo));
}

//
// gld_ResetDrawInfo
//
// Should be used between frames (in gld_StartDrawScene)
//
void gld_ResetDrawInfo(void)
{
  int i;

  for (i = 0; i < gld_drawinfo.maxsize; i++)
  {
    gld_drawinfo.data[i].size = 0;
  }
  gld_drawinfo.size = 0;

  for (i = 0; i < GLDIT_TYPES; i++)
  {
    gld_drawinfo.num_items[i] = 0;
  }
}

//
// gld_AddDrawRange
//
static dboolean gld_AddDrawRange(int size)
{
  int newmaxsize = gld_drawinfo.maxsize + 1;
  GLDrawDataItem_t *data = realloc(gld_drawinfo.data,
    newmaxsize * sizeof(gld_drawinfo.data[0]));

  if (!data)
  {
#ifdef PRBOOM_VITA_DIAGNOSTICS
    vita_diag_drawinfo_alloc_failure("draw-range table", newmaxsize * sizeof(gld_drawinfo.data[0]));
#endif
    return false;
  }

  gld_drawinfo.data = data;
  gld_drawinfo.data[gld_drawinfo.maxsize].data = malloc(size);
  if (!gld_drawinfo.data[gld_drawinfo.maxsize].data)
  {
    gld_drawinfo.data[gld_drawinfo.maxsize].maxsize = 0;
    gld_drawinfo.data[gld_drawinfo.maxsize].size = 0;
#ifdef PRBOOM_VITA_DIAGNOSTICS
    vita_diag_drawinfo_alloc_failure("draw-range data", size);
#endif
    return false;
  }

  gld_drawinfo.data[gld_drawinfo.maxsize].maxsize = size;
  gld_drawinfo.data[gld_drawinfo.maxsize].size = 0;
  gld_drawinfo.maxsize = newmaxsize;
  return true;
}

//
// gld_AddDrawItem
//
#define NEWSIZE (MAX(64 * 1024, itemsize))
#define SIZEOF8(type) ((sizeof(type)+7)&~7)
void gld_AddDrawItem(GLDrawItemType itemtype, void *itemdata)
{
  int itemsize = 0;
  byte *item_p = NULL;

  static int itemsizes[GLDIT_TYPES] = {
    0,
    SIZEOF8(GLWall), SIZEOF8(GLWall), SIZEOF8(GLWall), SIZEOF8(GLWall), SIZEOF8(GLWall),
    SIZEOF8(GLWall), SIZEOF8(GLWall),
    SIZEOF8(GLFlat), SIZEOF8(GLFlat),
    SIZEOF8(GLFlat), SIZEOF8(GLFlat),
    SIZEOF8(GLSprite), SIZEOF8(GLSprite), SIZEOF8(GLSprite),
    SIZEOF8(GLShadow),
    SIZEOF8(GLHealthBar)
  };

  if (itemtype <= GLDIT_NONE || itemtype >= GLDIT_TYPES)
  {
    I_Error("gld_AddDrawItem: unknown GLDrawItemType %d", itemtype);
  }
  itemsize = itemsizes[itemtype];
  if (itemsize == 0)
  {
    I_Error("gld_AddDrawItem: unknown GLDrawItemType %d", itemtype);
  }

  if (gld_drawinfo.maxsize == 0)
  {
    if (!gld_AddDrawRange(NEWSIZE))
      return;
  }

  if (!gld_drawinfo.data[gld_drawinfo.size].data ||
      gld_drawinfo.data[gld_drawinfo.size].size + itemsize >=
    gld_drawinfo.data[gld_drawinfo.size].maxsize)
  {
    gld_drawinfo.size++;
    if (gld_drawinfo.size >= gld_drawinfo.maxsize)
    {
      if (!gld_AddDrawRange(NEWSIZE))
      {
        gld_drawinfo.size--;
        return;
      }
    }
  }

  if (!gld_drawinfo.data[gld_drawinfo.size].data)
    return;

  item_p = gld_drawinfo.data[gld_drawinfo.size].data +
    gld_drawinfo.data[gld_drawinfo.size].size;

  memcpy(item_p, itemdata, itemsize);

  gld_drawinfo.data[gld_drawinfo.size].size += itemsize;

  if (gld_drawinfo.num_items[itemtype] >= gld_drawinfo.max_items[itemtype])
  {
    int newmaxitems = gld_drawinfo.max_items[itemtype] + 64;
    GLDrawItem *items = realloc(
      gld_drawinfo.items[itemtype],
      newmaxitems * sizeof(gld_drawinfo.items[0][0]));
    if (!items)
    {
#ifdef PRBOOM_VITA_DIAGNOSTICS
      vita_diag_drawinfo_alloc_failure("draw-item table", newmaxitems * sizeof(gld_drawinfo.items[0][0]));
#endif
      return;
    }
    gld_drawinfo.items[itemtype] = items;
    gld_drawinfo.max_items[itemtype] = newmaxitems;
  }

  gld_drawinfo.items[itemtype][gld_drawinfo.num_items[itemtype]].item.item = item_p;
  gld_drawinfo.num_items[itemtype]++;
}
#undef SIZEOF8
#undef NEWSIZE
