#include "PrimeEngine/PrimeEngineIncludes.h"

#include "ParticleSystemNode.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "PrimeEngine/Events/StandardEvents.h"
#include "PrimeEngine/Scene/MeshManager.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#define DAMPENRATIO .65
#define MAXVARIANCE .9
namespace PE {
	namespace Components {

		PE_IMPLEMENT_CLASS1(ParticleSystemNode, SceneNode);


		// Constructor -------------------------------------------------------------
		ParticleSystemNode::ParticleSystemNode(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself)
			: SceneNode(context, arena, hMyself)
		{
			m_cachedAspectRatio = 1.0f;
			m_scale = 1.0f;

			if (IRenderer* pS = context.getGPUScreen())
				m_cachedAspectRatio = float(pS->getWidth()) / float(pS->getHeight());
		}

		void ParticleSystemNode::addDefaultComponents()
		{
			SceneNode::addDefaultComponents();
			m_base.setPos(Vector3(-1000, -1000, -100));

			// event handlers
			PE_REGISTER_EVENT_HANDLER(Events::Event_UPDATE, ParticleSystemNode::do_UPDATE);
			PE_REGISTER_EVENT_HANDLER(Events::Event_PRE_GATHER_DRAWCALLS, ParticleSystemNode::do_PRE_GATHER_DRAWCALLS);
		}

		void ParticleSystemNode::setSelfAndMeshAssetEnabled(bool enabled)
		{
			setEnabled(enabled);

			if (m_hMyTextMesh.isValid())
			{
				m_hMyTextMesh.getObject<Component>()->setEnabled(enabled);
			}
		}

		void ParticleSystemNode::activate()
		{
			alive = true;
			elapsedTime = 0.f;

			//randomly set position
			float t, r;
			t = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
			t *= 2 * 3.1415;

			r = sqrt((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)));

			r *= DAMPENRATIO;

			float x, z;

			x = r*cos(t);
			z = r*sin(t);

			m_base.setPos(m_base.getPos()+Vector3(x*MAXVARIANCE, 0, z* MAXVARIANCE));

			//set velocity based somewhat on position
			x *= -1; //actually sets the x value
			z *= -1;//actually sets the z value
			velocity.m_x = x*r;
			velocity.m_y = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 2;
			velocity.m_z = z*r;
		}

		void ParticleSystemNode::do_UPDATE(PE::Events::Event *pEvt)
		{
			PE::Events::Event_UPDATE* pRealEvent = (PE::Events::Event_UPDATE*)pEvt;

			elapsedTime += pRealEvent->m_frameTime;
			if (elapsedTime > LifeTime)
			{
				m_base.setPos(Vector3(-1000.f, -1000.f, -10000.f));
				alive = false;
			}
			else
			{
				stage = (elapsedTime / LifeTime) * 100;
				//update scale (linear for now)
				
				//update position here
				m_base.setPos(m_base.getPos() + velocity*pRealEvent->m_frameTime);
				//m_base.setPos(Vector3(-1000, -1000, -100));
			}
		}

		void ParticleSystemNode::loadFromString_needsRC(const char *str, DrawType drawType, int &threadOwnershipMask)
		{
			m_drawType = drawType;

			ParticleMesh *pTextMesh = NULL;
			if (m_hMyTextMesh.isValid())
			{
				pTextMesh = m_hMyTextMesh.getObject<ParticleMesh>();
			}
			else
			{
				m_hMyTextMesh = PE::Handle("ParticleMesh", sizeof(ParticleMesh));
				pTextMesh = new(m_hMyTextMesh)ParticleMesh(*m_pContext, m_arena, m_hMyTextMesh);
				pTextMesh->addDefaultComponents();

				m_pContext->getMeshManager()->registerAsset(m_hMyTextMesh);

				m_hMyTextMeshInstance = PE::Handle("MeshInstance", sizeof(MeshInstance));
				MeshInstance *pInstance = new(m_hMyTextMeshInstance)MeshInstance(*m_pContext, m_arena, m_hMyTextMeshInstance);
				pInstance->addDefaultComponents();
				pInstance->initFromRegisteredAsset(m_hMyTextMesh);


				addComponent(m_hMyTextMeshInstance);
			}

			const char *tech = 0;
			if (drawType == Overlay2D_3DPos || drawType == Overlay2D)
				tech = "StdMesh_2D_Diffuse_A_RGBIntensity_Tech2";
			if (drawType == InWorldFacingCamera)
				tech = "StdMesh_Diffuse_Tech2";

			pTextMesh->loadFromString_needsRC(str, tech, threadOwnershipMask);
		}

		void ParticleSystemNode::do_PRE_GATHER_DRAWCALLS(Events::Event *pEvt)
		{
			Events::Event_PRE_GATHER_DRAWCALLS *pDrawEvent = NULL;
			pDrawEvent = (Events::Event_PRE_GATHER_DRAWCALLS *)(pEvt);

			Matrix4x4 projectionViewWorldMatrix = pDrawEvent->m_projectionViewTransform;
			Matrix4x4 worldMatrix;

			if (!m_hMyTextMeshInstance.isValid())
				return;

			ParticleMesh *pTextMesh = m_hMyTextMesh.getObject<ParticleMesh>();

			if (m_drawType == InWorldFacingCamera)
			{
				m_worldTransform.turnTo(pDrawEvent->m_eyePos);
			}

			float numCharsInFullLine = 100.0f;
			numCharsInFullLine *= .5; // need to divide by 2.0 since screen goes from -1 to 1, not 0..1

			if (m_drawType == Overlay2D_3DPos)
			{
				if (alive)
				{
					worldMatrix = m_worldTransform;
					projectionViewWorldMatrix = projectionViewWorldMatrix * worldMatrix;

					Vector3 pos(0, 0, 0);
					pos = projectionViewWorldMatrix * pos;


					if (pos.m_x < -1.0f || pos.m_x > 1.0f || pos.m_z <= 0.0f || pos.m_z > 1.0f)
					{
						// this will cancel further event handling by this mesh only
						pEvt->m_cancelSiblingAndChildEventHandling = true;
						return;
					}

					float factor = 1.0f / (numCharsInFullLine);// * m_scale;
					Matrix4x4 scale;
					scale.importScale(factor, factor*m_cachedAspectRatio, 1.f);

					m_worldTransform.loadIdentity();
					m_worldTransform.setPos(Vector3(pos.m_x, pos.m_y, pos.m_z));
					m_worldTransform = m_worldTransform * scale;//*/
				}
			}
 			if (m_drawType == Overlay2D)
			{
				worldMatrix = m_worldTransform;

				float factor = 1.0f / (numCharsInFullLine)* m_scale;

				Matrix4x4 scale;
				//scale.importScale(factor, factor*m_cachedAspectRatio, 1.f);
				m_worldTransform = worldMatrix * scale;
			}


		}

	}; // namespace Components
}; // namespace PE
