/*  SSNES - A Super Nintendo Entertainment System (SNES) Emulator frontend for libsnes.
 *  Copyright (C) 2010-2011 - Hans-Kristian Arntzen
 *
 *  Some code herein may be based on code found in BSNES.
 * 
 *  SSNES is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  SSNES is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with SSNES.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "snes_state.h"
#include <stdlib.h>
#include <assert.h>
#include "strl.h"

struct snes_tracker_internal
{
   char id[64];

   const uint8_t *ptr;
   uint32_t addr;
   uint8_t mask;

   enum snes_tracker_type type;

   uint8_t prev[2];
   int frame_count;
   uint8_t old_value; 
};

struct snes_tracker
{
   struct snes_tracker_internal *info;
   unsigned info_elem;
};

snes_tracker_t* snes_tracker_init(const struct snes_tracker_info *info)
{
   snes_tracker_t *tracker = calloc(1, sizeof(*tracker));
   if (!tracker)
      return NULL;

   tracker->info = calloc(info->info_elem, sizeof(struct snes_tracker_internal));
   tracker->info_elem = info->info_elem;

   for (unsigned i = 0; i < info->info_elem; i++)
   {
      strlcpy(tracker->info[i].id, info->info[i].id, sizeof(tracker->info[i].id));
      tracker->info[i].addr = info->info[i].addr;
      tracker->info[i].type = info->info[i].type;
      tracker->info[i].mask = (info->info[i].mask == 0) ? 0xff : info->info[i].mask;

      assert(info->wram && info->vram && info->cgram &&
            info->oam && info->apuram);

      switch (info->info[i].ram_type)
      {
         case SSNES_STATE_WRAM:
            tracker->info[i].ptr = info->wram;
            break;
         case SSNES_STATE_APURAM:
            tracker->info[i].ptr = info->apuram;
            break;
         case SSNES_STATE_OAM:
            tracker->info[i].ptr = info->oam;
            break;
         case SSNES_STATE_CGRAM:
            tracker->info[i].ptr = info->cgram;
            break;
         case SSNES_STATE_VRAM:
            tracker->info[i].ptr = info->vram;
            break;

         default:
            tracker->info[i].ptr = NULL;
      }
   }


   return tracker;
}

void snes_tracker_free(snes_tracker_t *tracker)
{
   free(tracker->info);
   free(tracker);
}

#define fetch(addr) (info->ptr[info->addr] & info->mask)

static void update_element(
      struct snes_tracker_uniform *uniform,
      struct snes_tracker_internal *info,
      unsigned frame_count)
{
   uniform->id = info->id;

   switch (info->type)
   {
      case SSNES_STATE_CAPTURE:
         uniform->value = fetch(addr);
         break;

      case SSNES_STATE_CAPTURE_PREV:
         if (info->prev[0] != fetch(addr))
         {
            info->prev[1] = info->prev[0];
            info->prev[0] = fetch(addr);
         }
         uniform->value = info->prev[1];
         break;

      case SSNES_STATE_TRANSITION:
         if (info->old_value != fetch(addr))
         {
            info->old_value = fetch(addr);
            info->frame_count = frame_count;
         }
         uniform->value = info->frame_count;

         break;

      case SSNES_STATE_TRANSITION_PREV:
         if (info->prev[0] != fetch(addr))
         {
            info->old_value = fetch(addr);
            info->prev[1] = info->prev[0];
            info->prev[0] = frame_count;
            info->frame_count = frame_count;
         }
         uniform->value = info->prev[1];

         break;
      
      default:
         break;
   }
}

#undef fetch

unsigned snes_get_uniform(snes_tracker_t *tracker, struct snes_tracker_uniform *uniforms, unsigned elem, unsigned frame_count)
{
   unsigned elems = tracker->info_elem < elem ? tracker->info_elem : elem;

   for (unsigned i = 0; i < elems; i++)
      update_element(&uniforms[i], &tracker->info[i], frame_count);

   return elems;
}

