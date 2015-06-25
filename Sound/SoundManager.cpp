#include "SoundManager.h"
#include "PrimeEngine/Lua/LuaEnvironment.h"

//--------------------------------------------------------------------------------------
// Some macro definitions
//--------------------------------------------------------------------------------------

//#ifndef SAFE_RELEASE
//#define SAFE_RELEASE(x) { if ( (x) != NULL && (x)->Release() == 0 ) { (x) = NULL; } }
//#endif
//
//#ifndef SAFE_DELETE
//#define SAFE_DELETE(x) { if ( (x) != NULL ) { delete (x); (x) = NULL; } }
//#endif
//
//#ifndef SAFE_DELETE_ARRAY
//#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p) = NULL; } }
//#endif
//
//#ifndef ARRAY_SIZE
//#define ARRAY_SIZE(x) ( sizeof(x) / sizeof(x[0] ) )
//#endif
//
//#ifndef RETURN_ON_FAIL
//#define RETURN_ON_FAIL(fn) { HRESULT hr; if ( FAILED( hr = (fn) ) ) return hr;}
//#endif
//
//#ifndef RETURN_ON_NULL
//#define RETURN_ON_NULL(x) { if ( (x) == NULL ) return E_FAIL;}
//#endif

//--------------------------------------------------------------------------------------
// Helper macros
//--------------------------------------------------------------------------------------

#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { ATG::FatalError( "Error %#X in file %s at line %d)\n", hr, __FILE__, (DWORD)__LINE__ ); } }
#endif

//defines
#		if APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
// Specify sound cone to add directionality to listener for artistic effect:
// Emitters behind the listener are defined here to be more attenuated,
// have a lower LPF cutoff frequency,
// yet have a slightly higher reverb send level.
static const X3DAUDIO_CONE Listener_DirectionalCone = { X3DAUDIO_PI*5.0f / 6.0f, X3DAUDIO_PI*11.0f / 6.0f, 1.0f, 0.75f, 0.0f, 0.25f, 0.708f, 1.0f };

// Specify LFE level distance curve such that it rolls off much sooner than
// all non-LFE channels, making use of the subwoofer more dramatic.
static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_LFE_CurvePoints[3] = { 0.0f, 1.0f, 0.25f, 0.0f, 1.0f, 0.0f };
static const X3DAUDIO_DISTANCE_CURVE       Emitter_LFE_Curve = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_LFE_CurvePoints[0], 3 };

// Specify reverb send level distance curve such that reverb send increases
// slightly with distance before rolling off to silence.
// With the direct channels being increasingly attenuated with distance,
// this has the effect of increasing the reverb-to-direct sound ratio,
// reinforcing the perception of distance.
static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_Reverb_CurvePoints[3] = { 0.0f, 0.5f, 0.75f, 1.0f, 1.0f, 0.0f };
static const X3DAUDIO_DISTANCE_CURVE       Emitter_Reverb_Curve = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_Reverb_CurvePoints[0], 3 };

