#ifndef __PYENGINE_2_0_PARTICLESYSTEMNODE_H__
#define __PYENGINE_2_0_PARTICLESYSTEMNODE_H__

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <assert.h>
#include <list>

// Inter-Engine includes
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "PrimeEngine/PrimitiveTypes/PrimitiveTypes.h"
#include "PrimeEngine/Events/Component.h"
#include "PrimeEngine/Utils/Array/Array.h"
#include "PrimeEngine/Scene/SceneNode.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "ParticleMesh.h"


//#define USE_DRAW_COMPONENT

namespace PE {
	namespace Components {
		struct ParticleSystemNode : public SceneNode
		{
			PE_DECLARE_CLASS(ParticleSystemNode);

			// Constructor -------------------------------------------------------------
			ParticleSystemNode(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself);

			virtual ~ParticleSystemNode() {}

			// Component ------------------------------------------------------------

			virtual void addDefaultComponents();

			void setSelfAndMeshAssetEnabled(bool enabled);
			// Individual events -------------------------------------------------------

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_PRE_GATHER_DRAWCALLS);
			virtual void do_PRE_GATHER_DRAWCALLS(Events::Event *pEvt);

			PE_DECLARE_IMPLEMENT_EVENT_HANDLER_WRAPPER(do_UPDATE);
			virtual void do_UPDATE(PE::Events::Event *pEvt);

			enum DrawType
			{
				InWorld,
				InWorldFacingCamera,
				Overlay2D,
				Overlay2D_3DPos
			};

			void loadFromString_needsRC(const char *str, DrawType drawType, int &threadOwnershipMask);

			void activate();

			DrawType m_drawType;
			float m_scale;
			Handle m_hMyTextMesh;
			Handle m_hMyTextMeshInstance;
			float m_cachedAspectRatio;

			bool m_canBeRecreated;

			bool alive;
			float elapsedTime;
			float LifeTime;
			Vector3 velocity;
			int stage;

		}; // class TextSceneNode

	}; // namespace Components
}; // namespace PE
#endif
