#include "std_include.hpp"

constexpr bool USE_PVD = false; // PhysX Visual Debugger

namespace components
{
	physx_impl* physx_impl::p_this = nullptr;

	constexpr int g_phys_minMsecStep = 3;	// 11
	constexpr int g_phys_maxMsecStep = 11;	// 67

	void physx_impl::run_frame(float seconds)
	{
		mScene->simulate(seconds);
		mScene->fetchResults(true);
	}

	void physx_impl::frame()
	{
		const auto fxs = fx_system::FX_GetSystem(0);
		const auto efx = fx_system::ed_active_effect;

		m_simulation_running = false;
		m_effect_is_using_physics = false;

		if (efx && fxs)
		{
			for (int elemDefIndex = 0; elemDefIndex != efx->def->elemDefCountLooping; ++elemDefIndex)
			{
				if (efx->def->elemDefs[elemDefIndex].elemType == fx_system::FX_ELEM_TYPE_MODEL && (efx->def->elemDefs[elemDefIndex].flags & fx_system::FX_ELEM_USE_MODEL_PHYSICS) != 0)
				{
					m_effect_is_using_physics = true;
					break;
				}
			}

			if (!m_effect_is_using_physics)
			{
				return;
			}

			const auto time_now = fxs->msecNow;

			if (m_time_last_update < time_now)
			{
				m_time_last_snapshot = m_time_last_update;

				auto maxIter = 2u;
				for (auto i = 2u; ; maxIter = i)
				{
					if (!maxIter)
					{
						Assert();
					}

					const auto delta = (time_now - m_time_last_update) / static_cast<int>(maxIter);
					auto step = m_phys_msec_step;

					if (step < delta)
					{
						step = delta;
					}

					--i;

					m_simulation_running = true;

					physx_impl::run_frame(static_cast<float>(step) * 0.001f);
					m_time_last_update += step;


					// #
					// dxPostProcessIslands(static_cast<PhysWorld>(worldIndex));

					mScene->getActiveActors(m_active_body_count);


					constexpr float REDUCE_MSEC_BEGIN_AT_COUNT = 64.0f; // object count needed to start increasing m_phys_msec_step # og: 32
					constexpr float REDUCE_MSEC_RANGE_TO_MAX = 64.0f;   // range - how many objects are needed to hit g_phys_maxMsecStep # og: 18

					const auto step_for_count = (static_cast<float>(m_active_body_count) - REDUCE_MSEC_BEGIN_AT_COUNT) / REDUCE_MSEC_RANGE_TO_MAX;
					const auto s0 = step_for_count - 1.0f < 0.0f ? step_for_count : 1.0f;
					const auto s1 = 0.0f - step_for_count < 0.0f ? s0 : 0.0f;

					m_phys_msec_step = g_phys_minMsecStep + static_cast<int>((static_cast<float>((g_phys_maxMsecStep - g_phys_minMsecStep)) * s1));

					// #

					if (m_time_last_update >= time_now)
					{
						break;
					}
				}
			}

			const physx::PxRenderBuffer& rb = mScene->getRenderBuffer();
			for (physx::PxU32 i = 0; i < rb.getNbLines(); i++)
			{
				const auto& line = rb.getLines()[i];

				game::GfxPointVertex vert[2];
				vert[0].xyz[0] = line.pos0.x;
				vert[0].xyz[1] = line.pos0.y;
				vert[0].xyz[2] = line.pos0.z;
				vert[0].color.packed = line.color0;

				vert[1].xyz[0] = line.pos1.x;
				vert[1].xyz[1] = line.pos1.y;
				vert[1].xyz[2] = line.pos1.z;
				vert[1].color.packed = line.color1;

				renderer::R_AddLineCmd(1, 4, 3, vert);

				if (mScene->getVisualizationParameter(physx::PxVisualizationParameter::eCONTACT_POINT) > 0.0f)
				{
					renderer::R_AddPointCmd(1, 12, 3, vert);
				}
			}

			// physx seems to not use points at all
			/*for (physx::PxU32 i = 0; i < rb.getNbPoints(); i++)
			{
				const auto& point = rb.getPoints()[i];

				game::GfxPointVertex vert;
				vert.xyz[0] = point.pos.x;
				vert.xyz[1] = point.pos.y;
				vert.xyz[2] = point.pos.z;
				vert.color.packed = point.color;

				renderer::R_AddLineCmd(1, 4, 3, &vert);
			}*/


			if (m_time_last_snapshot > time_now || time_now > m_time_last_update)
			{
				Assert();
			}

			if (m_time_last_update <= m_time_last_snapshot)
			{
				if (m_time_last_update != m_time_last_snapshot)
				{
					Assert();
				}

				m_time_now_lerp_frac = 1.0f;
			}
			else
			{
				auto delta = static_cast<float>((time_now - m_time_last_snapshot)) / static_cast<float>((m_time_last_update - m_time_last_snapshot));
				m_time_now_lerp_frac = delta;

				if (delta < 0.0f || delta > 1.0f)
				{
					Assert();
				}
			}
		}
	}

