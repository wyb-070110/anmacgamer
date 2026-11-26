// SPDX-License-Identifier: GPL-3.0-or-later
// The companion as the phone-side server of a mirroring session (SPEC 11.10): one mirror link carries the control,
// video and audio channels as virtual connections; the picture comes from screen capture, the sound from playback
// capture, and touch goes through AOA, not through here.
package com.gamescrcpy.companion

import android.content.Context
import android.media.MediaCodecList
import android.hardware.display.DisplayManager
import android.media.projection.MediaProjection
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.util.DisplayMetrics
import android.view.Display
import com.gamescrcpy.protocol.Codec
import com.gamescrcpy.protocol.Constants
import com.gamescrcpy.protocol.Direction
import com.gamescrcpy.protocol.Enums.AudioStatus
import com.gamescrcpy.protocol.Enums.ErrorCode
import com.gamescrcpy.protocol.Enums.NoticeLevel
import com.gamescrcpy.protocol.Enums.VideoCodec
import com.gamescrcpy.protocol.Enums.VideoSource
import com.gamescrcpy.protocol.FrameContext
import com.gamescrcpy.protocol.Message
import com.gamescrcpy.protocol.Messages
import com.gamescrcpy.protocol.Proofs
import java.io.EOFException
import java.io.IOException
import java.io.InputStream
import java.net.SocketTimeoutException
import java.security.SecureRandom
import java.util.concurrent.TimeUnit
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock

interface MirrorSink {
    fun videoPacket(packet: ByteArray, key: Boolean, frame: Boolean)

    fun audioPacket(packet: ByteArray)
}

interface MirrorHost {
    val context: Context

    fun askProjection(done: (MediaProjection?) -> Unit)

    fun projectionEnded()

    fun audioAllowed(): Boolean
}

class ChannelInput : InputStream() {
    private val lock = ReentrantLock()
    private val arrived = lock.newCondition()
    private val chunks = ArrayDeque<ByteArray>()
    private var position = 0
    private var ended = false

    @Volatile
    var timeoutMillis = 0L

    fun feed(bytes: ByteArray, offset: Int, length: Int) {
        lock.withLock {
            if (ended) {
                return
            }
            chunks.addLast(bytes.copyOfRange(offset, offset + length))
            arrived.signalAll()
        }
    }

    fun end() {
        lock.withLock {
            ended = true
            arrived.signalAll()
        }
    }

    override fun read(): Int {
        val one = ByteArray(1)
        return if (read(one, 0, 1) < 0) -1 else one[0].toInt() and 0xFF
    }

    override fun read(buffer: ByteArray, offset: Int, length: Int): Int {
        if (length == 0) {
            return 0
        }
        lock.withLock {
            val limit = timeoutMillis
            var left = TimeUnit.MILLISECONDS.toNanos(limit)
            while (chunks.isEmpty() && !ended) {
                if (limit <= 0) {
                    arrived.await()
                    continue
                }
                if (left <= 0) {
                    throw SocketTimeoutException("nothing came from the computer in time")
                }
                left = arrived.awaitNanos(left)
            }
            val head = chunks.firstOrNull() ?: return -1
            val count = minOf(length, head.size - position)
            System.arraycopy(head, position, buffer, offset, count)
            position += count
            if (position == head.size) {
                chunks.removeFirst()
                position = 0
            }
            return count
        }
    }
}

class MirrorWriter(private val link: MirrorConnection, private val broken: () -> Unit) {
    private class Item(val channel: Int, val record: ByteArray)

    private val lock = ReentrantLock()
    private val ready = lock.newCondition()
    private val drained = lock.newCondition()
    private val urgent = ArrayDeque<Item>()
    private val video = ArrayDeque<Item>()
    private var videoBytes = 0L
    private var sending = false
    private var closed = false
    private val thread = Thread(::run, "companion-mirror-out").apply { isDaemon = true }

    val videoBacklog: Long
        get() = lock.withLock { videoBytes }

    fun start() = thread.start()

    fun data(channel: Int, bytes: ByteArray, isVideo: Boolean) {
        lock.withLock {
            if (closed) {
                return
            }
            var offset = 0
            while (offset < bytes.size) {
                val size = minOf(MirrorCodec.DATA_MAX, bytes.size - offset)
                val item = Item(channel, MirrorCodec.data(channel, bytes, offset, size))
                if (isVideo) {
                    video.addLast(item)
                    videoBytes += item.record.size
                } else {
                    urgent.addLast(item)
                }
                offset += size
            }
            ready.signalAll()
        }
    }

