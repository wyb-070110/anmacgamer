// SPDX-License-Identifier: GPL-3.0-or-later
// Keeps the phone quiet while the computer plays its sound. A normal app's playback capture only copies the sound
// (Android keeps playing it on the phone); the capture is taken before the media volume, so the companion turns the
// media volume to 0 while mirroring and puts it back afterwards. The volume it found is kept in preferences, so a
// companion that was stopped while mirroring puts it back the next time it starts.
package com.gamescrcpy.companion

import android.content.Context
import android.media.AudioManager

object PhoneSpeaker {
    private const val FILE = "phone_speaker"
    private const val KEY = "media_volume_before"

    @Synchronized
    fun silence(context: Context) {
        val audio = context.getSystemService(AudioManager::class.java) ?: return
        val preferences = context.applicationContext.getSharedPreferences(FILE, Context.MODE_PRIVATE)
        if (!preferences.contains(KEY)) {
            val before = runCatching { audio.getStreamVolume(AudioManager.STREAM_MUSIC) }.getOrNull() ?: return
            preferences.edit().putInt(KEY, before).commit()
        }
        // Throws while Do Not Disturb holds the volume; the phone then keeps playing.
        runCatching { audio.setStreamVolume(AudioManager.STREAM_MUSIC, 0, 0) }
    }

    @Synchronized
    fun restore(context: Context) {
        val preferences = context.applicationContext.getSharedPreferences(FILE, Context.MODE_PRIVATE)
        if (!preferences.contains(KEY)) {
            return
        }
        val before = preferences.getInt(KEY, 0)
        val audio = context.getSystemService(AudioManager::class.java)
        if (audio != null) {
            // The user turned it up on the phone meanwhile: theirs stays.
            val now = runCatching { audio.getStreamVolume(AudioManager.STREAM_MUSIC) }.getOrDefault(0)
            if (now == 0) {
                runCatching { audio.setStreamVolume(AudioManager.STREAM_MUSIC, before, 0) }
            }
        }
        preferences.edit().remove(KEY).commit()
    }
}
