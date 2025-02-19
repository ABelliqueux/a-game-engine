/*
 * A Game Engine
 *
 * (C) 2021 Arthur Carvalho de Souza Lima
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. 
 */

#include "actor/a_obj_syokudai.h"
#include "scene/scene.h"

#include "spadstk.h"

struct SGM2 * obj_syokudai_model;
struct SGM2 * obj_flame_model;
struct SGM2 * obj_flame_high_model;

void ObjSyokudaiActorSetup() {

}

void ObjSyokudaiActorInitialize(struct Actor * a, void * descriptor, void * scene) {
  ObjSyokudaiActor * actor = (ObjSyokudaiActor *)a;
  actor->base.Initialize = (PTRFUNC_3ARG) ObjSyokudaiActorInitialize;
  actor->base.Destroy = (PTRFUNC_2ARG) ObjSyokudaiActorDestroy;
  actor->base.Update = (PTRFUNC_2ARG) ObjSyokudaiActorUpdate;
  actor->base.Draw = (PTRFUNC_4ARGCHAR) ObjSyokudaiActorDraw;
  actor->base.visible = 1;
  actor->base.flags = ACTOR_STATIC;
  actor->body = obj_syokudai_model;
  actor->flame = obj_flame_model;
  actor->flame_high = obj_flame_high_model;
  // Actor Tranformation
  actor->body_matrix.t[0] = actor->base.pos.vx;
  actor->body_matrix.t[1] = actor->base.pos.vy;
  actor->body_matrix.t[2] = actor->base.pos.vz;
  actor->body_matrix.m[0][0] = 4096;
  actor->body_matrix.m[0][1] = 0;
  actor->body_matrix.m[0][2] = 0;
  actor->body_matrix.m[1][0] = 0;
  actor->body_matrix.m[1][1] = 4096;
  actor->body_matrix.m[1][2] = 0;
  actor->body_matrix.m[2][0] = 0;
  actor->body_matrix.m[2][1] = 0;
  actor->body_matrix.m[2][2] = 4096;
  
  actor->flame_matrix.t[0] = actor->base.pos.vx;
  actor->flame_matrix.t[1] = actor->base.pos.vy + 453;
  actor->flame_matrix.t[2] = actor->base.pos.vz;
  actor->flame_matrix.m[1][0] = 0;
  actor->flame_matrix.m[1][1] = 4096;
  actor->flame_matrix.m[1][2] = 0;
}

void ObjSyokudaiActorDestroy(struct Actor * a, void * scene) {

}

void ObjSyokudaiActorUpdate(struct Actor * a, void * scene) {
  //SVECTOR local_rotate, camera_inverse, camera_inverse_cross;
  ObjSyokudaiActor * actor = (ObjSyokudaiActor *)a;
  Actor * player;
  struct Scene_Ctx * scene_ctx = (struct Scene_Ctx*)scene;
  player = scene_ctx->player;
  struct Camera * camera = scene_ctx->camera;
  actor->flame_matrix.m[0][0] = camera->zaxis.vx;
  actor->flame_matrix.m[0][1] = camera->zaxis.vy;
  actor->flame_matrix.m[0][2] = camera->zaxis.vz;
  actor->flame_matrix.m[2][0] = camera->zaxis.vz;
  actor->flame_matrix.m[2][1] = camera->zaxis.vy;
  actor->flame_matrix.m[2][2] = -camera->zaxis.vx;
}

u_char * ObjSyokudaiActorDraw(struct Actor * a, MATRIX * view, u_char * packet_ptr, void * scene) {
  ObjSyokudaiActor * actor = (ObjSyokudaiActor *)a;
  MATRIX local_identity;
  CompMatrixLV(view, &actor->body_matrix, &local_identity);
  gte_SetRotMatrix(&local_identity);
  gte_SetTransMatrix(&local_identity);
  SetSpadStack(SPAD_STACK_ADDR);
  packet_ptr = SGM2_UpdateModel(actor->body, packet_ptr, (u_long*)G.pOt, 0, SGM2_RENDER_AMBIENT, scene);
  //ResetSpadStack();
  //SetSpadStack(SPAD_STACK_ADDR);
  packet_ptr =  ObjSyokudaiActorDrawFlame(a, view, packet_ptr, scene);
  ResetSpadStack();
  return packet_ptr;
}