    fun op(op: Int, channel: Int) {
        lock.withLock {
            if (closed) {
                return
            }
            if (op == MirrorCodec.CLOSE) {
                video.removeAll { item ->
                    (item.channel == channel).also { if (it) videoBytes -= item.record.size }
                }
            }
            urgent.addLast(Item(-1, MirrorCodec.op(op, channel)))
            ready.signalAll()
        }
    }

    fun flush(timeoutMillis: Long) {
        lock.withLock {
            var left = TimeUnit.MILLISECONDS.toNanos(timeoutMillis)
            while (!closed && (urgent.isNotEmpty() || sending) && left > 0) {
                try {
                    left = drained.awaitNanos(left)
                } catch (e: InterruptedException) {
                    Thread.currentThread().interrupt()
                    return
                }
            }
        }
    }

    fun close() {
        lock.withLock {
            closed = true
            urgent.clear()
            video.clear()
            videoBytes = 0
            ready.signalAll()
            drained.signalAll()
        }
    }

    private fun run() {
        while (true) {
            val item = lock.withLock {
                sending = false
                if (urgent.isEmpty()) {
                    drained.signalAll()
                }
                while (!closed && urgent.isEmpty() && video.isEmpty()) {
                    ready.await()
                }
                if (closed) {
                    drained.signalAll()
                    return
                }
                sending = true
                urgent.removeFirstOrNull() ?: video.removeFirst().also { videoBytes -= it.record.size }
            }
            try {
                link.send(item.record)
            } catch (e: IOException) {
                lock.withLock {
                    sending = false
                    closed = true
                    drained.signalAll()
                }
                broken()
                return
            }
        }
    }
}

class MirrorServer(private val link: MirrorConnection, private val host: MirrorHost) : AutoCloseable {

    private inner class Channel(val number: Int) {
        val input = ChannelInput()
        var closeSent = false
        var closeReceived = false

        fun write(message: Message) = write(Codec.encodeFrame(message), false)

        fun write(bytes: ByteArray, isVideo: Boolean) {
            lock.withLock {
                if (closeSent) {
                    return
                }
                writer.data(number, bytes, isVideo)
            }
        }

        fun accept(): Boolean = lock.withLock {
            if (closeSent || closeReceived) {
                false
            } else {
                writer.op(MirrorCodec.ACCEPT, number)
                true
            }
        }

        fun close() {
            lock.withLock {
                if (closeSent) {
                    return
                }
                closeSent = true
                writer.op(MirrorCodec.CLOSE, number)
                forget(this)
            }
            input.end()
        }
    }

    private inner class Session(val control: Channel) : MirrorSink {
        var video: Channel? = null
        var audio: Channel? = null
        var videoConfig: Messages.ConfigureVideo? = null
        var paused = false
        var audioConfig: Messages.ConfigureAudio? = null
        var dropping = false

        @Synchronized
        fun handle(message: Message): Boolean {
            when (message) {
                is Messages.Ping -> control.write(pong(message))
                is Messages.Goodbye -> return false
                is Messages.ConfigureVideo -> {
                    if (message.source != VideoSource.DISPLAY || message.displayId != 0L ||
                        message.codec != VideoCodec.H264
                    ) {
                        control.write(notice(ErrorCode.CHANNEL_UNAVAILABLE, "the companion mirrors the screen in H.264 only"))
                    } else {
                        videoConfig = message
                        refreshVideo(restart = true)
                    }
                }
                is Messages.SetVideoBitrate -> {
                    videoConfig?.bitrate = message.bitrate
                    screen?.setBitrate(message.bitrate.toInt())
                }
                is Messages.SetMaxFps -> {
                    videoConfig?.maxFps = message.maxFps
                    refreshVideo(restart = true)
                }
                is Messages.RequestKeyFrame -> screen?.requestKeyFrame()
                is Messages.SetVideoPaused -> {
                    paused = message.paused
                    refreshVideo(restart = false)
                }
                is Messages.ConfigureAudio -> {
                    audioConfig = message
                    refreshAudio()
                }
                else -> Unit
            }
            return true
        }

        @Synchronized
        fun attach(channel: Channel) {
            if (channel.number == MirrorCodec.VIDEO) {
                video = channel
                refreshVideo(restart = true)
            } else {
                audio = channel
                refreshAudio()
            }
        }

        @Synchronized
        fun detach(channel: Channel) {
            if (video === channel) {
                video = null
                refreshVideo(restart = false)
            }
            if (audio === channel) {
                audio = null
                refreshAudio()
            }
        }

        @Synchronized
        fun end() {
            video?.close()
            audio?.close()
            video = null
            audio = null
            screen?.stop()
            sound?.stop()
        }

        private fun refreshVideo(restart: Boolean) {
            val config = videoConfig
            val shown = screen
            if (shown == null || video == null || config == null || paused) {
                shown?.stop()
                return
            }
            if (restart || !shown.running) {
                dropping = false
                shown.start(config.maxSize, config.maxFps, config.bitrate.toInt())
            }
        }

        private fun refreshAudio() {
            val config = audioConfig
            val heard = sound
            if (heard == null || audio == null || config == null || !config.enabled) {
                heard?.stop()
                return
            }
            if (!heard.running && !heard.start(config.bitrate.toInt())) {
                control.write(notice(ErrorCode.AUDIO_CAPTURE_FAILED, "the companion cannot capture the phone's sound"))
            }
        }

        override fun videoPacket(packet: ByteArray, key: Boolean, frame: Boolean) {
            val channel = video ?: return
            if (frame) {
                if (writer.videoBacklog > BACKLOG_BYTES && !dropping) {
                    dropping = true
                    screen?.requestKeyFrame()
                }
                if (dropping && !key) {
                    return
                }
                dropping = false
            }
            channel.write(packet, true)
        }

        override fun audioPacket(packet: ByteArray) {
            audio?.write(packet, false)
        }
    }