	physx::PxMaterial* physx_impl::create_material(game::PhysPreset* preset)
	{
		return mPhysics->createMaterial(preset->friction, preset->friction, preset->bounce);
	}

	// no nullptr checks besides the selected brush itself
	// does not exclude brushes
	void physx_impl::create_static_brush(game::selbrush_def_t* sb, bool is_prefab, const game::vec3_t position_offset, const float* quat)
	{
		if (sb)
		{
			std::vector<physx::PxVec3> verts;
			verts.reserve(50);

			game::vec3_t brush_center;
			utils::vector::add(sb->def->mins, sb->def->maxs, brush_center);
			utils::vector::scale(brush_center, 0.5f, brush_center);

			for (auto f = 0; f < sb->def->facecount; f++)
			{
				const auto face = &sb->def->brush_faces[f];
				for (auto p = 0; face->w && p < face->w->numPoints; p++)
				{
					game::vec3_t tw_point = { face->w->points[p][0], face->w->points[p][1], face->w->points[p][2] };

					if(!is_prefab)
					{
						utils::vector::subtract(tw_point, brush_center, tw_point);
					}
					
					const physx::PxVec3 t_point = { tw_point[0], tw_point[1], tw_point[2] };
					bool contains = false;

					for (auto x = 0; x < static_cast<int>(verts.size()); x++)
					{
						if (utils::vector::compare(&verts[x].x, &t_point[0]))
						{
							contains = true;
							break;
						}
					}

					if (!contains)
					{
						verts.push_back(t_point);
					}
				}
			}


			physx::PxConvexMeshDesc convexDesc;
			convexDesc.points.count = verts.size();
			convexDesc.points.stride = sizeof(physx::PxVec3);
			convexDesc.points.data = verts.data();
			convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

			physx::PxDefaultMemoryOutputStream buf;
			physx::PxConvexMeshCookingResult::Enum result;

			if (mCooking->cookConvexMesh(convexDesc, buf, &result))
			{
				physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
				physx::PxConvexMesh* convexMesh = mPhysics->createConvexMesh(input);

				const physx::PxTransform t
				(
					brush_center[0], brush_center[1], brush_center[2]
				);

				// do not offset prefab brushes by their brush center
				auto* actor = mPhysics->createRigidStatic(is_prefab ? physx::PxTransform(0.0f, 0.0f, 0.0f) : t);
				actor->setActorFlags(physx::PxActorFlag::eVISUALIZATION);

				physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, physx::PxConvexMeshGeometry(convexMesh), *mMaterial);

				physx::PxTransform local;
				local.p = physx::PxVec3(0.0f, 0.0f, 0.0f);
				local.q = physx::PxQuat(0.0f, 0.0f, 0.0f, 1.0f);

				// mainly for prefab brushes
				if (position_offset || quat)
				{
					if (position_offset)
					{
						local.p = physx::PxVec3(position_offset[0], position_offset[1], position_offset[2]);
					}

					if (quat)
					{
						local.q = physx::PxQuat(quat[0], quat[1], quat[2], quat[3]);
					}

					shape->setLocalPose(local);
				}
				
				actor->attachShape(*shape);
				shape->release();

				mScene->addActor(*actor);

				m_static_brushes.push_back(actor);
				m_static_brush_count++;
			}
		}
	}

	void physx_impl::create_static_terrain(game::selbrush_def_t* sb, const game::vec3_t position_offset, const float* quat)
	{
		// clipmap verts
		/*verts.reserve(100);
		for (auto v = 0u; v < components::d3dbsp::cm.vertCount; v++)
		{
			physx::PxVec3 tv = { components::d3dbsp::cm.verts[v][0], components::d3dbsp::cm.verts[v][1], components::d3dbsp::cm.verts[v][2] };
			verts.emplace_back(tv);
		}

		inds.reserve(100);
		for (auto i = 0; i < components::d3dbsp::cm.triCount * 3; i += 3)
		{
			inds.emplace_back(static_cast<int>(components::d3dbsp::cm.triIndices[i + 2]));
			inds.emplace_back(static_cast<int>(components::d3dbsp::cm.triIndices[i + 1]));
			inds.emplace_back(static_cast<int>(components::d3dbsp::cm.triIndices[i + 0]));
		}

		physx::PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = components::d3dbsp::cm.vertCount;
		meshDesc.points.stride = sizeof(physx::PxVec3);
		meshDesc.points.data = verts.data();

		meshDesc.triangles.count = components::d3dbsp::cm.triCount * 3;
		meshDesc.triangles.stride = 3 * sizeof(uint32_t);
		meshDesc.triangles.data = inds.data();*/


		// reference - Q3 - Terrain_GetTriangle

		const auto phys = components::physx_impl::get();
		const auto patch = sb->def->patch;

		std::vector<physx::PxVec3> verts;
		std::vector<uint32_t> inds;
		int tri_count = 0;


		if ((sb->def->patch->type & game::PATCH_TERRAIN) != 0)
		{
			for (auto x = 0; x < patch->width - 1; x++)
			{
				for (auto y = 0; y < patch->height; y++)
				{
					if (y != patch->height - 1)
					{
						const game::vec_t* v1;
						const game::vec_t* v2;
						const game::vec_t* v3;

						if (patch->ctrl[x][y].turned_edge)
						{
							v1 = patch->ctrl[x + 1][y + 0].xyz;
							v2 = patch->ctrl[x + 1][y + 1].xyz;
							v3 = patch->ctrl[x + 0][y + 0].xyz;
						}
						else
						{
							v1 = patch->ctrl[x + 0][y + 0].xyz;
							v2 = patch->ctrl[x + 1][y + 0].xyz;
							v3 = patch->ctrl[x + 0][y + 1].xyz;
						}

						verts.emplace_back(physx::PxVec3(v1[0], v1[1], v1[2]));
						verts.emplace_back(physx::PxVec3(v2[0], v2[1], v2[2]));
						verts.emplace_back(physx::PxVec3(v3[0], v3[1], v3[2]));

						inds.push_back(inds.size());
						inds.push_back(inds.size());
						inds.push_back(inds.size());

						tri_count++;
					}

					if (y != 0)
					{
						const game::vec_t* v1;
						const game::vec_t* v2;
						const game::vec_t* v3;

						if (patch->ctrl[x][y - 1].turned_edge)
						{
							v1 = patch->ctrl[x + 0][y + 0].xyz;
							v2 = patch->ctrl[x + 0][y - 1].xyz;
							v3 = patch->ctrl[x + 1][y + 0].xyz;
						}
						else
						{
							v1 = patch->ctrl[x + 1][y + 0].xyz;
							v2 = patch->ctrl[x + 0][y + 0].xyz;
							v3 = patch->ctrl[x + 1][y - 1].xyz;
						}

						verts.emplace_back(physx::PxVec3(v1[0], v1[1], v1[2]));
						verts.emplace_back(physx::PxVec3(v2[0], v2[1], v2[2]));
						verts.emplace_back(physx::PxVec3(v3[0], v3[1], v3[2]));

						inds.push_back(inds.size());
						inds.push_back(inds.size());
						inds.push_back(inds.size());

						tri_count++;
					}
				}
			}
		}

		// curve
		else
		{
			const auto width = patch->curveDef->width;
			const auto height = patch->curveDef->height;

			for (auto x = 0; x < width - 1; x++)
			{
				for (auto y = 0; y < height; y++)
				{
					if (y != height - 1)
					{
						const game::vec_t* v1;
						const game::vec_t* v2;
						const game::vec_t* v3;

						{
							v1 = patch->curveDef->verts[x + 0 + (width * (y + 0))].xyz;
							v2 = patch->curveDef->verts[x + 1 + (width * (y + 0))].xyz;
							v3 = patch->curveDef->verts[x + 0 + (width * (y + 1))].xyz;
						}

						verts.emplace_back(physx::PxVec3(v1[0], v1[1], v1[2]));
						verts.emplace_back(physx::PxVec3(v2[0], v2[1], v2[2]));
						verts.emplace_back(physx::PxVec3(v3[0], v3[1], v3[2]));

						inds.push_back(inds.size());
						inds.push_back(inds.size());
						inds.push_back(inds.size());

						tri_count++;
					}

					if (y != 0)
					{
						const game::vec_t* v1;
						const game::vec_t* v2;
						const game::vec_t* v3;

						{
							v1 = patch->curveDef->verts[x + 1 + (width * (y + 0))].xyz;
							v2 = patch->curveDef->verts[x + 0 + (width * (y + 0))].xyz;
							v3 = patch->curveDef->verts[x + 1 + (width * (y - 1))].xyz;
						}

						verts.emplace_back(physx::PxVec3(v1[0], v1[1], v1[2]));
						verts.emplace_back(physx::PxVec3(v2[0], v2[1], v2[2]));
						verts.emplace_back(physx::PxVec3(v3[0], v3[1], v3[2]));

						inds.push_back(inds.size());
						inds.push_back(inds.size());
						inds.push_back(inds.size());

						tri_count++;
					}
				}
			}
		}
		

		physx::PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = verts.size();
		meshDesc.points.stride = sizeof(physx::PxVec3);
		meshDesc.points.data = verts.data();

		meshDesc.triangles.count = tri_count;
		meshDesc.triangles.stride = 3 * sizeof(uint32_t);
		meshDesc.triangles.data = inds.data();

		physx::PxDefaultMemoryOutputStream buf;
		physx::PxTriangleMeshCookingResult::Enum result;

		if (phys->mCooking->cookTriangleMesh(meshDesc, buf, &result))
		{
			physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
			physx::PxTriangleMesh* triangleMesh = phys->mPhysics->createTriangleMesh(input);

			auto* actor = phys->mPhysics->createRigidStatic(physx::PxTransform(0.0f, 0.0f, 0.0f));
			actor->setActorFlags(physx::PxActorFlag::eVISUALIZATION);

			physx::PxShape* shape = physx::PxRigidActorExt::createExclusiveShape(*actor, physx::PxTriangleMeshGeometry(triangleMesh), *phys->mMaterial);

			physx::PxTransform local;
			local.p = physx::PxVec3(0.0f, 0.0f, 0.0f);
			local.q = physx::PxQuat(0.0f, 0.0f, 0.0f, 1.0f);

			// mainly for prefab brushes
			if (position_offset || quat)
			{
				if (position_offset)
				{
					local.p = physx::PxVec3(position_offset[0], position_offset[1], position_offset[2]);
				}

				if (quat)
				{
					local.q = physx::PxQuat(quat[0], quat[1], quat[2], quat[3]);
				}

				shape->setLocalPose(local);
			}

			actor->attachShape(*shape);
			shape->release();

			phys->mScene->addActor(*actor);
			phys->m_static_terrain.push_back(actor);
		}
	}

	bool exclude_brushes_from_static_collision(game::selbrush_def_t* b)
	{
		// skip sky
		if (b->def->contents & game::BRUSHCONTENTS_SKY)
		{
			return true;
		}

		// skip fixed size objects
		if (b->brushflags & game::BRUSHFLAG_FIXED_SIZE)
		{
			return true;
		}

		// skip all nodes (reflection_probes, script_origins, lights etc)
		if (b->owner && b->owner->firstActive && b->owner->firstActive->eclass)
		{
			const auto class_type = b->owner->firstActive->eclass->classtype;

			if (class_type & game::ECLASS_RADIANT_NODE)
			{
				return true;
			}
		}

		// include all generic solid + detail and weaponclip brushes
		if (!(b->brushflags & game::BRUSHFLAG_SOLID) && (b->def->contents & game::BRUSHCONTENTS_DETAIL || b->def->contents & game::BRUSHCONTENTS_WEAPONCLIP))
		{
			return true;
		}

		// skip all tooling (spawns, lightgrid ...) but include (some) clip
		if ((b->brushflags & game::BRUSHFLAG_TOOL) && !(b->def->contents & 0x10000 || b->def->contents & 0x20000 || b->def->contents & 0x30000))
		{
			return true;
		}

		return false;
	}


	// only call via components::process
	void physx_impl::create_static_collision()
	{
		const auto phys = components::physx_impl::get();
		phys->m_static_brush_estimated_count = 0;
		phys->m_static_brush_count = 0;
		phys->m_static_terrain_estimated_count = 0;
		phys->m_static_terrain_count = 0;

		for (const auto brush : phys->m_static_brushes)
		{
			brush->release();
		}
		phys->m_static_brushes.clear();
		phys->m_static_brushes.reserve(1000);

		// #

		for (const auto terrain : phys->m_static_terrain) 
		{
			terrain->release();
		}
		phys->m_static_terrain.clear();
		phys->m_static_terrain.reserve(100);


		// #

		const auto process = process::get();

		// #
		// count all brushes (used for progressbar)

 		FOR_ALL_ACTIVE_BRUSHES(sb)
		{
			if (process->pending_termination())
			{
				return;
			}

			// prefabs
			if (sb && sb->owner && sb->owner->prefab && sb->owner->firstActive && sb->owner->firstActive->eclass && sb->owner->firstActive->eclass->classtype & game::ECLASS_PREFAB)
			{
				FOR_ALL_BRUSHES(prefab, sb->owner->prefab->active_brushlist, sb->owner->prefab->active_brushlist_next)
				{
					if (process->pending_termination())
					{
						return;
					}

					if (prefab && prefab->def)
					{
						// patches
						if (prefab->def->patch)
						{
							if (sb->def->contents & game::BRUSHCONTENTS_NONCOLLIDING)
							{
								continue;
							}

							phys->m_static_terrain_estimated_count++;
							continue;
						}

						// brushes
						if (exclude_brushes_from_static_collision(prefab))
						{
							continue;
						}

						phys->m_static_brush_estimated_count++;
					}
				}
			}

			// root map
			else if (sb && sb->def)
			{
				// patches
				if (sb->def->patch)
				{
					if (sb->def->contents & game::BRUSHCONTENTS_NONCOLLIDING)
					{
						continue;
					}

					phys->m_static_terrain_estimated_count++;
					continue;
				}

				// brushes
				if (exclude_brushes_from_static_collision(sb))
				{
					continue;
				}

				phys->m_static_brush_estimated_count++;
			}
		}


		// #
		// generate static collision

		FOR_ALL_ACTIVE_BRUSHES(sb)
		{
			if (process->pending_termination())
			{
				return;
			}

			// prefab brushes and terrain
			if (sb && sb->owner && sb->owner->prefab && sb->owner->firstActive && sb->owner->firstActive->eclass && sb->owner->firstActive->eclass->classtype & game::ECLASS_PREFAB)
			{
				FOR_ALL_BRUSHES(prefab, sb->owner->prefab->active_brushlist, sb->owner->prefab->active_brushlist_next)
				{
					if (process->pending_termination())
					{
						return;
					}

					if (prefab && prefab->def)
					{
						const auto is_patch = prefab->def->patch;
						if (is_patch)
						{
							if (sb->def->contents & game::BRUSHCONTENTS_NONCOLLIDING)
							{
								continue;
							}
						}
						else if (exclude_brushes_from_static_collision(prefab))
						{
							continue;
						}

						game::vec3_t prefab_angles = {};
						game::vec4_t quat = { 0.0f, 0.0f, 0.0f, 1.0f };

						// angles to quat - use identity if prefab has no angles kvp
						if (GET_GUI(ggui::entity_dialog)->get_vec3_for_key_from_entity(sb->owner->firstActive, prefab_angles, "angles"))
						{
							game::orientation_t orientation = {};
							game::AnglesToAxis(prefab_angles, &orientation.axis[0][0]);
							fx_system::AxisToQuat(orientation.axis, quat);
						}

						if (is_patch)
						{
							phys->create_static_terrain(prefab, sb->owner->firstActive->origin, quat);
						}
						else
						{
							phys->create_static_brush(prefab, true, sb->owner->firstActive->origin, quat);
						}
					}
				}
			}

			// root map brushes and terrain
			else if (sb && sb->def)
			{
				// patches
				if (sb->def->patch)
				{
					if (sb->def->contents & game::BRUSHCONTENTS_NONCOLLIDING)
					{
						continue;
					}

					phys->create_static_terrain(sb);
				}

				// brushes
				else
				{
					// skip brushes that should not be part of the static collision
					if (exclude_brushes_from_static_collision(sb))
					{
						continue;
					}

					phys->create_static_brush(sb);
				}
			}
		}

		phys->m_static_brush_count = phys->m_static_brushes.size();
		phys->m_static_brush_estimated_count = 0;

		phys->m_static_terrain_count = phys->m_static_terrain.size();
		phys->m_static_terrain_estimated_count = 0;
	}


	void physx_impl::obj_destroy(int id)
	{
		const auto shape = reinterpret_cast<physx::PxShape*>(id);
		const auto actor = shape->getActor();

		if (actor->userData)
		{
			const auto material = static_cast<physx::PxMaterial*>(actor->userData);
			material->release();
		}

		actor->release();
	}

	void physx_impl::obj_get_interpolated_state(int id, float* out_pos, float* out_quat)
	{
		const auto shape = reinterpret_cast<physx::PxShape*>(id);

		const physx::PxQuat quat = shape->getActor()->getGlobalPose().q;
		const auto pos = shape->getActor()->getGlobalPose().p;
		out_pos[0] = pos.x;
		out_pos[1] = pos.y;
		out_pos[2] = pos.z;

		out_quat[0] = quat.x;
		out_quat[1] = quat.y;
		out_quat[2] = quat.z;
		out_quat[3] = quat.w;
	}

	int physx_impl::create_physx_object(game::XModel* model, const float* world_pos, const float* quat)
	{
		const auto material = create_material(model->physPreset);

		game::vec3_t half_bounds;
		utils::vector::subtract(model->maxs, model->mins, half_bounds);
		utils::vector::scale(half_bounds, 0.5f, half_bounds);

		const auto box_geom = physx::PxBoxGeometry(half_bounds[0], half_bounds[1], half_bounds[2]);
		physx::PxShape* shape = mPhysics->createShape(box_geom, *mMaterial, true);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, true);

		game::vec3_t origin_offset;
		utils::vector::subtract(model->maxs, half_bounds, origin_offset);
		shape->setLocalPose(physx::PxTransform(origin_offset[0], origin_offset[1], origin_offset[2]));

		const physx::PxTransform t
		(
			world_pos[0], world_pos[1], world_pos[2],
			physx::PxQuat(quat[0], quat[1], quat[2], quat[3])
		);

		physx::PxRigidDynamic* body = mPhysics->createRigidDynamic(t);

		body->setActorFlags(physx::PxActorFlag::eVISUALIZATION);
		body->userData = material;
		body->attachShape(*shape);
		shape->release();

		/*const physx::PxVec3 center_of_mass =
		{
			(model->mins[0] + model->maxs[0]) * 0.5f,
			(model->mins[1] + model->maxs[1]) * 0.5f,
			(model->mins[2] + model->maxs[2]) * 0.5f,
		};*/

		physx::PxRigidBodyExt::updateMassAndInertia(*body, model->physPreset->mass/*, &center_of_mass*/);

		//auto m = body->getMass();
		//body->setMass(model->physPreset->mass);

		mScene->addActor(*body);

		return reinterpret_cast<int>(shape);
	}

	static physx::PxRigidStatic* groundPlane;
	void physx_impl::create_plane()
	{
		auto gui = GET_GUI(ggui::camera_settings_dialog);

		if (mMaterial)
		{
			mMaterial->release();
		}
		
		mMaterial = mPhysics->createMaterial(gui->phys_material[0], gui->phys_material[1], gui->phys_material[2]);

		if (groundPlane)
		{
			groundPlane->release();
		}

		groundPlane = PxCreatePlane(*mPhysics, physx::PxPlane(gui->phys_plane[0], gui->phys_plane[1], gui->phys_plane[2], gui->phys_plane[3]), *mMaterial);

		if (groundPlane)
		{
			mScene->addActor(*groundPlane);
		}
		else
		{
			ImGuiToast toast(ImGuiToastType_Error, 4000);
			toast.set_title("Invalid groundplane settings!");
			ImGui::InsertNotification(toast);
		}
		
	}

	void physx_impl::register_dvars()
	{
		dvars::physx_debug_visualization_box_size = dvars::register_float(
			/* name		*/ "physx_debug_visualization_box_size",
			/* default	*/ 2000.0f,
			/* mins		*/ 0.0f,
			/* maxs		*/ FLT_MAX,
			/* flags	*/ game::dvar_flags::saved,
			/* desc		*/ "size of culling box in which to draw debug visualizations. 0 disables the culling box");
	}

	physx_impl::physx_impl()
	{
		physx_impl::p_this = this;

		m_simulation_running = false;
		m_effect_is_using_physics = false;

		m_time_last_snapshot = 0;
		m_time_last_update = 0;
		m_time_now_lerp_frac = 0;

		m_phys_msec_step = 3;
		m_active_body_count = 0;
		m_static_brush_count = 0;
		m_static_brush_estimated_count = 0;
		m_static_terrain_estimated_count = 0;
		m_static_terrain_count = 0;

		// #

		mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mDefaultAllocatorCallback, mDefaultErrorCallback);
		if (!mFoundation)
		{
			AssertS("PxCreateFoundation failed!");
		}

		mCooking = PxCreateCooking(PX_PHYSICS_VERSION, *mFoundation, mToleranceScale);
		if (!mCooking)
		{
			AssertS("PxCreateCooking failed!");
		}

		if (USE_PVD)
		{
			physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
			mPvd = PxCreatePvd(*mFoundation);
			mPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
		}

		mToleranceScale.length = 1;  // typical length of an object
		mToleranceScale.speed = 100; // typical speed of an object, gravity*1s is a reasonable choice
		mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, mToleranceScale, true, USE_PVD ? mPvd : nullptr);
		mDispatcher = physx::PxDefaultCpuDispatcherCreate(2);

		/*if (!PxInitExtensions(*mPhysics, mPvd))
		{
			AssertS("PxInitExtensions failed!");
		}*/

		physx::PxSceneDesc scene_desc(mPhysics->getTolerancesScale());
		scene_desc.gravity = physx::PxVec3(0.0f, 0.0f, -800.0f); // default: -9.81 // scale of 80
		scene_desc.bounceThresholdVelocity = 1400.0f; // default: 20
		scene_desc.cpuDispatcher = mDispatcher;
		scene_desc.filterShader = physx::PxDefaultSimulationFilterShader;
		scene_desc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
		mScene = mPhysics->createScene(scene_desc);

		//auto xx = mScene->getBounceThresholdVelocity();
		//mScene->getFrictionOffsetThreshold();

		// do not ship with 1 enabled by default!
		//mScene->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, 1.0f);
		//mScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_AABBS, 1.0f);

		if (USE_PVD)
		{
			physx::PxPvdSceneClient* pvdClient = mScene->getScenePvdClient();
			if (pvdClient)
			{
				pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
				pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
				pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
			}
		}

		// add a simple ground plane for now
		mMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.6f);
		groundPlane = PxCreatePlane(*mPhysics, physx::PxPlane(0, 0, 1, 0), *mMaterial);
		mScene->addActor(*groundPlane);

		// #

		components::command::register_command("physx_plane"s, [this](auto)
		{
			create_plane();
		});
	}

	physx_impl::~physx_impl()
	{
		//PxCloseExtensions();

		mCooking->release();
		mPhysics->release();
		mFoundation->release();
	}
}
