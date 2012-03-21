/* AudioHardwareALSA.cpp
 **
 ** Copyright 2008-2009 Wind River Systems
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#define LOG_TAG "AudioHardwareALSA"
#include <utils/Log.h>
#include <utils/String8.h>

#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include "AudioHardwareALSA.h"

extern "C"
{
    //
    // Function for dlsym() to look up for creating a new AudioHardwareInterface.
    //
    android::AudioHardwareInterface *createAudioHardware(void) {
        return android::AudioHardwareALSA::create();
    }
}         // extern "C"

namespace android
{

// ----------------------------------------------------------------------------

static void ALSAErrorHandler(const char *file,
                             int line,
                             const char *function,
                             int err,
                             const char *fmt,
                             ...)
{
    char buf[BUFSIZ];
    va_list arg;
    int l;

    va_start(arg, fmt);
    l = snprintf(buf, BUFSIZ, "%s:%i:(%s) ", file, line, function);
    vsnprintf(buf + l, BUFSIZ - l, fmt, arg);
    buf[BUFSIZ-1] = '\0';
    /* LGE_CHANGE_S taewook.bae@lge.com 2010.01.27 */
    //LOG(LOG_ERROR, "ALSALib", buf);
    LOG(LOG_ERROR, "ALSALib", "%s", buf);
    /* LGE_CHANGE_E taewook.bae@lge.com 2010.01.27 */
    va_end(arg);
}

AudioHardwareInterface *AudioHardwareALSA::create() {
    return new AudioHardwareALSA();
}

AudioHardwareALSA::AudioHardwareALSA() :
    mALSADevice(0),
//[LG_FW_AUDIO_TTY Start] - jungsoo1221.lee
    mTtyMode(TTY_OFF),
 //[LG_FW_AUDIO_TTY End] 
    mAcousticDevice(0)
{
    snd_lib_error_set_handler(&ALSAErrorHandler);
    mMixer = new ALSAMixer;

    hw_module_t *module;
    int err = hw_get_module(ALSA_HARDWARE_MODULE_ID,
            (hw_module_t const**)&module);

    if (err == 0) {
        hw_device_t* device;
        err = module->methods->open(module, ALSA_HARDWARE_NAME, &device);
        if (err == 0) {
            mALSADevice = (alsa_device_t *)device;
            mALSADevice->init(mALSADevice, mDeviceList);
        } else
            LOGE("ALSA Module could not be opened!!!");
    } else
        LOGE("ALSA Module not found!!!");

    err = hw_get_module(ACOUSTICS_HARDWARE_MODULE_ID,
            (hw_module_t const**)&module);

    if (err == 0) {
        hw_device_t* device;
        err = module->methods->open(module, ACOUSTICS_HARDWARE_NAME, &device);
        if (err == 0)
            mAcousticDevice = (acoustic_device_t *)device;
        else
            LOGE("Acoustics Module not found.");
    }

    mControl = new ALSAControl; /* [2011.02.21]jung.chanmin@lge.com - add the voice volume gain */
    mMutestate = true;		//[LG_FW_P970_MERGE] - jungsoo1221.lee // 20100426 junyeop.kim@lge.com Add the mic mute [START_LGE]
}

AudioHardwareALSA::~AudioHardwareALSA()
{
    if (mMixer) delete mMixer;
    if (mALSADevice)
        mALSADevice->common.close(&mALSADevice->common);
    if (mControl) delete mControl; /* [2011.02.21]jung.chanmin@lge.com - add the voice volume gain */
    if (mAcousticDevice)
        mAcousticDevice->common.close(&mAcousticDevice->common);
}

status_t AudioHardwareALSA::initCheck()
{
    if (mALSADevice && mMixer && mMixer->isValid())
        return NO_ERROR;
    else
        return NO_INIT;
}