    private val lock = ReentrantLock()
    private val channels = arrayOfNulls<Channel>(MirrorCodec.CHANNELS)
    private val writer = MirrorWriter(link) { link.close() }
    private val random = SecureRandom()

    @Volatile
    private var projection: MediaProjection? = null

    @Volatile
    private var screen: ScreenMirror? = null

    @Volatile
    private var sound: SoundMirror? = null

    @Volatile
    private var session: Session? = null

    private val stopped = object : MediaProjection.Callback() {
        override fun onStop() {
            Thread({ projectionStopped() }, "companion-mirror-stop").apply { isDaemon = true }.start()
        }
    }

    fun run() {
        writer.start()
        try {
            while (true) {
                val record = link.receive() ?: break
                when (val parsed = MirrorCodec.parse(record)) {
                    is MirrorRecord.Data -> lock.withLock { channels[parsed.channel] }
                        ?.takeUnless { it.closeReceived }
                        ?.input?.feed(parsed.bytes, parsed.offset, parsed.length)
                    is MirrorRecord.Op -> if (!linkOp(parsed)) {
                        break
                    }
                }
            }
        } catch (e: IOException) {
            Unit
        } finally {
            close()
        }
    }

    fun stop() {
        session?.let { active ->
            active.control.write(serverGoodbye(ErrorCode.NONE, "stopped on the phone"))
            active.control.close()
            active.end()
            writer.flush(DRAIN_MILLIS)
        }
        close()
    }

    override fun close() {
        val open = lock.withLock { channels.filterNotNull() }
        open.forEach { it.input.end() }
        session?.end()
        session = null
        writer.close()
        link.close()
        releaseProjection()
    }

    private fun linkOp(op: MirrorRecord.Op): Boolean {
        if (op.op == MirrorCodec.OPEN) {
            val channel = lock.withLock {
                if (channels[op.channel] != null) {
                    return false
                }
                Channel(op.channel).also { channels[op.channel] = it }
            }
            when (op.channel) {
                MirrorCodec.CONTROL -> spawn("companion-mirror-control") { control(channel) }
                MirrorCodec.VIDEO, MirrorCodec.AUDIO -> spawn("companion-mirror-media") { media(channel) }
                else -> channel.close()
            }
            return true
        }
        val channel = lock.withLock { channels[op.channel] } ?: return true
        lock.withLock { channel.closeReceived = true }
        channel.input.end()
        channel.close()
        lock.withLock { forget(channel) }
        return true
    }

    private fun forget(channel: Channel) {
        if (channel.closeSent && channel.closeReceived && channels[channel.number] === channel) {
            channels[channel.number] = null
        }
    }

    private fun spawn(name: String, body: () -> Unit) {
        Thread(body, name).apply { isDaemon = true }.start()
    }

    private fun control(channel: Channel) {
        try {
            converse(channel)
        } catch (e: IOException) {
            Unit
        } catch (e: RuntimeException) {
            Unit
        } finally {
            channel.close()
        }
    }

