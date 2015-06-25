#ifndef __PYENGINE_2_0_SOUND_MANAGER__
#define __PYENGINE_2_0_SOUND_MANAGER__

#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "../Events/Component.h"
#include "../Events/Event.h"
#include "../Events/StandardEvents.h"


//Audio Includes
#	if APIABSTRACTION_X360

#include <xtl.h>
#include <Atg/AtgUtil.h>
#include <Atg/AtgAudio.h>

# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11

#include <windows.h>

# endif

#include "wave.h"
#include "xaudio2.h"
#include "Xaudio2fx.h"
#include "x3daudio.h"
#include "d3dx9math.h"

//defines
#define INPUTCHANNELS 1  // number of source channels
#define OUTPUTCHANNELS 8 // maximum number of destination channels supported in this sample

#define NUM_PRESETS 30


// Sibling/Children includes
namespace PE {
namespace Components {

struct AudioSource
{
	//Variables:
	Handle m_hXACT;
	bool active;
#if APIABSTRACTION_X360
	//WaveFile* buffer;
#elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
	Wave* buffer;
#endif
	// XAudio2
	IXAudio2* pXAudio2;
	IXAudio2MasteringVoice* pMasteringVoice;
	IXAudio2SourceVoice* pSourceVoice;
	IXAudio2SubmixVoice* pSubmixVoice;
	IUnknown* pReverbEffect;
	BYTE* pbSampleData;
	UINT32 m_channel;
	// 3D
	X3DAUDIO_HANDLE x3DInstance;
	int nFrameToApply3DAudio;

	DWORD dwChannelMask;
	UINT32 nChannels;

	X3DAUDIO_DSP_SETTINGS dspSettings;
	X3DAUDIO_LISTENER listener;
	X3DAUDIO_EMITTER emitter;
	X3DAUDIO_CONE emitterCone;

	#if APIABSTRACTION_X360
		XMVECTOR vListenerPos;//this will be the player
		XMVECTOR vListenerForward; //this will be for the player as well
		XMVECTOR vEmitterPos;
	# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
		Vector3 vListenerPos;//this will be the player
		Vector3 vListenerForward; //this will be for the player as well
		Vector3 vEmitterPos;
	# endif

	float fListenerAngle;
	bool  fUseListenerCone;
	bool  fUseInnerRadius;
	bool  fUseRedirectToLFE;

	FLOAT32 emitterAzimuths[INPUTCHANNELS];
	FLOAT32 matrixCoefficients[INPUTCHANNELS * OUTPUTCHANNELS];


	//Functions
	void Start();
	void Stop();
	HRESULT InitAudio();
	HRESULT PrepareAudio(char* file, bool loop);
	HRESULT UpdateAudio(float deltaTime);
	void PauseAudio(bool resume);
	void CleanupAudio();
	D3DXVECTOR3 getDVect(Vector3 inVal)
	{
		return D3DXVECTOR3(inVal.m_x, inVal.m_y, inVal.m_z);
	}



//#endif
};

struct SoundManager : public Component
{
	PE_DECLARE_CLASS(SoundManager);

	SoundManager(PE::GameContext &context, PE::MemoryArena arena, Handle hMyself, const char* wbFilename, bool isActive = true): Component(context, arena, hMyself)
	{
		#if APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
		#endif
	}

	virtual ~SoundManager() {}
	
	// Component ------------------------------------------------------------

	virtual void addDefaultComponents();

	static void Construct(PE::GameContext &context, PE::MemoryArena arena, const char* wbFilename, bool isActive = true)
	{
		Handle h("SOUND_MANAGER", sizeof(SoundManager));
		SoundManager *pSoundManager = new(h) SoundManager(context, arena, h, wbFilename, isActive);
		pSoundManager->addDefaultComponents();
		SetInstance(h);
		s_isActive = isActive;
	}
	
	static void SetInstance(Handle h){s_hInstance = h;}

	static SoundManager *Instance() {return s_hInstance.getObject<SoundManager>();}
	static Handle InstanceHandle() {return s_hInstance;}
	static Handle s_hInstance;
	static bool s_isActive;

	//#if APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
	//Variables:
	Handle m_hXACT;
	Vector3 playerPos;//this will be the player
	Vector3 playerForward; //this will be for the player as well
	AudioSource planeFire; //add this when you want to add in a sound source. For all sounds we can use the player as the listener.
	AudioSource soldierFootSteps;
	AudioSource playerFireGun;

	void Start();
	void UpdateSounds(float deltaSeconds);

	//#endif
};
}; // namespace Components
}; // namespace PE
#endif