// Simplified SGM renderer for flames
u_char * ObjSyokudaiActorDrawFlame(struct Actor * a, MATRIX * view, u_char * packet_ptr, void * scene) {
  MATRIX local_identity;
  SVECTOR local_rotate; 
  ObjSyokudaiActor * actor = (ObjSyokudaiActor*)a;
  struct SGM2 * model = actor->flame;
  POLY_GT3 * dest_pgt3_ptr;
  SGM2_POLYGT3 * pgt3_ptr;
  u_int i;
  u_short tpageid, tpageido;
  u_short clutid, clutido;
  u_short pg_count;
  u_char v_offs = actor->flame_rand;

  CompMatrixLV(view, &actor->flame_matrix, &local_identity);

  /*{
  VECTOR scale = {4096*2, 4096*2, 4096*2, 0};
  ScaleMatrixL(&local_identity, &scale);
  }*/

  gte_SetRotMatrix(&local_identity);
  gte_SetTransMatrix(&local_identity);

  // Select LOD
  //if(actor->flame_high != NULL) {
  SVECTOR vec0, vec1;
  long otzv;
  vec0.vx = 0;
  vec0.vy = 0;
  vec0.vz = 0;
  gte_ldv0(&vec0);
  gte_rtps();
  gte_stsxy((long*)&vec1.vx);
  //gte_stotz(&otzv);
  gte_stsz(&otzv);
  // Z culling
  if(otzv < 0) return packet_ptr;
  // Frustum culling
  if(vec1.vx < -64 || vec1.vx > SCREEN_W+64) return packet_ptr;
  if(vec1.vy < -64 || vec1.vy > SCREEN_H+64) return packet_ptr;
  if(otzv < 1700) model = actor->flame_high;
  //}

  clutido = clutid = model->material[0].clut;
  tpageido = tpageid = model->material[0].tpage;

  clutid += (actor->flame_color << 6);
  tpageido = (tpageido & ~(1<<5)) | (2 << 5);
  

  // Transform to buffer
  AGM_TransformModel(model->vertex_data, model->vertex_count);

  pgt3_ptr = model->poly_gt3;

  dest_pgt3_ptr = (POLY_GT3*)packet_ptr;
  pg_count = model->poly_gt3_count;

  for(i = 0; i < pg_count; i++, pgt3_ptr++) {
      long outer_product, otz;
      long tempz0,tempz1,tempz2;
      SVECTOR * vec0 = &TransformBuffer[pgt3_ptr->idx0];
      SVECTOR * vec1 = &TransformBuffer[pgt3_ptr->idx1];
      SVECTOR * vec2 = &TransformBuffer[pgt3_ptr->idx2];
            
      // Load screen XY coordinates to GTE Registers
      gte_ldsxy3(*(long*)&vec0->vx,*(long*)&vec1->vx,*(long*)&vec2->vx);
      // Load Z coordinates into GTE
      tempz0 = vec0->vz;
      tempz1 = vec1->vz;
      tempz2 = vec2->vz;
      gte_ldsz3(tempz0,tempz1,tempz2);
      // Get the average Z value
      gte_avsz3();
      // Store value to otz
      gte_stotz(&otz);
      // Reduce OTZ size
      otz = (otz >> OTSUBDIV);
      //otz = 50;
      if (otz > OTMIN && otz < OTSIZE) {
        long temp0, temp1, temp2;
        // If all tests have passed, now process the rest of the primitive.
        // Copy values already in the registers
        gte_stsxy3((long*)&dest_pgt3_ptr->x0,
                   (long*)&dest_pgt3_ptr->x1,
                   (long*)&dest_pgt3_ptr->x2);
        // Vertex Colors
        temp0 = *(long*)(&pgt3_ptr->r0);
        temp1 = *(long*)(&pgt3_ptr->r1);
        temp2 = *(long*)(&pgt3_ptr->r2);
        *(long*)(&dest_pgt3_ptr->r0) = temp0;
        *(long*)(&dest_pgt3_ptr->r1) = temp1;
        *(long*)(&dest_pgt3_ptr->r2) = temp2;

        // Set Texture Coordinates
        dest_pgt3_ptr->u0 = pgt3_ptr->u0;
        dest_pgt3_ptr->v0 = pgt3_ptr->v0 + v_offs;
        dest_pgt3_ptr->u1 = pgt3_ptr->u1;
        dest_pgt3_ptr->v1 = pgt3_ptr->v1 + v_offs;
        dest_pgt3_ptr->u2 = pgt3_ptr->u2;
        dest_pgt3_ptr->v2 = pgt3_ptr->v2 + v_offs;
        
        // Set CLUT
        dest_pgt3_ptr->clut = clutid;
        // Set Texture Page
        dest_pgt3_ptr->tpage = tpageid;

        setPolyGT3(dest_pgt3_ptr);

        setSemiTrans(dest_pgt3_ptr, 1);
        
        addPrim(G.pOt+otz, dest_pgt3_ptr);
        /*POLY_GT3* dupli = dest_pgt3_ptr;
        dest_pgt3_ptr++;
        *dest_pgt3_ptr = *dupli;
        dest_pgt3_ptr->clut = clutido;
        dest_pgt3_ptr->tpage = tpageido;
        addPrim(G.pOt+otz, dest_pgt3_ptr);*/
        dest_pgt3_ptr++;

      }
  }

  return (char*)dest_pgt3_ptr;
}