    private fun converse(channel: Channel) {
        val granted = ensureProjection()
        if (!channel.accept()) {
            return
        }
        if (granted == null) {
            channel.write(handshakeError(ErrorCode.PERMISSION_DENIED, "the screen capture was not allowed on the phone"))
            return
        }
        val nonce = ByteArray(16).also { random.nextBytes(it) }
        channel.write(serverHello(nonce))
        channel.input.timeoutMillis = HANDSHAKE_MILLIS
        val hello = read(channel.input, FrameContext.HANDSHAKE)
        if (hello !is Messages.ClientHello) {
            channel.write(handshakeError(ErrorCode.UNEXPECTED_MESSAGE, "expected ClientHello"))
            return
        }
        if (hello.protocolVersion != Constants.PROTOCOL_VERSION) {
            channel.write(
                handshakeError(
                    ErrorCode.VERSION_MISMATCH,
                    "client protocol ${hello.protocolVersion}, companion protocol ${Constants.PROTOCOL_VERSION}",
                ),
            )
            return
        }
        if (!Proofs.matches(Proofs.hello(link.token, nonce), hello.proof)) {
            channel.write(handshakeError(ErrorCode.AUTHENTICATION_FAILED, "wrong proof"))
            return
        }
        channel.write(welcome())
        val current = Session(channel)
        session?.end()
        session = current
        channel.input.timeoutMillis = SILENCE_MILLIS
        var goodbye = false
        try {
            while (true) {
                val message = read(channel.input, FrameContext.CONTROL)
                if (!current.handle(message)) {
                    goodbye = true
                    break
                }
            }
        } catch (e: EOFException) {
            Unit
        } finally {
            current.end()
            if (session === current) {
                session = null
            }
            if (goodbye) {
                releaseProjection()
            }
        }
    }

    private fun media(channel: Channel) {
        var attached: Session? = null
        try {
            if (!channel.accept()) {
                return
            }
            val nonce = ByteArray(16).also { random.nextBytes(it) }
            channel.write(serverHello(nonce))
            channel.input.timeoutMillis = HANDSHAKE_MILLIS
            val open = read(channel.input, FrameContext.HANDSHAKE)
            val current = session
            if (open !is Messages.ChannelOpen || open.channel.wireValue() != channel.number) {
                channel.write(handshakeError(ErrorCode.UNEXPECTED_MESSAGE, "expected ChannelOpen"))
                return
            }
            if (current == null || !Proofs.matches(Proofs.channel(link.token, nonce, channel.number), open.proof)) {
                channel.write(handshakeError(ErrorCode.AUTHENTICATION_FAILED, "wrong proof"))
                return
            }
            val accepted = Messages.ChannelAccepted()
            accepted.channel = open.channel
            channel.write(accepted)
            current.attach(channel)
            attached = current
            channel.input.timeoutMillis = 0
            val scratch = ByteArray(256)
            while (channel.input.read(scratch, 0, scratch.size) >= 0) {
                Unit
            }
        } catch (e: IOException) {
            Unit
        } catch (e: RuntimeException) {
            Unit
        } finally {
            attached?.detach(channel)
            channel.close()
        }
    }