status_t AudioHardwareALSA::setVoiceVolume(float volume)
{
    LOGD("AudioHardwareALSA::setVoiceVolume() : %f", volume);
#ifdef AUDIO_MODEM_TI
    if (mMixer)
        return mMixer->setVoiceVolume(volume);
    else
        return INVALID_OPERATION;
#else
#if 0	 /* [2011.02.21]jung.chanmin@lge.com - add the voice volume gain */
    // The voice volume is used by the VOICE_CALL audio stream.
    if (mMixer)
        return mMixer->setVolume(AudioSystem::DEVICE_OUT_EARPIECE, volume, volume);
    else
        return INVALID_OPERATION;
#else
	unsigned int val = 0;
#if 0	//0x44 volume tuning
    if(volume == 1.0f)	
//	    val = 0x0d;     //2dB
        val = 0x09;     //-6dB
	else if(volume == 0.8f)	
//	    val = 0x0b;     //-2dB
        val = 0x07;     //-10dB
	else if(volume == 0.6f)	
//	    val = 0x09;     //-6dB
        val = 0x05;     //-14dB
	else if(volume == 0.4f)	
//	    val = 0x07;     //-10dB
        val = 0x03;     //-18dB
	else if(volume == 0.2f)
//	    val = 0x04;     //-16dB
        val = 0x01;     //-22dB
	else if(volume == 0.0f)
//	    val = 0x02;     //-20dB
        val = 0x00;     //-24dB
	else	//current vol restore
	{
	    val = 0xff;
	    LOGD("[LUCKYJUN77] Invalid volume");
    }
#else	//0x14 volume tuning
#if 1	//6 level volume
    
	if(volume < 0.15)
	{
	  val = 0;	
	}
	else if(volume >= 0.15 && volume < 0.3)
	{
	    val = 1;
	}
	else if(volume >= 0.3 && volume < 0.45)
	{
	  val = 2;	
	}
	else if(volume >= 0.45 && volume < 0.6)
	{
	  val = 3;	
	}
	else if(volume >= 0.6 && volume < 0.75)
	{
	  val = 4;	
	}
	else if(volume >= 0.75 && volume < 0.9)
	{
	  val = 5;	
	}
	else if(volume >= 0.9)
	{
	  val = 6;	
	}
	else	//current vol restore
	{
	    val = 0xff;
	    LOGD("[LUCKYJUN77] Invalid volume");
    }
	
#else	//10 level volume
    if(volume == 1.0f)	//-3dB(10)
	    val = 0x28;
	else if(volume < 1.0f && volume >=  0.8f )	//-6dB(9)
	    val = 0x25;
	else if(volume < 0.8f && volume >=  0.7f )	//-9dB(8)
	    val = 0x22;
	else if(volume < 0.7f && volume >=  0.6f ) 	//-12dB(7)
	    val = 0x1f;
	else if(volume < 0.6f && volume >=  0.5f )	//-15dB(6)
	    val = 0x1c;
	else if(volume < 0.5f && volume >=  0.4f )	//-18dB(5)
	    val = 0x19;
	else if(volume < 0.4f && volume >=  0.3f )	//-21dB(4)
	    val = 0x16;
	else if(volume < 0.3f && volume >=  0.2f )	//-24dB(3)
	    val = 0x13;
	else if(volume < 0.2f && volume >=  0.1f )	//-26dB(2)
	    val = 0x10;
	else if(volume == 0.0f)		//-27dB(1)
	    val = 0x0d;
	else	//current vol restore
	{
	    val = 0xff;
	    LOGD("[LUCKYJUN77] Invalid volume");
    }

#endif

#endif

  mControl->setVoiceVolume(val);

	return NO_ERROR;
#endif	
#endif
}