// Must match order of g_PRESET_NAMES
XAUDIO2FX_REVERB_I3DL2_PARAMETERS g_PRESET_PARAMS[NUM_PRESETS] =
{
	XAUDIO2FX_I3DL2_PRESET_FOREST,
	XAUDIO2FX_I3DL2_PRESET_DEFAULT,
	XAUDIO2FX_I3DL2_PRESET_GENERIC,
	XAUDIO2FX_I3DL2_PRESET_PADDEDCELL,
	XAUDIO2FX_I3DL2_PRESET_ROOM,
	XAUDIO2FX_I3DL2_PRESET_BATHROOM,
	XAUDIO2FX_I3DL2_PRESET_LIVINGROOM,
	XAUDIO2FX_I3DL2_PRESET_STONEROOM,
	XAUDIO2FX_I3DL2_PRESET_AUDITORIUM,
	XAUDIO2FX_I3DL2_PRESET_CONCERTHALL,
	XAUDIO2FX_I3DL2_PRESET_CAVE,
	XAUDIO2FX_I3DL2_PRESET_ARENA,
	XAUDIO2FX_I3DL2_PRESET_HANGAR,
	XAUDIO2FX_I3DL2_PRESET_CARPETEDHALLWAY,
	XAUDIO2FX_I3DL2_PRESET_HALLWAY,
	XAUDIO2FX_I3DL2_PRESET_STONECORRIDOR,
	XAUDIO2FX_I3DL2_PRESET_ALLEY,
	XAUDIO2FX_I3DL2_PRESET_CITY,
	XAUDIO2FX_I3DL2_PRESET_MOUNTAINS,
	XAUDIO2FX_I3DL2_PRESET_QUARRY,
	XAUDIO2FX_I3DL2_PRESET_PLAIN,
	XAUDIO2FX_I3DL2_PRESET_PARKINGLOT,
	XAUDIO2FX_I3DL2_PRESET_SEWERPIPE,
	XAUDIO2FX_I3DL2_PRESET_UNDERWATER,
	XAUDIO2FX_I3DL2_PRESET_SMALLROOM,
	XAUDIO2FX_I3DL2_PRESET_MEDIUMROOM,
	XAUDIO2FX_I3DL2_PRESET_LARGEROOM,
	XAUDIO2FX_I3DL2_PRESET_MEDIUMHALL,
	XAUDIO2FX_I3DL2_PRESET_LARGEHALL,
	XAUDIO2FX_I3DL2_PRESET_PLATE,
};
#endif