    private fun ensureProjection(): MediaProjection? {
        projection?.let { return it }
        val answer = java.util.concurrent.LinkedBlockingQueue<Any>(1)
        host.askProjection { granted -> answer.offer(granted ?: NONE) }
        val granted = answer.poll(CONSENT_MILLIS, TimeUnit.MILLISECONDS) as? MediaProjection ?: return null
        granted.registerCallback(stopped, Handler(Looper.getMainLooper()))
        projection = granted
        screen = ScreenMirror(host.context, granted) { session }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q && host.audioAllowed()) {
            sound = SoundMirror(host.context, granted) { session }
        }
        return granted
    }

    private fun projectionStopped() {
        val ended = projection ?: return
        session?.let { active ->
            active.control.write(serverGoodbye(ErrorCode.NONE, "screen sharing stopped on the phone"))
            active.control.close()
            active.end()
        }
        if (projection === ended) {
            releaseProjection()
        }
    }

    @Synchronized
    private fun releaseProjection() {
        val ended = projection ?: return
        projection = null
        screen?.release()
        screen = null
        sound?.stop()
        sound = null
        ended.unregisterCallback(stopped)
        ended.stop()
        host.projectionEnded()
    }

    private fun read(input: InputStream, context: FrameContext): Message {
        val header = ByteArray(Constants.FRAME_HEADER_SIZE)
        readFully(input, header, header.size)
        val frame = Codec.decodeFrameHeader(context, Direction.TO_SERVER, header, 0, header.size)
        val payload = ByteArray(frame.length)
        readFully(input, payload, payload.size)
        return Codec.decodePayload(frame.type.create(), payload, 0, frame.length)
    }

    private fun readFully(input: InputStream, buffer: ByteArray, length: Int) {
        var offset = 0
        while (offset < length) {
            val count = input.read(buffer, offset, length - offset)
            if (count < 0) {
                throw EOFException()
            }
            offset += count
        }
    }

    private fun serverHello(nonce: ByteArray) = Messages.ServerHello().apply {
        protocolVersion = Constants.PROTOCOL_VERSION
        this.nonce = nonce
    }

    private fun handshakeError(code: ErrorCode, text: String) = Messages.HandshakeError().apply {
        this.code = code
        message = text
    }

    private fun serverGoodbye(code: ErrorCode, text: String) = Messages.ServerGoodbye().apply {
        reason = code
        message = text
    }

    private fun notice(code: ErrorCode, text: String) = Messages.Notice().apply {
        level = NoticeLevel.ERROR
        this.code = code
        message = text
    }

    private fun pong(ping: Messages.Ping) = Messages.Pong().apply {
        nonce = ping.nonce
        clientTimeUs = ping.clientTimeUs
        serverTimeUs = TimeUnit.NANOSECONDS.toMicros(System.nanoTime()).coerceAtLeast(0)
    }

    @Suppress("DEPRECATION")
    private fun welcome(): Messages.Welcome {
        val context = host.context
        val message = Messages.Welcome()
        val version = runCatching {
            context.packageManager.getPackageInfo(context.packageName, 0).versionName
        }.getOrNull().orEmpty()
        message.serverVersion = clip("companion $version", 32)
        message.manufacturer = clip(Build.MANUFACTURER.orEmpty(), 64)
        message.model = clip(Build.MODEL.orEmpty(), 64)
        message.androidRelease = clip(Build.VERSION.RELEASE.orEmpty(), 32)
        message.sdkInt = Build.VERSION.SDK_INT
        message.serialNo = ""
        val display = context.getSystemService(DisplayManager::class.java)?.getDisplay(Display.DEFAULT_DISPLAY)
        if (display != null) {
            val metrics = DisplayMetrics()
            display.getRealMetrics(metrics)
            val shown = Messages.Display()
            shown.displayId = 0
            shown.width = metrics.widthPixels.coerceIn(0, MAX_U16)
            shown.height = metrics.heightPixels.coerceIn(0, MAX_U16)
            shown.rotation = display.rotation and 3
            shown.densityDpi = metrics.densityDpi.coerceIn(0, MAX_U16)
            shown.refreshMilliHz = (display.refreshRate * 1000).toLong().coerceAtLeast(0)
            message.displays.add(shown)
        }
        for (info in MediaCodecList(MediaCodecList.REGULAR_CODECS).codecInfos) {
            if (!info.isEncoder || info.supportedTypes.none { it.equals(ScreenMirror.MIME, ignoreCase = true) }) {
                continue
            }
            if (message.encoders.size() >= MAX_ENCODERS) {
                break
            }
            val encoder = Messages.Encoder()
            encoder.codec = VideoCodec.H264
            encoder.name = clip(info.name, 128)
            encoder.hardware = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) info.isHardwareAccelerated else true
            message.encoders.add(encoder)
        }
        message.audioStatus = when {
            Build.VERSION.SDK_INT < Build.VERSION_CODES.Q -> AudioStatus.UNSUPPORTED_ANDROID
            sound == null -> AudioStatus.CAPTURE_FAILED
            else -> AudioStatus.AVAILABLE
        }
        message.capabilities = 0
        message.resumed = false
        return message
    }

    private fun clip(text: String, bytes: Int): String {
        var end = text.length
        while (end > 0 && text.substring(0, end).toByteArray(Charsets.UTF_8).size > bytes) {
            end--
        }
        return text.substring(0, end)
    }

    private companion object {
        val NONE = Any()
        const val HANDSHAKE_MILLIS = 5_000L
        const val SILENCE_MILLIS = 6_000L
        const val CONSENT_MILLIS = 60_000L
        const val DRAIN_MILLIS = 1_000L
        const val BACKLOG_BYTES = 4L * 1024 * 1024
        const val MAX_U16 = 0xFFFF
        const val MAX_ENCODERS = 32
    }
}
