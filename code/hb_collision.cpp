/*
 * Written by Gyuhyun Lee
 */

#include "hb_collision.h"

internal void
start_contact_group(ContactGroup *contact_group, MemoryArena *arena, u32 max_count)
{
    contact_group->data = push_array(arena, ContactData, max_count);
    contact_group->count = 0;
    contact_group->max_count = max_count;
}


internal void
generate_contact_if_applicable(ContactGroup *contact_group,
                                RigidBody *host, RigidBody *test)
{
    if(contact_group->count < contact_group->max_count)
    {
        Entity *host_entity = host->entity;
        Entity *test_entity = test->entity;

        // NOTE(gh) These are world space COM of the rigid body
        v3 com0 = host_entity->position + host->center_of_mass_offset;
        v3 com1 = test_entity->position + test->center_of_mass_offset;

        CollisionVolumeType *volume_type0 = (CollisionVolumeType *)(host->collision_volume);
        CollisionVolumeType *volume_type1 = (CollisionVolumeType *)(test->collision_volume);

        if(*volume_type0 == CollisionVolumeType_Sphere && *volume_type1 == CollisionVolumeType_Sphere)
        {
            // sphere - sphere collision

            CollisionVolumeSphere *sphere0 = (CollisionVolumeSphere *)host->collision_volume;
            CollisionVolumeSphere *sphere1 = (CollisionVolumeSphere *)test->collision_volume;

            v3 center_to_center = (com1 - com0);
            f32 distance_between_centers = length(center_to_center);
            if(distance_between_centers > 0.0f && 
                distance_between_centers < (sphere0->r + sphere1->r))
            {
                v3 contact_normal = center_to_center / distance_between_centers; // world space

#if 0
                v3 relative_velocity = host->velocity;

                ContactData *data = contact_group->contact_data + 
                                    contact_group->contact_data_used++;

                data->normal = contact_normal;
                // TODO(gh) Game Physics Development uses something weird that doesn't make sense, so double check!
                data->point = sphere0->center + sphere0->r * data->collision_normal;
                data->penetration_depth = sphere0->r + sphere1->r - distance_between_centers;
                data->collision_restitution = 1.0f;
#endif

                // TODO(gh) Also set friction
            }
        }
#if 0
        else if((*volume_type0 == CollisionVolumeType_Sphere && *volume_type1 == CollisionVolumeType_Plane) ||
                (*volume_type1 == CollisionVolumeType_Sphere && *volume_type0 == CollisionVolumeType_Plane))
        {
            // sphere - plane collision

            f32 distance_to_plane = dot(sphere->center, half_plane->normal) - half_plane->d;

            if(distance_to_plane < sphere->r)
            {
                ContactData *data = contact_group->contact_data + 
                                    contact_group->contact_data_used++;

                data->collision_normal = half_plane->normal;
                data->penetration_depth = sphere->r - distance_to_plane;
                data->contact_point = sphere->center - distance_to_plane * half_plane->normal;
                // TODO(gh) Also set friction, collision restitution
            }
        }
#endif
    }
}


