status_t AudioHardwareALSA::setMasterVolume(float volume)
{
    LOGD("AudioHardwareALSA::setMasterVolume() : %f", volume);
//[LG_FW_P970_MERGE Start] - jungsoo1221.lee
#if 0    // 20100515 junyeop.kim@lge.com set the master volume gain for HW request [START_LGE]
    if (mMixer)
        return mMixer->setMasterVolume(volume);
    else
        return INVALID_OPERATION;
#else
    if (mMixer)
        mMixer->setMasterVolume(volume);
    else
        return INVALID_OPERATION;

    mControl->setMasterVolume(5); // 20100912 junyeop.kim@lge.com set the master volume gain for HW request [START_LGE]
    return NO_ERROR;
#endif	// 20100515 junyeop.kim@lge.com set the master volume gain for HW request [START_LGE]
//[LG_FW_P970_MERGE End]
}


 //[LG_FW_AUDIO_TTY Start]  - jungsoo1221.lee
status_t AudioHardwareALSA::setTtyMode(int mode)
{
	Mutex::Autolock lock(mLock);
	mTtyMode = mode;
	LOGV("setTtyMode set as %d", mode);
	mControl->setTtyMode(mode);
       return NO_ERROR;
}

status_t AudioHardwareALSA::getTtyMode(int* mode)
{
	*mode = mTtyMode;
	LOGV("getTtyMode get as %d", mode);
       return NO_ERROR;
}
 //[LG_FW_AUDIO_TTY End] 

status_t AudioHardwareALSA::setParameters(const String8& keyValuePairs)
{
   AudioParameter param = AudioParameter(keyValuePairs);
   String8 key;
   int i;
   status_t status = NO_ERROR;
 //[LG_FW_AUDIO_TTY]  - jungsoo1221.lee
   String8 value;  

 //[LG_FW_AUDIO_TTY Start] - jungsoo1221.lee
   key = String8("tty_mode");
    if (param.get(key, value) == NO_ERROR) {
        if (value == "full") {
            mTtyMode = TTY_FULL;
        } else if (value == "hco") {
            mTtyMode = TTY_HCO;
        } else if (value == "vco") {
            mTtyMode = TTY_VCO;
        } else {
            mTtyMode = TTY_OFF;
        }
       setTtyMode(mTtyMode);    
	LOGD("[eklee]TTY_MODE_KEY mMode : %d", mTtyMode);
    }
 //[LG_FW_AUDIO_TTY End] 


   if (mALSADevice && mALSADevice->set){
        LOGI("setParameters got %s", keyValuePairs.string());
        return mALSADevice->set(keyValuePairs);
    }
    else {
        LOGE("setParameters INVALID OPERATION");
        return INVALID_OPERATION;
    }
 
}

status_t AudioHardwareALSA::setMode(int mode)
{
    status_t status = NO_ERROR;

    if (mode != mMode) {
        status = AudioHardwareBase::setMode(mode);

        if (status == NO_ERROR) {
            // take care of mode change.
            for(ALSAHandleList::iterator it = mDeviceList.begin();
                it != mDeviceList.end(); ++it) {
                status = mALSADevice->route(&(*it), it->curDev, mode);
                if (status != NO_ERROR)
                    break;
            }
        }
    }

    return status;
}
//2011.03.15 minyoung1.kim@lge.com FM Radio [START]
/*
status_t AudioHardwareALSA::setParameters(const String8& keyValuePairs)
{
    if (mALSADevice && mALSADevice->set){
        LOGI("setParameters got %s", keyValuePairs.string());
        return mALSADevice->set(keyValuePairs);
    }
    else {
        LOGE("setParameters INVALID OPERATION");
        return INVALID_OPERATION;
    }
}
*/
AudioStreamOut *
AudioHardwareALSA::openOutputStream(uint32_t devices,
                                    int *format,
                                    uint32_t *channels,
                                    uint32_t *sampleRate,
                                    status_t *status)
{
    AutoMutex lock(mLock);

    LOGD("openOutputStream called for devices: 0x%08x", devices);

    status_t err = BAD_VALUE;
    AudioStreamOutALSA *out = 0;

    if (devices & (devices - 1)) {
        if (status) *status = err;
        LOGD("openOutputStream called with bad devices");
        return out;
    }

    // Find the appropriate alsa device
    for(ALSAHandleList::iterator it = mDeviceList.begin();
        it != mDeviceList.end(); ++it)
        if (it->devices & devices) {
            err = mALSADevice->open(&(*it), devices, mode());
            if (err) break;
            out = new AudioStreamOutALSA(this, &(*it));
            err = out->set(format, channels, sampleRate);
            break;
        }

    if (status) *status = err;
    return out;
}

