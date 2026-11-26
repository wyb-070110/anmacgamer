package com.gamescrcpy.companion

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.media.projection.MediaProjectionConfig
import android.media.projection.MediaProjectionManager
import android.os.Build
import android.os.Bundle

class CaptureGrant(val resultCode: Int, val data: Intent)

object CaptureGrants {
    @Volatile
    private var listener: ((CaptureGrant?) -> Unit)? = null

    fun listen(callback: ((CaptureGrant?) -> Unit)?) {
        listener = callback
    }

    fun deliver(grant: CaptureGrant?) {
        listener?.invoke(grant)
    }
}

class CaptureActivity : Activity() {
    private var answered = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (savedInstanceState != null) {
            return
        }
        if (intent.getBooleanExtra(EXTRA_AUDIO, false) && Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q &&
            checkSelfPermission(Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED
        ) {
            requestPermissions(arrayOf(Manifest.permission.RECORD_AUDIO), REQUEST_AUDIO)
            return
        }
        askCapture()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_AUDIO) {
            askCapture()
        }
    }

    private fun askCapture() {
        val manager = getSystemService(MediaProjectionManager::class.java)
        if (manager == null) {
            answer(null)
            return
        }
        try {
            // The computer mirrors the whole phone: from Android 14 on, ask for the whole screen only, so the dialog
            // has no "single app" choice and one tap on Start is enough.
            val intent = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
                manager.createScreenCaptureIntent(MediaProjectionConfig.createConfigForDefaultDisplay())
            } else {
                manager.createScreenCaptureIntent()
            }
            startActivityForResult(intent, REQUEST_CAPTURE)
        } catch (e: RuntimeException) {
            answer(null)
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != REQUEST_CAPTURE) {
            return
        }
        answer(if (resultCode == RESULT_OK && data != null) CaptureGrant(resultCode, data) else null)
    }

    override fun onDestroy() {
        if (isFinishing) {
            answer(null)
        }
        super.onDestroy()
    }

    private fun answer(grant: CaptureGrant?) {
        if (answered) {
            return
        }
        answered = true
        CaptureGrants.deliver(grant)
        finish()
    }

    companion object {
        const val EXTRA_AUDIO = "audio"
        private const val REQUEST_CAPTURE = 1
        private const val REQUEST_AUDIO = 2
    }
}