namespace PE {
namespace Components {
Handle SoundManager::s_hInstance;
bool SoundManager::s_isActive;

PE_IMPLEMENT_CLASS1(SoundManager, Component);

void SoundManager::addDefaultComponents()
{
	Component::addDefaultComponents();

#		if APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
	//for each audio source, make sure to set the channel! I think we can have up to 8 channels/ different sounds
	{
		HRESULT hr = planeFire.InitAudio();
		if (FAILED(hr))
		{
			OutputDebugString("InitAudio() failed.  Disabling audio support\n");
			return;
		}
		char* file = "AssetsOut/Sound/Fire.wav";
		//char* file = "AssetsOut/Sound/GravelWalk.wav";
		hr = planeFire.PrepareAudio(file, true);//yes loop
		if (FAILED(hr))
		{
			OutputDebugString("PrepareAudio() failed\n");
		}
		planeFire.m_channel = 1;
		planeFire.pSourceVoice->SetVolume(0.2f);
	}

	{
		HRESULT hr = playerFireGun.InitAudio();
		//playerFireGun.emitter.CurveDistanceScaler = 20.f;//falloff radius, maybe adjust volume here too?
		if (FAILED(hr))
		{
			OutputDebugString("InitAudio() failed.  Disabling audio support\n");
			return;
		}
	}

	{
		HRESULT hr =  soldierFootSteps.InitAudio();
		soldierFootSteps.emitter.CurveDistanceScaler = 25.f;//falloff radius, maybe adjust volume here too?
		if (FAILED(hr))
		{
			OutputDebugString("InitAudio() failed.  Disabling audio support\n");
			return;
		}
		char* file = "AssetsOut/Sound/GravelWalk.wav";
		hr = soldierFootSteps.PrepareAudio(file, true);//yes loop
		if (FAILED(hr))
		{
			OutputDebugString("PrepareAudio() failed\n");
		}
		//soldierFootSteps.pSourceVoice = planeFire.pSourceVoice;
		//soldierFootSteps.pMasteringVoice = planeFire.pMasteringVoice;
		//soldierFootSteps.pSubmixVoice = planeFire.pSubmixVoice;
		soldierFootSteps.m_channel = 2;
		soldierFootSteps.pSourceVoice->SetVolume(1.0f);
	}



	//planeFire.CleanupAudio();
	//soldierFootSteps.CleanupAudio();
	//soldierFireGun.CleanupAudio();


#		endif

}
#		if APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
void SoundManager::Start()
{
	//start each audioFile
	planeFire.Start();
	soldierFootSteps.Start();
	//playerFireGun.Start();
}

void SoundManager::UpdateSounds(float deltaSeconds)
{
	//for each sound, make sure to update player pos/ forward
	{
#if APIABSTRACTION_X360
		soldierFootSteps.vListenerPos.x = playerPos.m_x;
		soldierFootSteps.vListenerPos.y = playerPos.m_y;
		soldierFootSteps.vListenerPos.z = playerPos.m_z;
		soldierFootSteps.vListenerForward.x = playerForward.m_x;
		soldierFootSteps.vListenerForward.y = playerForward.m_y;
		soldierFootSteps.vListenerForward.z = playerForward.m_z;
		

# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
		soldierFootSteps.vListenerPos = playerPos;
# endif

		soldierFootSteps.UpdateAudio(deltaSeconds);
	}

	{
		
#if APIABSTRACTION_X360
		planeFire.vListenerPos.x = playerPos.m_x;
		planeFire.vListenerPos.y = playerPos.m_y;
		planeFire.vListenerPos.z = playerPos.m_z;
		planeFire.vListenerForward.x = playerForward.m_x;
		planeFire.vListenerForward.y = playerForward.m_y;
		planeFire.vListenerForward.z = playerForward.m_z;
		
# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
		planeFire.vListenerPos = playerPos;
# endif
		planeFire.UpdateAudio(deltaSeconds);
	}

	{

#if APIABSTRACTION_X360
		playerFireGun.vListenerPos.x = playerPos.m_x;
		playerFireGun.vListenerPos.y = playerPos.m_y;
		playerFireGun.vListenerPos.z = playerPos.m_z;
		playerFireGun.vListenerForward.x = playerForward.m_x;
		playerFireGun.vListenerForward.y = playerForward.m_y;
		playerFireGun.vListenerForward.z = playerForward.m_z;
# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
		playerFireGun.vListenerPos = playerPos;
# endif
		playerFireGun.UpdateAudio(deltaSeconds);
	}
}

void AudioSource::Start()
{
	pSourceVoice->Start(0);
	active = true;
}

void AudioSource::Stop()
{
	pSourceVoice->Stop(0);
	//active = false;
}

HRESULT AudioSource::InitAudio()
{

#if	APIABSTRACTION_X360
	// Clear struct
	//ZeroMemory(&(SoundManager::Instance()->planeFire), sizeof(AudioSource));
	//ZeroMemory(&(SoundManager::Instance()->soldierFootSteps), sizeof(AudioSource));
	//ZeroMemory(&(SoundManager::Instance()->playerFireGun), sizeof(AudioSource));

#elif (APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11)
	//
	// Initialize XAudio2
	//
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif

	UINT32 flags = 0;
#ifdef _DEBUG
	flags |= XAUDIO2_DEBUG_ENGINE;
#endif

	HRESULT hr;

	if (FAILED(hr = XAudio2Create(&pXAudio2, flags)))
		return hr;

	//
	// Create a mastering voice
	//
	if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasteringVoice)))
	{
		#if APIABSTRACTION_X360
			SAFE_RELEASE( pXAudio2 );
		# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
			pXAudio2->Release();
		# endif
			
		
		return hr;
	}

	// Check device details to make sure it's within our sample supported parameters
	XAUDIO2_DEVICE_DETAILS details;
	if (FAILED(hr = pXAudio2->GetDeviceDetails(0, &details)))
	{
		#if APIABSTRACTION_X360
			SAFE_RELEASE( pXAudio2 );
		# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
			pXAudio2->Release();
		# endif
			
		return hr;
	}

	if (details.OutputFormat.Format.nChannels > OUTPUTCHANNELS)
	{
		#if APIABSTRACTION_X360
			SAFE_RELEASE( pXAudio2 );
		# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
			pXAudio2->Release();
		# endif
			
		return E_FAIL;
	}

	dwChannelMask = details.OutputFormat.dwChannelMask;
	nChannels = details.OutputFormat.Format.nChannels;

	//
	// Create reverb effect
	//
	flags = 0;
#ifdef _DEBUG
	flags |= XAUDIO2FX_DEBUG;
