/* AudioStreamOutALSA.cpp
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
#include <sys/time.h>

#define LOG_TAG "AudioHardwareALSA"
#include <utils/Log.h>
#include <utils/String8.h>

#include <cutils/properties.h>
#include <media/AudioRecord.h>
#include <hardware_legacy/power.h>

#include "AudioHardwareALSA.h"

#ifndef ALSA_DEFAULT_SAMPLE_RATE
#define ALSA_DEFAULT_SAMPLE_RATE 44100 // in Hz
#endif

namespace android
{

// ----------------------------------------------------------------------------

static const int DEFAULT_SAMPLE_RATE = ALSA_DEFAULT_SAMPLE_RATE;

// ----------------------------------------------------------------------------

AudioStreamOutALSA::AudioStreamOutALSA(AudioHardwareALSA *parent, alsa_handle_t *handle) :
    ALSAStreamOps(parent, handle),
    mFrameCount(0)
{
}

AudioStreamOutALSA::~AudioStreamOutALSA()
{
    close();
}

uint32_t AudioStreamOutALSA::channels() const
{
    int c = ALSAStreamOps::channels();
    return c;
}

status_t AudioStreamOutALSA::setVolume(float left, float right)
{
    return mixer()->setVolume (mHandle->curDev, left, right);
}

ssize_t AudioStreamOutALSA::write(const void *buffer, size_t bytes)
{
    AutoMutex lock(mLock);

    if (!mPowerLock) {
        acquire_wake_lock (PARTIAL_WAKE_LOCK, "AudioOutLock");
        mPowerLock = true;
		mHandle->module->popAttenu(0);        // 20100604 junyeop.kim@lge.com, No pop noise set(amp on delay) [START_LGE]
		mHandle->module->route(mHandle, mHandle->curDev, mHandle->curMode); //jongik2.kim@lge.com 20100308 codec_onoff
		usleep(35000);  //201104011 junday.lee@lge.com, fix sound tick noise from global B
    }

	/* check if handle is still valid, otherwise we are coming out of standby */
	if(mHandle->handle == NULL) {
         nsecs_t previously = systemTime();
	     mHandle->module->open(mHandle, mHandle->curDev, mHandle->curMode);
         nsecs_t delta = systemTime() - previously;
         LOGE("1.RE-OPEN AFTER STANDBY:: took %llu msecs\n", ns2ms(delta));
	}

    acoustic_device_t *aDev = acoustics();

    // For output, we will pass the data on to the acoustics module, but the actual
    // data is expected to be sent to the audio device directly as well.
    if (aDev && aDev->write)
        aDev->write(aDev, buffer, bytes);

#if 0
    snd_pcm_sframes_t n;
#else
//minyoung1.kim@lge.com - BT connect -> music play -> voice dialer start/end 시 music restart 되는 현상 수정.
    snd_pcm_sframes_t n = 0;  
