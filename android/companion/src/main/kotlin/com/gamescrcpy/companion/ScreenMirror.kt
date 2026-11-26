// SPDX-License-Identifier: GPL-3.0-or-later
// The picture of a companion mirroring session: screen capture drawn into an H.264 encoder's input surface. One
// virtual display per screen-capture grant (Android 14 allows no second one); a new encoding session resizes it and
// gives it the new encoder's surface.
package com.gamescrcpy.companion

import android.content.Context
import android.hardware.display.DisplayManager
import android.hardware.display.VirtualDisplay
import android.media.MediaCodec
import android.media.MediaCodecInfo
import android.media.MediaFormat
import android.media.projection.MediaProjection
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.util.DisplayMetrics
import android.view.Display
import android.view.Surface
import com.gamescrcpy.protocol.Enums.VideoCodec
import com.gamescrcpy.protocol.Media
import com.gamescrcpy.protocol.MediaChannel
import com.gamescrcpy.protocol.Messages
import java.io.IOException
import java.util.concurrent.TimeUnit

class ScreenMirror(context: Context, private val projection: MediaProjection, private val sink: () -> MirrorSink?) {

    private data class Shape(val width: Int, val height: Int, val rotation: Int, val dpi: Int)

    private val displays: DisplayManager? = context.getSystemService(DisplayManager::class.java)
    private val thread = HandlerThread("companion-mirror-display").apply { start() }
    private val handler = Handler(thread.looper)
    private var display: VirtualDisplay? = null
    @Volatile
    private var codec: MediaCodec? = null
    private var surface: Surface? = null
    private var drain: Thread? = null
    private var session = 0
    private var maxSize = 0
    private var maxFps = 0
    private var bitrate = DEFAULT_BITRATE
    private var shape: Shape? = null

    @Volatile
    private var draining = false

    @Volatile
    var running = false
        private set

    private val watcher = object : DisplayManager.DisplayListener {
        override fun onDisplayAdded(displayId: Int) = Unit

        override fun onDisplayRemoved(displayId: Int) = Unit

        override fun onDisplayChanged(displayId: Int) {
            if (displayId == Display.DEFAULT_DISPLAY) {
                reshape()
            }
        }
    }

    init {
        displays?.registerDisplayListener(watcher, handler)
    }

    @Synchronized
    fun start(maxSize: Int, maxFps: Int, bitrate: Int) {
        this.maxSize = maxSize
        this.maxFps = maxFps
        this.bitrate = if (bitrate > 0) bitrate else DEFAULT_BITRATE
        rebuild()
    }

    @Synchronized
    fun setBitrate(value: Int) {
        if (value <= 0) {
            return
        }
        bitrate = value
        val params = Bundle()
        params.putInt(MediaCodec.PARAMETER_KEY_VIDEO_BITRATE, value)
        runCatching { codec?.setParameters(params) }
    }

