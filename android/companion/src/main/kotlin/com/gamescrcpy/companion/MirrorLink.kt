// SPDX-License-Identifier: GPL-3.0-or-later
// The companion's mirror link (SPEC 11.10): the GSCM handshake, its encrypted records and the multiplexed records.
package com.gamescrcpy.companion

import java.io.DataInputStream
import java.io.EOFException
import java.io.IOException
import java.net.InetSocketAddress
import java.net.Socket
import java.nio.ByteBuffer
import java.security.SecureRandom
import javax.crypto.Mac
import javax.crypto.spec.SecretKeySpec

sealed interface MirrorRecord {
    class Data(val channel: Int, val bytes: ByteArray, val offset: Int, val length: Int) : MirrorRecord

    data class Op(val op: Int, val channel: Int) : MirrorRecord
}

object MirrorCodec {
    const val CONTEXT = "GSCP/1 companion mirror"
    const val TOKEN_CONTEXT = "GSCP/1 companion mirror token"
    const val HEADER = 5
    const val DATA_MAX = 1024 * 1024
    const val LINK = 0xFF
    const val CHANNELS = 4
    const val OPEN = 0
    const val ACCEPT = 1
    const val CLOSE = 2
    const val CONTROL = 0
    const val VIDEO = 1
    const val AUDIO = 2
    const val MIN_RECORD = HEADER + 1 + LanCodec.TAG_SIZE
    const val MAX_RECORD = HEADER + DATA_MAX + LanCodec.TAG_SIZE
    val MAGIC = byteArrayOf('G'.code.toByte(), 'S'.code.toByte(), 'C'.code.toByte(), 'M'.code.toByte())

    fun sessionKey(key: ByteArray, hello: ByteArray, reply: ByteArray): ByteArray {
        val mac = Mac.getInstance("HmacSHA256")
        mac.init(SecretKeySpec(key, "HmacSHA256"))
        mac.update(CONTEXT.toByteArray(Charsets.US_ASCII))
        mac.update(hello)
        mac.update(reply)
        return mac.doFinal()
    }

    fun token(session: ByteArray): ByteArray {
        val mac = Mac.getInstance("HmacSHA256")
        mac.init(SecretKeySpec(session, "HmacSHA256"))
        return mac.doFinal(TOKEN_CONTEXT.toByteArray(Charsets.US_ASCII)).copyOf(16)
    }

    fun data(channel: Int, bytes: ByteArray, offset: Int, length: Int): ByteArray {
        require(channel in 0 until CHANNELS && length in 1..DATA_MAX)
        val out = ByteArray(HEADER + length)
        out[0] = channel.toByte()
        ByteBuffer.wrap(out, 1, 4).putInt(length)
        System.arraycopy(bytes, offset, out, HEADER, length)
        return out
    }

    fun op(op: Int, channel: Int): ByteArray =
        byteArrayOf(LINK.toByte(), 0, 0, 0, 2, op.toByte(), channel.toByte())

    fun parse(record: ByteArray): MirrorRecord {
        if (record.size < HEADER) {
            throw LanException(LanError.TRUNCATED)
        }
        val channel = record[0].toInt() and 0xFF
        val length = ByteBuffer.wrap(record, 1, 4).int.toLong() and 0xFFFFFFFFL
        val rest = record.size - HEADER
        if (channel != LINK && channel >= CHANNELS) {
            throw LanException(LanError.INVALID_ENUM)
        }
        if (channel != LINK) {
            if (length == 0L) {
                throw LanException(LanError.OUT_OF_RANGE)
            }
            if (length > DATA_MAX) {
                throw LanException(LanError.LENGTH_EXCEEDED)
            }
            if (rest.toLong() != length) {
                throw LanException(LanError.TRUNCATED)
            }
            return MirrorRecord.Data(channel, record, HEADER, length.toInt())
        }
        if (length != 2L || rest != 2) {
            throw LanException(LanError.OUT_OF_RANGE)
        }
        val op = record[HEADER].toInt() and 0xFF
        val target = record[HEADER + 1].toInt() and 0xFF
        if (op > CLOSE || target >= CHANNELS) {
            throw LanException(LanError.INVALID_ENUM)
        }
        if (op == ACCEPT) {
            throw LanException(LanError.OUT_OF_RANGE)
        }
        return MirrorRecord.Op(op, target)
    }
}

class MirrorConnection(private val socket: Socket, session: ByteArray) : AutoCloseable {
    val token: ByteArray = MirrorCodec.token(session)
    private val input = DataInputStream(socket.getInputStream())
    private val output = socket.getOutputStream()
    private val sealer = RecordSealer(session, LanCodec.TO_COMPUTER)
    private val opener =
        RecordOpener(session, LanCodec.TO_COMPANION, 0, MirrorCodec.MIN_RECORD, MirrorCodec.MAX_RECORD)

    @Throws(IOException::class)
    fun send(record: ByteArray) {
        val sealed = synchronized(sealer) { sealer.seal(record) }
        synchronized(output) {
            output.write(sealed)
            output.flush()
        }
    }

    @Throws(IOException::class)
    fun receive(): ByteArray? {
        val prefix = ByteArray(4)
        try {
            input.readFully(prefix)
        } catch (e: EOFException) {
            return null
        }
        val body = ByteArray(LanCodec.recordLength(prefix, MirrorCodec.MIN_RECORD, MirrorCodec.MAX_RECORD))
        input.readFully(body)
        return opener.open(prefix, body)
    }

    override fun close() {
        runCatching { socket.close() }
    }

    companion object {
        private val random = SecureRandom()

        fun join(host: String, port: Int, phone: ByteArray, key: ByteArray, pc: ByteArray): MirrorConnection? {
            if (key.size != LanCodec.KEY_SIZE || host.isEmpty() || port <= 0) {
                return null
            }
            val socket = Socket()
            try {
                socket.connect(InetSocketAddress(host, port), CONNECT_MILLIS)
                socket.soTimeout = HANDSHAKE_MILLIS
                socket.tcpNoDelay = true
                val nonce = ByteArray(LanCodec.NONCE_SIZE).also { random.nextBytes(it) }
                val hello = LanCodec.hello("", phone, nonce, MirrorCodec.MAGIC)
                socket.getOutputStream().apply {
                    write(hello)
                    flush()
                }
                val answer = ByteArray(LanCodec.REPLY_SIZE)
                DataInputStream(socket.getInputStream()).readFully(answer)
                val reply = LanCodec.parseReply(answer, MirrorCodec.MAGIC)
                if (reply.status != LanStatus.ACCEPTED || !reply.pc.contentEquals(pc)) {
                    socket.close()
                    return null
                }
                socket.soTimeout = 0
                return MirrorConnection(socket, MirrorCodec.sessionKey(key, hello, answer))
            } catch (e: IOException) {
                runCatching { socket.close() }
                return null
            } catch (e: IllegalArgumentException) {
                runCatching { socket.close() }
                return null
            }
        }

        private const val CONNECT_MILLIS = 3_000
        private const val HANDSHAKE_MILLIS = 5_000
    }
}