#endif
    size_t            sent = 0;
    status_t          err;

    do {
//minyoung1.kim@lge.com - BT connect -> music play -> voice dialer start/end 시 music restart 되는 현상 수정. [START]
		if(mHandle->handle != NULL)   
		{                 
//minyoung1.kim@lge.com - BT connect -> music play -> voice dialer start/end 시 music restart 되는 현상 수정. [END]		
        n = snd_pcm_writei(mHandle->handle,
                           (char *)buffer + sent,
                           snd_pcm_bytes_to_frames(mHandle->handle, bytes - sent));
//minyoung1.kim@lge.com - BT connect -> music play -> voice dialer start/end 시 music restart 되는 현상 수정. [START]
		}
		else{
          	nsecs_t previously = systemTime();
          	mHandle->module->open(mHandle, mHandle->curDev, mHandle->curMode);
          	nsecs_t delta = systemTime() - previously;
          	LOGD("2.RE-OPEN AFTER STANDBY:: took %llu msecs\n", ns2ms(delta));
        }
//minyoung1.kim@lge.com - BT connect -> music play -> voice dialer start/end 시 music restart 되는 현상 수정. [END] 	
        if (n == -EBADFD) {
            // Somehow the stream is in a bad state. The driver probably
            // has a bug and snd_pcm_recover() doesn't seem to handle this.
            mHandle->module->open(mHandle, mHandle->curDev, mHandle->curMode);

            if (aDev && aDev->recover) aDev->recover(aDev, n);
            //bail
            if (n) return static_cast<ssize_t>(n);
        }
        else if (n < 0) {
            if (mHandle->handle) {

                // snd_pcm_recover() will return 0 if successful in recovering from
                // an error, or -errno if the error was unrecoverable.
                n = snd_pcm_recover(mHandle->handle, n, 1);

                if (aDev && aDev->recover) aDev->recover(aDev, n);

                if (n) return static_cast<ssize_t>(n);
            }
        }
        else {
			if(mHandle->handle){	//minyoung1.kim@lge.com - BT connect -> music play -> voice dialer start/end 시 music restart 되는 현상 수정.		
            	mFrameCount += n;
            	sent += static_cast<ssize_t>(snd_pcm_frames_to_bytes(mHandle->handle, n));
//minyoung1.kim@lge.com - BT connect -> music play -> voice dialer start/end 시 music restart 되는 현상 수정. [START]
				}   
			else{
          		nsecs_t previously = systemTime();
         		mHandle->module->open(mHandle, mHandle->curDev, mHandle->curMode);
          		nsecs_t delta = systemTime() - previously;
         	    LOGD("3.RE-OPEN AFTER STANDBY:: took %llu msecs\n", ns2ms(delta));
			}
//minyoung1.kim@lge.com - BT connect -> music play -> voice dialer start/end 시 music restart 되는 현상 수정. [END]
        }

    } while (mHandle->handle && sent < bytes);

    return sent;
}

status_t AudioStreamOutALSA::dump(int fd, const Vector<String16>& args)
{
    return NO_ERROR;
}

status_t AudioStreamOutALSA::open(int mode)
{
    AutoMutex lock(mLock);
    return ALSAStreamOps::open(mode);
}

status_t AudioStreamOutALSA::close()
{
    AutoMutex lock(mLock);

    snd_pcm_drain (mHandle->handle);
    ALSAStreamOps::close();

    if (mPowerLock) {
        release_wake_lock ("AudioOutLock");
        mPowerLock = false;
    }

    return NO_ERROR;
}

status_t AudioStreamOutALSA::standby()
{
    ALSAControl control("hw:00"); /* jung.chanmin@lge.com - change wm9093 amp off */
    AutoMutex lock(mLock);
    LOGW("CALLING STANDBY....mHandle->curMode=%d mHandle->module->standby=%d\n",mHandle->curMode,mHandle->module->standby);	
    if (mHandle->module->standby)
    {
		LOGD("Veena In AudioStreamOutAlsa Standby1\n");
		if(mHandle->curMode != 2)
        {
		control.set("ExtAmp","OFF"); /* jung.chanmin@lge.com - change wm9093 amp off */
		mHandle->module->popAttenu(0); 
		mHandle->module->route(mHandle, -1, mHandle->curMode); //jongik2.kim@lge.com 20100308 codec_onoff	        
		mHandle->module->standby(mHandle);
        }
    }
    else
	{
		LOGD("Veena In AudioStreamOutAlsa Standby2\n");
	    mHandle->module->close(mHandle);
    }
	
    if (mPowerLock) {
        release_wake_lock ("AudioOutLock");
        mPowerLock = false;
    }

    mFrameCount = 0;

    return NO_ERROR;
}

#define USEC_TO_MSEC(x) ((x + 999) / 1000)

uint32_t AudioStreamOutALSA::latency() const
{
    // Android wants latency in milliseconds.
//    return USEC_TO_MSEC (mHandle->latency);

    /* ugly hack, add to the teams technical debt */
    return 20;  //[LGE_UPDATE] 2011.05.16 minyoung1.kim@lge.com, fix sound tick noise from global B
}

// return the number of audio frames written by the audio dsp to DAC since
// the output has exited standby
status_t AudioStreamOutALSA::getRenderPosition(uint32_t *dspFrames)
{
    //*dspFrames = mFrameCount;
    *dspFrames = 0;

    return NO_ERROR;
}
// 20100604 junyeop.kim@lge.com, No pop noise set(amp on delay) [START_LGE]
status_t AudioStreamOutALSA::popNoise_attenu(int enable)
{
	mHandle->module->popAttenu(enable);

    return NO_ERROR;
}
// 20100604 junyeop.kim@lge.com, No pop noise set(amp on delay) [END_LGE]        
}       // namespace android