void
AudioHardwareALSA::closeOutputStream(AudioStreamOut* out)
{
    AutoMutex lock(mLock);
    delete out;
}

AudioStreamIn *
AudioHardwareALSA::openInputStream(uint32_t devices,
                                   int *format,
                                   uint32_t *channels,
                                   uint32_t *sampleRate,
                                   status_t *status,
                                   AudioSystem::audio_in_acoustics acoustics)
{
    AutoMutex lock(mLock);

    status_t err = BAD_VALUE;
    AudioStreamInALSA *in = 0;

    if (devices & (devices - 1)) {
        if (status) *status = err;
        return in;
    }

    // Find the appropriate alsa device
    for(ALSAHandleList::iterator it = mDeviceList.begin();
        it != mDeviceList.end(); ++it)
        if (it->devices & devices) {
            err = mALSADevice->open(&(*it), devices, mode());
            if (err) break;
            in = new AudioStreamInALSA(this, &(*it), acoustics);
            err = in->set(format, channels, sampleRate);
            break;
        }

    if (status) *status = err;
    return in;
}

// non-default implementation
#if 1 //[LG_FW_P970_MERGE Start] - jungsoo1221.lee
size_t AudioHardwareALSA::getInputBufferSize(uint32_t sampleRate, int format, int channelCount)
{

	LOGW("getInputBufferSize sampling rate: %d, format : %d, channelCount : %d", sampleRate, format, channelCount);

#if 0
    if (!(sampleRate == 8000 ||
        sampleRate == 16000 ||
        sampleRate == 11025)) {
        LOGW("getInputBufferSize bad sampling rate: %d", sampleRate);
        return 0;
    }
#else
    if (!(sampleRate == 8000 ||sampleRate == 16000 ||sampleRate == 11025 ||sampleRate == 22050 || sampleRate == 44100)) {
        LOGW("getInputBufferSize bad sampling rate: %d", sampleRate);
        return 0;
    }
#endif

    if (format != AudioSystem::PCM_16_BIT) {
        LOGW("getInputBufferSize bad format: %d", format);
        return 0;
    }
#if 0    
    if (channelCount != 1) {
#else
    if (!(channelCount == 1 ||channelCount == 2)) {
#endif
        LOGW("getInputBufferSize bad channel count: %d", channelCount);
        return 0;
    }

#if 0
    return 320;
#else
	if(sampleRate == 44100)
		return 4096 * 2;
	else if(sampleRate == 8000 && channelCount == 1)   //minyoung1.kim@lge.com myoung_recording  ............
		return 2048;    //minyoung1.kim@lge.com myoung_recording .............
	else
//	return 4096;
	return 1024;
//	return 2048 * channelCount;
#endif
}
#else
size_t AudioHardwareALSA::getInputBufferSize(uint32_t sampleRate, int format, int channelCount)
{
    if (!(sampleRate == 8000 ||
        sampleRate == 16000 ||
        sampleRate == 11025)) {
        LOGW("getInputBufferSize bad sampling rate: %d", sampleRate);
        return 0;
    }
    if (format != AudioSystem::PCM_16_BIT) {
        LOGW("getInputBufferSize bad format: %d", format);
        return 0;
    }
    if (channelCount != 1) {
        LOGW("getInputBufferSize bad channel count: %d", channelCount);
        return 0;
    }

    return 320;
}
#endif //[LG_FW_P970_MERGE End] 

void
AudioHardwareALSA::closeInputStream(AudioStreamIn* in)
{
    AutoMutex lock(mLock);
    delete in;
}

status_t AudioHardwareALSA::setMicMute(bool state)
{
	//[LG_FW_P970_MERGE Start] - jungsoo1221.lee // 20100426 junyeop.kim@lge.com Add the mic mute [START_LGE]
	LOGW("[LUCKYJUN77]setMicMute....mMutestate : %d, state :%d", mMutestate, state);
	ALSAControl control("hw:00");   //minyoung1.kim@lge.com myoung_recording .................
	if(state == mMutestate)
		return NO_ERROR;

	mMutestate = state;	

	if(state)
		mControl->set("Mic", "OFF");
	else
		mControl->set("Mic", "Restore");

	//[LG_FW_P970_MERGE End] // 20100426 junyeop.kim@lge.com Add the mic mute [END_LGE]
	
    if (mMixer)
        return mMixer->setCaptureMuteState(AudioSystem::DEVICE_OUT_EARPIECE, state);

    return NO_INIT;
}

status_t AudioHardwareALSA::getMicMute(bool *state)
{
#if 0 //[LG_FW_P970_MERGE Start] - jungsoo1221.lee // 20100426 junyeop.kim@lge.com Add the mic mute [START_LGE]
    if (mMixer)
        return mMixer->getCaptureMuteState(AudioSystem::DEVICE_OUT_EARPIECE, state);

    return NO_ERROR;
#else
    if (mMixer)
        mMixer->getCaptureMuteState(AudioSystem::DEVICE_OUT_EARPIECE, state);
	*state = mMutestate;

	return NO_ERROR;

#endif    
//[LG_FW_P970_MERGE End]	// 20100426 junyeop.kim@lge.com Add the mic mute [END_LGE]
}

status_t AudioHardwareALSA::dump(int fd, const Vector<String16>& args)
{
    return NO_ERROR;
}

//#ifdef LG_FW_AUDIO_ENGINEER_MODE
#if 1
int AudioHardwareALSA::setLoopbackMode(int mode)
{
//    ALSAControl control("hw:00");    

//    status = mALSADevice->route(&(*it), it->curDev, mode);
    
    if(mode == 0)
    {
	    LOGW("[if mode = 0] setLoopbackMode::(0)"); 
 		mControl->set("PredriveL Mixer AudioL2", (unsigned int)0); // on
 		mControl->set("PredriveR Mixer AudioR2", (unsigned int)0); // on
 		mControl->set("Earpiece Mixer AudioL2", (unsigned int)0); // on
 		mControl->set("HeadsetR Mixer AudioR2", (unsigned int)0); // on
 		mControl->set("HeadsetL Mixer AudioL2", (unsigned int)0); // on
 		mControl->set("ExtAmp", "OFF");

        mControl->set ("Voice", "OFF");
	}
    else if(mode ==1)
    {
	    LOGW("[if mode = 1] setLoopbackMode::(1)");        

	    mControl->set ("ExtAmp", "Bypass");
	    mControl->set ("Voice", "Receiver");    	

        mControl->set("PredriveL Mixer AudioL2", (unsigned int)0); // on
        mControl->set("PredriveR Mixer AudioR2", (unsigned int)0); // on
        mControl->set("Earpiece Mixer AudioL2", 1); // on
        mControl->set("HeadsetR Mixer AudioR2", (unsigned int)0); // on
        mControl->set("HeadsetL Mixer AudioL2", (unsigned int)0); // on
        mControl->set("ExtAmp", "Bypass");
	}
	else
	    LOGW("setLoopbackMode : Invalid mode::(%d)", mode);

	return NO_ERROR;
}
#endif
//#endif
}       // namespace android