#endif

	if (FAILED(hr = XAudio2CreateReverb(&pReverbEffect, flags)))
	{
		#if APIABSTRACTION_X360
			SAFE_RELEASE( pXAudio2 );
		# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
			pXAudio2->Release();
		# endif
		

		return hr;
	}

	//
	// Create a submix voice
	//

	// Performance tip: you need not run global FX with the sample number
	// of channels as the final mix.  For example, this sample runs
	// the reverb in mono mode, thus reducing CPU overhead.
	XAUDIO2_EFFECT_DESCRIPTOR effects[] = { {pReverbEffect, TRUE, 1 } };
	XAUDIO2_EFFECT_CHAIN effectChain = { 1, effects };

	if (FAILED(hr =pXAudio2->CreateSubmixVoice(&pSubmixVoice, 1,
		details.OutputFormat.Format.nSamplesPerSec, 0, 0,
		NULL, &effectChain)))
	{
		#if APIABSTRACTION_X360
			SAFE_RELEASE( pXAudio2 );
			SAFE_RELEASE( pReverbEffect );
		# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
			pXAudio2->Release();
			pReverbEffect->Release();
		# endif


		return hr;
	}

	// Set default FX params
	XAUDIO2FX_REVERB_PARAMETERS native;
	ReverbConvertI3DL2ToNative(&g_PRESET_PARAMS[1], &native);//use default param for reverb
	pSubmixVoice->SetEffectParameters(0, &native, sizeof(native));

	//
	// Initialize X3DAudio
	//  Speaker geometry configuration on the final mix, specifies assignment of channels
	//  to speaker positions, defined as per WAVEFORMATEXTENSIBLE.dwChannelMask
	//
	//  SpeedOfSound - speed of sound in user-defined world units/second, used
	//  only for doppler calculations, it must be >= FLT_MIN
	//
	const float SPEEDOFSOUND = X3DAUDIO_SPEED_OF_SOUND;
#if APIABSTRACTION_X360
	X3DAudioInitialize(SPEAKER_XBOX, SPEEDOFSOUND, x3DInstance);

	vListenerPos = XMVectorSet( 0, 0, 0, 0 );
    vEmitterPos = XMVectorSet( 0, 0, 0, 0 );
# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
	X3DAudioInitialize(details.OutputFormat.dwChannelMask, SPEEDOFSOUND, x3DInstance);

	vListenerPos = Vector3(0, 0, 0);
	vListenerForward = Vector3(0, 0, 1);
	vEmitterPos = Vector3(0, 0, 0);
# endif
	fListenerAngle = 0;
	fUseListenerCone = TRUE;
	fUseInnerRadius = TRUE;
	fUseRedirectToLFE = ((details.OutputFormat.dwChannelMask & SPEAKER_LOW_FREQUENCY) != 0);
	//
	// Setup 3D audio structs
	//
#if APIABSTRACTION_X360
	listener.Position.x = vListenerPos.x;
    listener.Position.y = vListenerPos.y;
    listener.Position.z = vListenerPos.z;
    listener.OrientFront.x = 0;
    listener.OrientFront.y = 0;
    listener.OrientFront.z = 1;
    listener.OrientTop.x = 0;
    listener.OrientTop.y = 1;
    listener.OrientTop.z = 0;
# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
	listener.Position = getDVect(vListenerPos);
	listener.OrientFront = getDVect(vListenerForward);
	listener.OrientTop = D3DXVECTOR3(0, 1, 0);
# endif
	listener.pCone = (X3DAUDIO_CONE*)&Listener_DirectionalCone;

	emitter.pCone = &emitterCone;
	emitter.pCone->InnerAngle = X3DAUDIO_2PI;
	// Setting the inner cone angles to X3DAUDIO_2PI and
	// outer cone other than 0 causes
	// the emitter to act like a point emitter using the
	// INNER cone settings only.
	emitter.pCone->OuterAngle = 0.0f;
	// Setting the outer cone angles to zero causes
	// the emitter to act like a point emitter using the
	// OUTER cone settings only.
	emitter.pCone->InnerVolume = 0.0f;
	emitter.pCone->OuterVolume = 1.0f;
	emitter.pCone->InnerLPF = 0.0f;
	emitter.pCone->OuterLPF = 1.0f;
	emitter.pCone->InnerReverb = 0.0f;
	emitter.pCone->OuterReverb = 1.0f;

