// SPDX-License-Identifier: GPL-3.0-or-later
package com.gamescrcpy.companion

import com.gamescrcpy.protocol.Codec
import com.gamescrcpy.protocol.Direction
import com.gamescrcpy.protocol.Enums.ErrorCode
import com.gamescrcpy.protocol.FrameContext
import com.gamescrcpy.protocol.Messages
import java.io.DataInputStream
import java.net.InetAddress
import java.net.ServerSocket
import java.security.SecureRandom
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit
import kotlin.concurrent.thread
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertInstanceOf
import org.junit.jupiter.api.Test

class MirrorServerTest {

    @Test
    fun closingAChannelFlushesControlFramesQueuedBeforeClose() {
        val key = ByteArray(LanCodec.KEY_SIZE) { (it + 3).toByte() }
        val phone = ByteArray(LanCodec.ID_SIZE) { (0x40 + it).toByte() }
        val pc = ByteArray(LanCodec.ID_SIZE) { (0x80 + it).toByte() }
        val received = LinkedBlockingQueue<MirrorRecord>()

        ServerSocket(0, 1, InetAddress.getLoopbackAddress()).use { listener ->
            val reader = thread(isDaemon = true, name = "fake-mac-mirror") {
                listener.accept().use { socket ->
                    socket.soTimeout = 5_000
                    val input = DataInputStream(socket.getInputStream())
                    val head = ByteArray(LanCodec.HEAD_SIZE)
                    input.readFully(head)
                    val rest = ByteArray(LanCodec.helloSize(head) - LanCodec.HEAD_SIZE)
                    input.readFully(rest)
                    val hello = head + rest
                    val nonce = ByteArray(LanCodec.NONCE_SIZE).also { SecureRandom().nextBytes(it) }
                    val reply = MirrorCodec.MAGIC + byteArrayOf(LanCodec.VERSION.toByte(), 0) + pc + nonce
                    socket.getOutputStream().write(reply)
                    socket.getOutputStream().flush()
                    val session = MirrorCodec.sessionKey(key, hello, reply)
                    val opener = RecordOpener(
                        session,
                        LanCodec.TO_COMPUTER,
                        0,
                        MirrorCodec.MIN_RECORD,
                        MirrorCodec.MAX_RECORD,
                    )
                    repeat(2) {
                        val prefix = ByteArray(4)
                        input.readFully(prefix)
                        val body = ByteArray(LanCodec.recordLength(prefix, MirrorCodec.MIN_RECORD, MirrorCodec.MAX_RECORD))
                        input.readFully(body)
                        received.put(MirrorCodec.parse(opener.open(prefix, body)))
                    }
                }
            }

            val connection = MirrorConnection.join("127.0.0.1", listener.localPort, phone, key, pc)
                ?: throw AssertionError("mirror handshake failed")
            connection.use { link ->
                val writer = MirrorWriter(link) { link.close() }
                writer.start()
                val goodbye = Messages.ServerGoodbye().apply {
                    reason = ErrorCode.NONE
                    message = "screen sharing stopped on the phone"
                }
                writer.data(MirrorCodec.CONTROL, Codec.encodeFrame(goodbye), isVideo = false)
                writer.data(MirrorCodec.VIDEO, ByteArray(128) { 0x5A }, isVideo = true)
                writer.op(MirrorCodec.CLOSE, MirrorCodec.VIDEO)
                writer.op(MirrorCodec.CLOSE, MirrorCodec.CONTROL)
                writer.flush(1_000)
                writer.close()
            }
            reader.join(5_000)

            val first = assertInstanceOf(MirrorRecord.Data::class.java, received.poll(2, TimeUnit.SECONDS))
            assertEquals(MirrorCodec.CONTROL, first.channel)
            val decoded = Codec.decodeFrame(FrameContext.CONTROL, Direction.TO_CLIENT, first.bytes, first.offset, first.length)
            val goodbye = assertInstanceOf(Messages.ServerGoodbye::class.java, decoded)
            assertEquals(ErrorCode.NONE, goodbye.reason)
            assertEquals("screen sharing stopped on the phone", goodbye.message)

            val second = assertInstanceOf(MirrorRecord.Op::class.java, received.poll(2, TimeUnit.SECONDS))
            assertEquals(MirrorRecord.Op(MirrorCodec.CLOSE, MirrorCodec.VIDEO), second)
        }
    }
}
