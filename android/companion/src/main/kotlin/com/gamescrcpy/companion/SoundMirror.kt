// SPDX-License-Identifier: GPL-3.0-or-later
// The sound of a companion mirroring session: the phone's playback captured through the screen-capture grant
// (AudioPlaybackCapture) and encoded to Opus, 48 kHz stereo.
package com.gamescrcpy.companion

import android.annotation.SuppressLint
import android.content.Context
import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioPlaybackCaptureConfiguration
import android.media.AudioRecord
import android.media.AudioTimestamp
import android.media.MediaCodec
import android.media.MediaFormat
import android.media.projection.MediaProjection
import android.os.Build
import androidx.annotation.RequiresApi
import com.gamescrcpy.protocol.Enums.AudioCodec
import com.gamescrcpy.protocol.Media
import com.gamescrcpy.protocol.MediaChannel
import com.gamescrcpy.protocol.Messages
import java.io.IOException
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.concurrent.TimeUnit

@RequiresApi(Build.VERSION_CODES.Q)
class SoundMirror(
    private val context: Context,
    private val projection: MediaProjection,
    private val sink: () -> MirrorSink?,
) {
    private var record: AudioRecord? = null
    private var codec: MediaCodec? = null
    private var feeder: Thread? = null
    private var sender: Thread? = null
    private var session = 0

    @Volatile
    private var active = false

    @Volatile
    var running = false
        private set

    @Synchronized
    fun start(bitrate: Int): Boolean {
        stop()
        val opened = open() ?: return false
        val encoder = try {
            MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_OPUS).also { made ->
                val format = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_OPUS, SAMPLE_RATE, CHANNELS)
                format.setInteger(MediaFormat.KEY_BIT_RATE, if (bitrate > 0) bitrate else DEFAULT_BITRATE)
                made.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
                made.start()
            }
        } catch (e: IOException) {
            opened.release()
            return false
        } catch (e: RuntimeException) {
            opened.release()
            return false
        }
        try {
            opened.startRecording()
        } catch (e: RuntimeException) {
            opened.release()
            encoder.release()
            return false
        }
        record = opened
        codec = encoder
        session = if (session >= MAX_SESSION) 1 else session + 1
        val info = Messages.AudioSessionInfo()
        info.codec = AudioCodec.OPUS
        info.sampleRate = SAMPLE_RATE.toLong()
        info.channels = CHANNELS
        sink()?.audioPacket(Media.encodeAudioSession(session, info))
        active = true
        running = true
        PhoneSpeaker.silence(context)
        val number = session
        feeder = Thread({ feed(opened, encoder) }, "companion-mirror-sound-in").apply {
            isDaemon = true
            start()
        }
        sender = Thread({ send(encoder, number) }, "companion-mirror-sound-out").apply {
            isDaemon = true
            start()
        }
        return true
    }

    @Synchronized
    fun stop() {
        if (running) {
            PhoneSpeaker.restore(context)
        }
        active = false
        running = false
        record?.let { runCatching { it.stop() } }
        listOf(feeder, sender).forEach { thread -> thread?.let { runCatching { it.join(JOIN_MILLIS) } } }
        feeder = null
        sender = null
        codec?.let { encoder ->
            runCatching { encoder.stop() }
            encoder.release()
        }
        codec = null
        record?.release()
        record = null
    }

    @SuppressLint("MissingPermission")
    private fun open(): AudioRecord? {
        val config = AudioPlaybackCaptureConfiguration.Builder(projection)
            .addMatchingUsage(AudioAttributes.USAGE_MEDIA)
            .addMatchingUsage(AudioAttributes.USAGE_GAME)
            .addMatchingUsage(AudioAttributes.USAGE_UNKNOWN)
            .build()
        val format = AudioFormat.Builder()
            .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
            .setSampleRate(SAMPLE_RATE)
            .setChannelMask(AudioFormat.CHANNEL_IN_STEREO)
            .build()
        val minimum = AudioRecord.getMinBufferSize(SAMPLE_RATE, AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_16BIT)
        return try {
            AudioRecord.Builder()
                .setAudioFormat(format)
                .setAudioPlaybackCaptureConfig(config)
                .setBufferSizeInBytes(if (minimum > 0) 8 * minimum else READ_BYTES * 8)
                .build()
                .takeIf { it.state == AudioRecord.STATE_INITIALIZED }
        } catch (e: RuntimeException) {
            null
        }
    }

    private fun feed(source: AudioRecord, encoder: MediaCodec) {
        val pcm = ByteArray(READ_BYTES)
        val stamp = AudioTimestamp()
        var frame = 0L
        try {
            while (active) {
                val read = source.read(pcm, 0, pcm.size)
                if (read <= 0) {
                    return
                }
                val capturedUs = if (source.getTimestamp(stamp, AudioTimestamp.TIMEBASE_MONOTONIC) == AudioRecord.SUCCESS) {
                    (stamp.nanoTime + (frame - stamp.framePosition) * NANOS_PER_SECOND / SAMPLE_RATE) / 1000
                } else {
                    TimeUnit.NANOSECONDS.toMicros(System.nanoTime()) - read / BYTES_PER_FRAME * MICROS_PER_SECOND / SAMPLE_RATE
                }
                var offset = 0
                while (offset < read && active) {
                    val index = encoder.dequeueInputBuffer(DEQUEUE_US)
                    if (index < 0) {
                        continue
                    }
                    val buffer = encoder.getInputBuffer(index) ?: return
                    val size = minOf(read - offset, buffer.remaining())
                    buffer.put(pcm, offset, size)
                    val at = capturedUs + (offset / BYTES_PER_FRAME) * MICROS_PER_SECOND / SAMPLE_RATE
                    encoder.queueInputBuffer(index, 0, size, at.coerceAtLeast(0), 0)
                    offset += size
                }
                frame += read / BYTES_PER_FRAME
            }
        } catch (e: IllegalStateException) {
            Unit
        }
    }

    private fun send(encoder: MediaCodec, number: Int) {
        val info = MediaCodec.BufferInfo()
        var previous = Long.MIN_VALUE
        try {
            while (active) {
                val index = encoder.dequeueOutputBuffer(info, DEQUEUE_US)
                if (index < 0) {
                    continue
                }
                val buffer = encoder.getOutputBuffer(index)
                if (buffer != null && info.size > 0) {
                    buffer.position(info.offset)
                    buffer.limit(info.offset + info.size)
                    if (info.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG != 0) {
                        opusHead(buffer)?.let { head ->
                            sink()?.audioPacket(Media.encodeConfig(MediaChannel.AUDIO, number, head))
                        }
                    } else {
                        val bytes = ByteArray(info.size)
                        buffer.get(bytes)
                        val pts = maxOf(info.presentationTimeUs, previous + 1)
                        previous = pts
                        val nowUs = TimeUnit.NANOSECONDS.toMicros(System.nanoTime())
                        val delay = (nowUs - pts).coerceIn(0, MAX_U32)
                        sink()?.audioPacket(Media.encodeFrame(MediaChannel.AUDIO, number, 0, pts, delay, bytes))
                    }
                }
                encoder.releaseOutputBuffer(index, false)
            }
        } catch (e: IllegalStateException) {
            Unit
        }
    }

    companion object {
        private const val SAMPLE_RATE = 48_000
        private const val CHANNELS = 2
        private const val BYTES_PER_FRAME = CHANNELS * 2
        private const val READ_BYTES = 960 * BYTES_PER_FRAME
        private const val DEFAULT_BITRATE = 128_000
        private const val DEQUEUE_US = 100_000L
        private const val JOIN_MILLIS = 1_000L
        private const val MAX_SESSION = 0xFFFF
        private const val MAX_U32 = 0xFFFF_FFFFL
        private const val NANOS_PER_SECOND = 1_000_000_000L
        private const val MICROS_PER_SECOND = 1_000_000L
        private val OPUS_HEAD = "OpusHead".toByteArray(Charsets.US_ASCII)
        private val HEADER_SECTION = "AOPUSHDR".toByteArray(Charsets.US_ASCII)

        // The Opus encoder hands its configuration either as a bare OpusHead or as sections (an 8-byte id and a
        // native-order 64-bit length each), one of them AOPUSHDR holding the OpusHead.
        fun opusHead(config: ByteBuffer): ByteArray? {
            val sections = config.slice().order(ByteOrder.nativeOrder())
            if (startsWith(sections, OPUS_HEAD)) {
                return ByteArray(sections.remaining()).also { sections.get(it) }
            }
            while (sections.remaining() >= 16) {
                val header = startsWith(sections, HEADER_SECTION)
                sections.position(sections.position() + 8)
                val length = sections.long
                if (length < 0 || length > sections.remaining()) {
                    return null
                }
                if (header) {
                    val head = ByteArray(length.toInt())
                    sections.get(head)
                    return head.takeIf { it.size >= OPUS_HEAD.size && it.copyOf(OPUS_HEAD.size).contentEquals(OPUS_HEAD) }
                }
                sections.position(sections.position() + length.toInt())
            }
            return null
        }

        private fun startsWith(buffer: ByteBuffer, prefix: ByteArray): Boolean {
            if (buffer.remaining() < prefix.size) {
                return false
            }
            return prefix.indices.all { buffer.get(buffer.position() + it) == prefix[it] }
        }
    }
}