#if APIABSTRACTION_X360
	emitter.Position.x = vEmitterPos.x;
    emitter.Position.y = vEmitterPos.y;
    emitter.Position.z = vEmitterPos.z;
    emitter.OrientFront.x = 0;
    emitter.OrientFront.y = 0;
    emitter.OrientFront.z = 1;
    emitter.OrientTop.x = 0;
    emitter.OrientTop.y = 1;
    emitter.OrientTop.z = 0;
# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11

	emitter.Position = getDVect(vEmitterPos);
	emitter.OrientFront = D3DXVECTOR3(0, 0, 1);
	emitter.OrientTop = D3DXVECTOR3(0, 1, 0);
# endif
	emitter.ChannelCount = INPUTCHANNELS;
	emitter.ChannelRadius = 1.0f;
	emitter.pChannelAzimuths = emitterAzimuths;

	// Use of Inner radius allows for smoother transitions as
	// a sound travels directly through, above, or below the listener.
	// It also may be used to give elevation cues.
	emitter.InnerRadius = 2.0f;
	emitter.InnerRadiusAngle = X3DAUDIO_PI / 4.0f;;

	emitter.pVolumeCurve = (X3DAUDIO_DISTANCE_CURVE*)&X3DAudioDefault_LinearCurve;
	emitter.pLFECurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_LFE_Curve;
	emitter.pLPFDirectCurve = NULL; // use default curve
	emitter.pLPFReverbCurve = NULL; // use default curve
	emitter.pReverbCurve = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_Reverb_Curve;
	emitter.CurveDistanceScaler = 35.0f;
	emitter.DopplerScaler = 1.5f;

	dspSettings.SrcChannelCount = INPUTCHANNELS;
	dspSettings.DstChannelCount = nChannels;
	dspSettings.pMatrixCoefficients = matrixCoefficients;

	//
	// Done
	//
	
	//active = true;

	return S_OK;
}
HRESULT AudioSource::PrepareAudio(char* file, bool loop)
{

	//if (!active)
	//{
	//	return E_FAIL;
	//}

	if (pSourceVoice)
	{
		pSourceVoice->Stop(0);
		pSourceVoice->DestroyVoice();
		pSourceVoice = 0;
	}

	//
	// Search for media
	//

	HRESULT hr;

	//
	// Read in the wave file
	//
# if APIABSTRACTION_X360
	ATG::WaveFile wav;
    V_RETURN( wav.Open( file ) );

	WAVEFORMATEXTENSIBLE wfx;

    V_RETURN( wav.GetFormat( &wfx ) );

	if( wfx.Format.nChannels > 1 )
        ATG::FatalError( "The sample only currently supports MONO sounds\n" );

    WAVEFORMATEX* pwfx = ( WAVEFORMATEX* )&wfx;

	// Calculate how many bytes and samples are in the wave
    DWORD cbWaveSize;
    wav.GetDuration( &cbWaveSize );

    // Read the sample data into memory
    SAFE_DELETE_ARRAY( pbSampleData );

    pbSampleData = new BYTE[ cbWaveSize ];

    V_RETURN( wav.ReadSample( 0, pbSampleData, cbWaveSize, &cbWaveSize ) );

	//
    // Play the wave using a source voice that sends to both the submix and mastering voices
    //
    XAUDIO2_SEND_DESCRIPTOR sendDescriptors[2];
    sendDescriptors[0].Flags = XAUDIO2_SEND_USEFILTER; // LPF direct-path
    sendDescriptors[0].pOutputVoice = pMasteringVoice;
    sendDescriptors[1].Flags = XAUDIO2_SEND_USEFILTER; // LPF reverb-path -- omit for better performance at the cost of less realistic occlusion
    sendDescriptors[1].pOutputVoice = pSubmixVoice;
    const XAUDIO2_VOICE_SENDS sendList = { 2, sendDescriptors };

    // Create the source voice
    V_RETURN( pXAudio2->CreateSourceVoice( &pSourceVoice, pwfx, 0,
                                                        2.0f, NULL, &sendList ) );

    // Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER buffer = {0};
    buffer.pAudioData = pbSampleData;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes = cbWaveSize;
    buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

    V_RETURN( pSourceVoice->SubmitSourceBuffer( &buffer ) );

    //V_RETURN( pSourceVoice->Start( 0 ) );

# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11

	buffer = new Wave();
	if (!buffer->load(file))
	{
		pXAudio2->Release();
		CoUninitialize();
		return -2;
	}

	if (loop)
	{
		buffer->m_xa.LoopCount = XAUDIO2_LOOP_INFINITE;
	}
	else
	{
		buffer->m_xa.LoopCount = 0;
	}

	//
	// Play the wave using a source voice that sends to both the submix and mastering voices
	//
	XAUDIO2_SEND_DESCRIPTOR sendDescriptors[2];
	sendDescriptors[0].Flags = XAUDIO2_SEND_USEFILTER; // LPF direct-path
	sendDescriptors[0].pOutputVoice = pMasteringVoice;
	sendDescriptors[1].Flags = XAUDIO2_SEND_USEFILTER; // LPF reverb-path -- omit for better performance at the cost of less realistic occlusion
	sendDescriptors[1].pOutputVoice = pSubmixVoice;
	const XAUDIO2_VOICE_SENDS sendList = { 2, sendDescriptors };
	HRESULT hr2;
	// create the source voice
	if (FAILED(pXAudio2->CreateSourceVoice(&pSourceVoice, buffer->wf(), 0,
		2.0f, NULL, &sendList)))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		pXAudio2->Release();
		CoUninitialize();
		return -3;
	}

	// Submit the wave sample data using an XAUDIO2_BUFFER structure
	//pSourceVoice->SubmitSourceBuffer(buffer.xaBuffer());
	//play the sound
	pSourceVoice->SubmitSourceBuffer(buffer->xaBuffer());

