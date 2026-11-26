package com.gamescrcpy.server.audio;

import android.annotation.SuppressLint;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.AudioTimestamp;
import android.media.MediaRecorder;
import android.os.Build;
import com.gamescrcpy.protocol.Enums.ErrorCode;
import com.gamescrcpy.server.Log;
import com.gamescrcpy.server.wrappers.ProcessEnvironment;
import java.lang.reflect.Method;

final class AudioCapture implements AutoCloseable {

    static final int SAMPLE_RATE = 48_000;
    static final int CHANNELS = 2;
    static final int BYTES_PER_FRAME = CHANNELS * 2;
    static final int MAX_READ_BYTES = 960 * BYTES_PER_FRAME;

    // AudioRecord.SUBMIX_FIXED_VOLUME (hidden): while a REMOTE_SUBMIX recording carries this tag, AudioService plays
    // the remote submix at fixed full volume, so the phone's volume no longer changes what the computer hears; the
    // computer has its own volume.
    static final String SUBMIX_FIXED_VOLUME = "fixedVolume";

    private final AudioRecord record;
    private final AudioTimestamp timestamp = new AudioTimestamp();
    private long framesRead;

    @SuppressLint({"MissingPermission", "WrongConstant"})
    AudioCapture() throws AudioException {
        final AudioFormat format = new AudioFormat.Builder()
                .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                .setSampleRate(SAMPLE_RATE)
                .setChannelMask(AudioFormat.CHANNEL_IN_STEREO)
                .build();
        AudioRecord opened = null;
        final AudioAttributes fixedVolume = fixedVolumeAttributes();
        if (fixedVolume != null) {
            try {
                opened = builder(format, fixedVolume).build();
                Log.i("capturing the audio output at fixed volume: the computer sets the volume");
            } catch (RuntimeException e) {
                Log.w("cannot capture the audio output at fixed volume; the phone's volume applies", e);
            }
        }
        if (opened == null) {
            try {
                opened = builder(format, null).build();
            } catch (RuntimeException e) {
                throw new AudioException(ErrorCode.AUDIO_CAPTURE_FAILED, "cannot open the audio output for capture",
                        e);
            }
        }
        record = opened;
        try {
            record.startRecording();
        } catch (RuntimeException e) {
            record.release();
            throw new AudioException(ErrorCode.AUDIO_CAPTURE_FAILED, "cannot start capturing the audio output", e);
        }
        if (record.getRecordingState() != AudioRecord.RECORDSTATE_RECORDING) {
            record.release();
            throw new AudioException(ErrorCode.AUDIO_CAPTURE_FAILED, "the audio output did not start recording");
        }
    }

    private static AudioRecord.Builder builder(AudioFormat format, AudioAttributes attributes) {
        final AudioRecord.Builder builder = new AudioRecord.Builder()
                .setAudioSource(MediaRecorder.AudioSource.REMOTE_SUBMIX)
                .setAudioFormat(format);
        if (attributes != null) {
            setAttributes(builder, attributes);
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            builder.setContext(ProcessEnvironment.context());
        }
        final int minimum = AudioRecord.getMinBufferSize(SAMPLE_RATE, AudioFormat.CHANNEL_IN_STEREO,
                AudioFormat.ENCODING_PCM_16BIT);
        if (minimum > 0) {
            builder.setBufferSizeInBytes(8 * minimum);
        }
        return builder;
    }

    /** REMOTE_SUBMIX attributes tagged {@link #SUBMIX_FIXED_VOLUME}, or null where the hidden calls are missing. */
    static AudioAttributes fixedVolumeAttributes() {
        try {
            final AudioAttributes.Builder attributes = new AudioAttributes.Builder();
            final Method preset = AudioAttributes.Builder.class.getMethod("setInternalCapturePreset", int.class);
            preset.invoke(attributes, MediaRecorder.AudioSource.REMOTE_SUBMIX);
            final Method tag = AudioAttributes.Builder.class.getMethod("addTag", String.class);
            tag.invoke(attributes, SUBMIX_FIXED_VOLUME);
            return attributes.build();
        } catch (ReflectiveOperationException | RuntimeException e) {
            Log.w("fixed-volume audio capture is not available; the phone's volume applies", e);
            return null;
        }
    }

    // setAudioAttributes replaces what setAudioSource set (both write the same field), so it is called after it.
    private static void setAttributes(AudioRecord.Builder builder, AudioAttributes attributes) {
        try {
            AudioRecord.Builder.class.getMethod("setAudioAttributes", AudioAttributes.class).invoke(builder,
                    attributes);
        } catch (ReflectiveOperationException | RuntimeException e) {
            Log.w("cannot set the audio capture attributes; the phone's volume applies", e);
        }
    }

    int read(byte[] buffer, int length) {
        final int read = record.read(buffer, 0, Math.min(length, MAX_READ_BYTES));
        if (read > 0) {
            framesRead += read / BYTES_PER_FRAME;
        }
        return read;
    }

    long captureTimeUs(long frame) {
        if (record.getTimestamp(timestamp, AudioTimestamp.TIMEBASE_MONOTONIC) == AudioRecord.SUCCESS) {
            return (timestamp.nanoTime + (frame - timestamp.framePosition) * 1_000_000_000L / SAMPLE_RATE) / 1000;
        }
        return System.nanoTime() / 1000 - (framesRead - frame) * 1_000_000L / SAMPLE_RATE;
    }

    void stop() {
        try {
            record.stop();
        } catch (IllegalStateException ignored) {
        }
    }

    @Override
    public void close() {
        record.release();
    }
}