    fun requestKeyFrame() {
        val params = Bundle()
        params.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0)
        runCatching { codec?.setParameters(params) }
    }

    @Synchronized
    fun stop() {
        running = false
        stopCodec()
        display?.surface = null
    }

    fun release() {
        displays?.unregisterDisplayListener(watcher)
        synchronized(this) {
            stop()
            display?.release()
            display = null
        }
        thread.quitSafely()
    }

    private fun reshape() {
        synchronized(this) {
            if (running && current() != shape) {
                rebuild()
            }
        }
    }

    @Suppress("DEPRECATION")
    private fun current(): Shape? {
        val shown = displays?.getDisplay(Display.DEFAULT_DISPLAY) ?: return null
        val metrics = DisplayMetrics()
        shown.getRealMetrics(metrics)
        return Shape(metrics.widthPixels, metrics.heightPixels, shown.rotation and 3, metrics.densityDpi)
    }

    private fun rebuild() {
        stopCodec()
        val now = current()
        if (now == null || now.width <= 0 || now.height <= 0) {
            running = false
            return
        }
        shape = now
        val made = try {
            MediaCodec.createEncoderByType(MIME)
        } catch (e: IOException) {
            running = false
            return
        } catch (e: RuntimeException) {
            running = false
            return
        }
        val size = fit(made, now.width, now.height)
        val format = MediaFormat.createVideoFormat(MIME, size.first, size.second)
        format.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface)
        format.setInteger(MediaFormat.KEY_BIT_RATE, bitrate)
        format.setInteger(MediaFormat.KEY_FRAME_RATE, if (maxFps > 0) maxFps else DEFAULT_FPS)
        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, KEY_FRAME_SECONDS)
        format.setLong(MediaFormat.KEY_REPEAT_PREVIOUS_FRAME_AFTER, REPEAT_US)
        format.setInteger(MediaFormat.KEY_PRIORITY, 0)
        if (maxFps > 0) {
            format.setFloat(KEY_MAX_FPS_TO_ENCODER, maxFps.toFloat())
        }
        val input = try {
            made.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
            made.createInputSurface().also { made.start() }
        } catch (e: RuntimeException) {
            made.release()
            running = false
            return
        }
        codec = made
        surface = input
        val shown = display
        try {
            if (shown == null) {
                display = projection.createVirtualDisplay(
                    DISPLAY_NAME,
                    size.first,
                    size.second,
                    now.dpi,
                    DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
                    input,
                    null,
                    handler,
                )
            } else {
                shown.resize(size.first, size.second, now.dpi)
                shown.surface = input
            }
        } catch (e: RuntimeException) {
            stopCodec()
            running = false
            return
        }
        session = if (session >= MAX_SESSION) 1 else session + 1
        val info = Messages.VideoSessionInfo()
        info.codec = VideoCodec.H264
        info.frameWidth = size.first
        info.frameHeight = size.second
        info.displayId = 0
        info.displayWidth = now.width.coerceAtMost(MAX_U16)
        info.displayHeight = now.height.coerceAtMost(MAX_U16)
        info.rotation = now.rotation
        sink()?.videoPacket(Media.encodeVideoSession(session, info), key = false, frame = false)
        running = true
        draining = true
        val number = session
        drain = Thread({ pump(made, number) }, "companion-mirror-video").apply {
            isDaemon = true
            start()
        }
    }

    private fun pump(encoder: MediaCodec, number: Int) {
        val info = MediaCodec.BufferInfo()
        try {
            while (draining) {
                val index = encoder.dequeueOutputBuffer(info, DEQUEUE_US)
                if (index < 0) {
                    continue
                }
                val buffer = encoder.getOutputBuffer(index)
                if (buffer != null && info.size > 0) {
                    val bytes = ByteArray(info.size)
                    buffer.position(info.offset)
                    buffer.get(bytes, 0, info.size)
                    if (info.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG != 0) {
                        sink()?.videoPacket(Media.encodeConfig(MediaChannel.VIDEO, number, bytes), false, false)
                    } else {
                        val key = info.flags and MediaCodec.BUFFER_FLAG_KEY_FRAME != 0
                        val nowUs = TimeUnit.NANOSECONDS.toMicros(System.nanoTime())
                        val pts = info.presentationTimeUs.coerceAtLeast(0)
                        val delay = (nowUs - pts).coerceIn(0, MAX_U32)
                        val packet = Media.encodeFrame(
                            MediaChannel.VIDEO,
                            number,
                            if (key) Media.FLAG_KEY_FRAME else 0,
                            pts,
                            delay,
                            bytes,
                        )
                        sink()?.videoPacket(packet, key, true)
                    }
                }
                encoder.releaseOutputBuffer(index, false)
            }
        } catch (e: IllegalStateException) {
            Unit
        }
    }

    private fun stopCodec() {
        draining = false
        drain?.let { pumping ->
            if (pumping !== Thread.currentThread()) {
                runCatching { pumping.join(JOIN_MILLIS) }
            }
        }
        drain = null
        codec?.let { encoder ->
            runCatching { encoder.stop() }
            encoder.release()
        }
        codec = null
        surface?.release()
        surface = null
    }

    private fun fit(encoder: MediaCodec, width: Int, height: Int): Pair<Int, Int> {
        val longest = maxOf(width, height)
        var scale = if (maxSize in 1 until longest) maxSize.toDouble() / longest else 1.0
        val video = runCatching { encoder.codecInfo.getCapabilitiesForType(MIME).videoCapabilities }.getOrNull()
        while (true) {
            val w = even8((width * scale).toInt())
            val h = even8((height * scale).toInt())
            if (video == null || video.isSizeSupported(w, h) || w <= MIN_SIDE || h <= MIN_SIDE) {
                return w to h
            }
            scale *= SHRINK
        }
    }

    private fun even8(value: Int): Int = (value / 8 * 8).coerceAtLeast(8)

    companion object {
        const val MIME = MediaFormat.MIMETYPE_VIDEO_AVC
        private const val KEY_MAX_FPS_TO_ENCODER = "max-fps-to-encoder"
        private const val DISPLAY_NAME = "GameScrcpy mirror"
        private const val DEFAULT_BITRATE = 8_000_000
        private const val DEFAULT_FPS = 60
        private const val KEY_FRAME_SECONDS = 10
        private const val REPEAT_US = 100_000L
        private const val DEQUEUE_US = 100_000L
        private const val JOIN_MILLIS = 1_000L
        private const val MAX_SESSION = 0xFFFF
        private const val MAX_U16 = 0xFFFF
        private const val MAX_U32 = 0xFFFF_FFFFL
        private const val MIN_SIDE = 144
        private const val SHRINK = 0.9
    }
}