#endif
	nFrameToApply3DAudio = 0;//*/

	return S_OK;
}

HRESULT AudioSource::UpdateAudio(float deltaTime)
{
	if (!active)
		return S_FALSE;

	if (nFrameToApply3DAudio == 0)
	{
# if APIABSTRACTION_X360
		 // Calculate listener orientation in x-z plane
        if( vListenerPos.x != listener.Position.x
            || vListenerPos.z != listener.Position.z )
        {
            XMVECTOR vDelta = vListenerPos
                - XMVectorSet( listener.Position.x,
                               listener.Position.y,
                               listener.Position.z, 0 );

            fListenerAngle = float( atan2( vDelta.x, vDelta.z ) );

            vDelta.y = 0.0f;
            vDelta = XMVector3Normalize( vDelta );

            listener.OrientFront.x = vDelta.x;
            listener.OrientFront.y = 0.f;
            listener.OrientFront.z = vDelta.z;
        }
# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11
		// Calculate listener orientation in x-z plane
		if (getDVect(vListenerPos).x != listener.Position.x
			|| getDVect(vListenerPos).z != listener.Position.z)
		{
			D3DXVECTOR3 vDelta = getDVect(vListenerPos) - listener.Position;

			fListenerAngle = float(atan2(vDelta.x, vDelta.z));

			vDelta.y = 0.0f;
			D3DXVec3Normalize(&vDelta, &vDelta);
		}

# endif

		if (fUseListenerCone)
		{
			listener.pCone = (X3DAUDIO_CONE*)&Listener_DirectionalCone;
		}
		else
		{
			listener.pCone = NULL;
		}
		if (fUseInnerRadius)
		{
			emitter.InnerRadius = 2.0f;
			emitter.InnerRadiusAngle = X3DAUDIO_PI / 4.0f;
		}
		else
		{
			emitter.InnerRadius = 0.0f;
			emitter.InnerRadiusAngle = 0.0f;
		}

		if (deltaTime > 0)
		{
#if APIABSTRACTION_X360
			XMVECTOR lVelocity = ( vListenerPos
                                   - XMVectorSet( listener.Position.x,
                                                  listener.Position.y,
                                                  listener.Position.z, 0 ) ) / deltaTime;
            listener.Position.x = vListenerPos.x;
            listener.Position.y = vListenerPos.y;
            listener.Position.z = vListenerPos.z;
            listener.Velocity.x = lVelocity.x;
            listener.Velocity.y = lVelocity.y;
            listener.Velocity.z = lVelocity.z;

            XMVECTOR eVelocity = ( vEmitterPos
                                   - XMVectorSet( emitter.Position.x,
                                                  emitter.Position.y,
                                                  emitter.Position.z, 0 ) ) / deltaTime;
            emitter.Position.x = vEmitterPos.x;
            emitter.Position.y = vEmitterPos.y;
            emitter.Position.z = vEmitterPos.z;
            emitter.Velocity.x = eVelocity.x;
            emitter.Velocity.y = eVelocity.y;
            emitter.Velocity.z = eVelocity.z;

# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11

			D3DXVECTOR3 lVelocity = (getDVect(vListenerPos) - listener.Position) / deltaTime;
			listener.Position = getDVect(vListenerPos);
			listener.OrientFront = getDVect(vListenerForward);
			listener.Velocity = lVelocity;

			D3DXVECTOR3 eVelocity = (getDVect(vEmitterPos) - emitter.Position) / deltaTime;
			emitter.Position = getDVect(vEmitterPos);
			emitter.Velocity = eVelocity;
# endif
		}

		DWORD dwCalcFlags = X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER
			| X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_LPF_REVERB
			| X3DAUDIO_CALCULATE_REVERB;
		if (fUseRedirectToLFE)
		{
			// On devices with an LFE channel, allow the mono source data
			// to be routed to the LFE destination channel.
			dwCalcFlags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;
		}

		X3DAudioCalculate(x3DInstance, &listener, &emitter, dwCalcFlags,
			&dspSettings);

		IXAudio2SourceVoice* voice = pSourceVoice;
		if (voice)
		{
			// Apply X3DAudio generated DSP settings to XAudio2
			voice->SetFrequencyRatio(dspSettings.DopplerFactor);
			//if (m_channel == 1 || m_channel == 2)
			{
				voice->SetOutputMatrix(pMasteringVoice, m_channel, nChannels,
					matrixCoefficients);//*/

				voice->SetOutputMatrix(pSubmixVoice, m_channel, 1, &dspSettings.ReverbLevel);
			}

			XAUDIO2_FILTER_PARAMETERS FilterParametersDirect = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * dspSettings.LPFDirectCoefficient), 1.0f }; // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
			voice->SetOutputFilterParameters(pMasteringVoice, &FilterParametersDirect);
			XAUDIO2_FILTER_PARAMETERS FilterParametersReverb = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * dspSettings.LPFReverbCoefficient), 1.0f }; // see XAudio2CutoffFrequencyToRadians() in XAudio2.h for more information on the formula used here
			voice->SetOutputFilterParameters(pSubmixVoice, &FilterParametersReverb);
		}
	}

	nFrameToApply3DAudio++;
	nFrameToApply3DAudio &= 1;

	return S_OK;
}
void AudioSource::PauseAudio(bool resume)
{
	if (!active)
		return;

	if (resume)
		pXAudio2->StartEngine();
	else
		pXAudio2->StopEngine();
}
void AudioSource::CleanupAudio()
{
	if (!active)
		return;

	if (pSourceVoice)
	{
		pSourceVoice->DestroyVoice();
		pSourceVoice = NULL;
	}

	if (pSubmixVoice)
	{
		pSubmixVoice->DestroyVoice();
		pSubmixVoice = NULL;
	}

	if (pMasteringVoice)
	{
		pMasteringVoice->DestroyVoice();
		pMasteringVoice = NULL;
	}


#if APIABSTRACTION_X360
	pXAudio2->StopEngine();
    SAFE_RELEASE( pXAudio2 );
    SAFE_RELEASE( pReverbEffect );

    SAFE_DELETE_ARRAY( pbSampleData );

# elif APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11

	pXAudio2->StopEngine();
	pXAudio2->Release();
	pReverbEffect->Release();

	delete(pbSampleData);

	CoUninitialize();
# endif
	active = false;
}
#		endif

}; // namespace Components
}; // namespace PE