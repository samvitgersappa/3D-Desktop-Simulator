#include "audio.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#ifdef USE_OPENAL
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#endif

namespace audio {

namespace {

struct WavData {
	std::vector<std::uint8_t> pcm;
	int channels = 0;
	int sampleRate = 0;
	int bitsPerSample = 0;
};

static bool g_inited = false;

#ifdef USE_OPENAL
static ALCdevice* g_device = nullptr;
static ALCcontext* g_context = nullptr;
static std::unordered_map<std::string, ALuint> g_bufferCache;
static std::unordered_set<std::string> g_missingSounds;
static std::vector<ALuint> g_sources;
static Vec3 g_listenerPos{0, 0, 0};

static ALuint g_uiSource = 0;
static ALuint g_stepSource = 0;
static ALuint g_actionSource = 0;

static float len(Vec3 v) {
	return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static Vec3 normalize(Vec3 v) {
	float l = len(v);
	if (l <= 1e-6f) return {0.0f, 0.0f, -1.0f};
	return {v.x / l, v.y / l, v.z / l};
}

static bool read_u32_le(std::istream& in, std::uint32_t& out) {
	std::uint8_t b[4]{};
	if (!in.read(reinterpret_cast<char*>(b), 4)) return false;
	out = (std::uint32_t)b[0] | ((std::uint32_t)b[1] << 8) | ((std::uint32_t)b[2] << 16) | ((std::uint32_t)b[3] << 24);
	return true;
}

static bool read_u16_le(std::istream& in, std::uint16_t& out) {
	std::uint8_t b[2]{};
	if (!in.read(reinterpret_cast<char*>(b), 2)) return false;
	out = (std::uint16_t)b[0] | ((std::uint16_t)b[1] << 8);
	return true;
}

static bool load_wav_file(const std::string& path, WavData& wav) {
	std::ifstream in(path, std::ios::binary);
	if (!in) return false;

	char riff[4]{};
	if (!in.read(riff, 4)) return false;
	if (std::string(riff, 4) != "RIFF") return false;

	std::uint32_t riffSize = 0;
	if (!read_u32_le(in, riffSize)) return false;
	(void)riffSize;

	char wave[4]{};
	if (!in.read(wave, 4)) return false;
	if (std::string(wave, 4) != "WAVE") return false;

	bool haveFmt = false;
	bool haveData = false;
	std::uint16_t audioFormat = 0;
	std::vector<std::uint8_t> rawData;

	while (in && !(haveFmt && haveData)) {
		char chunkId[4]{};
		if (!in.read(chunkId, 4)) break;
		std::uint32_t chunkSize = 0;
		if (!read_u32_le(in, chunkSize)) break;

		std::string id(chunkId, 4);
		if (id == "fmt ") {
			std::uint16_t numChannels = 0;
			std::uint32_t sampleRate = 0;
			std::uint32_t byteRate = 0;
			std::uint16_t blockAlign = 0;
			std::uint16_t bitsPerSample = 0;

			if (!read_u16_le(in, audioFormat)) return false;
			if (!read_u16_le(in, numChannels)) return false;
			if (!read_u32_le(in, sampleRate)) return false;
			if (!read_u32_le(in, byteRate)) return false;
			if (!read_u16_le(in, blockAlign)) return false;
			if (!read_u16_le(in, bitsPerSample)) return false;
			(void)byteRate;
			(void)blockAlign;

			// Skip any remaining fmt bytes
			std::uint32_t remaining = chunkSize > 16 ? (chunkSize - 16) : 0;
			if (remaining) in.seekg(remaining, std::ios::cur);

			wav.channels = (int)numChannels;
			wav.sampleRate = (int)sampleRate;
			wav.bitsPerSample = (int)bitsPerSample;
			haveFmt = true;
		} else if (id == "data") {
			rawData.resize(chunkSize);
			if (!in.read(reinterpret_cast<char*>(rawData.data()), chunkSize)) return false;
			haveData = true;
		} else {
			in.seekg(chunkSize, std::ios::cur);
		}

		// Chunks are word-aligned
		if (chunkSize % 2 == 1) in.seekg(1, std::ios::cur);
	}

	if (!haveFmt || !haveData) return false;
	if (!((wav.channels == 1) || (wav.channels == 2))) return false;
	if (!((audioFormat == 1) || (audioFormat == 3))) return false; // 1=PCM, 3=IEEE float
	if (!((wav.bitsPerSample == 8) || (wav.bitsPerSample == 16) || (wav.bitsPerSample == 24) || (wav.bitsPerSample == 32))) return false;

	// Convert uncommon formats to 16-bit PCM for reliable OpenAL upload.
	if (wav.bitsPerSample == 8) {
		wav.pcm = std::move(rawData);
		return true;
	}

	if (wav.bitsPerSample == 16 && audioFormat == 1) {
		wav.pcm = std::move(rawData);
		return true;
	}

	// Convert to signed 16-bit little-endian.
	std::vector<std::uint8_t> out;
	out.reserve((rawData.size() / (wav.bitsPerSample / 8)) * 2);

	auto push_i16_le = [&](std::int16_t s) {
		out.push_back((std::uint8_t)(s & 0xFF));
		out.push_back((std::uint8_t)((s >> 8) & 0xFF));
	};

	if (audioFormat == 3 && wav.bitsPerSample == 32) {
		// IEEE float32 [-1,1] -> int16
		for (size_t i = 0; i + 3 < rawData.size(); i += 4) {
			float f = 0.0f;
			std::memcpy(&f, rawData.data() + i, 4);
			if (f > 1.0f) f = 1.0f;
			if (f < -1.0f) f = -1.0f;
			int v = (int)std::lround(f * 32767.0f);
			if (v > 32767) v = 32767;
			if (v < -32768) v = -32768;
			push_i16_le((std::int16_t)v);
		}
		wav.pcm = std::move(out);
		wav.bitsPerSample = 16;
		return true;
	}

	if (audioFormat == 1 && wav.bitsPerSample == 24) {
		// Signed 24-bit PCM -> int16 (drop lowest 8 bits)
		for (size_t i = 0; i + 2 < rawData.size(); i += 3) {
			int v = (int)rawData[i] | ((int)rawData[i + 1] << 8) | ((int)rawData[i + 2] << 16);
			// sign extend 24-bit
			if (v & 0x800000) v |= ~0xFFFFFF;
			v >>= 8;
			if (v > 32767) v = 32767;
			if (v < -32768) v = -32768;
			push_i16_le((std::int16_t)v);
		}
		wav.pcm = std::move(out);
		wav.bitsPerSample = 16;
		return true;
	}

	if (audioFormat == 1 && wav.bitsPerSample == 32) {
		// Signed 32-bit PCM -> int16 (drop lowest 16 bits)
		for (size_t i = 0; i + 3 < rawData.size(); i += 4) {
			std::int32_t v = (std::int32_t)((std::uint32_t)rawData[i] |
				((std::uint32_t)rawData[i + 1] << 8) |
				((std::uint32_t)rawData[i + 2] << 16) |
				((std::uint32_t)rawData[i + 3] << 24));
			v >>= 16;
			if (v > 32767) v = 32767;
			if (v < -32768) v = -32768;
			push_i16_le((std::int16_t)v);
		}
		wav.pcm = std::move(out);
		wav.bitsPerSample = 16;
		return true;
	}

	return false;
}

static ALenum to_al_format(const WavData& wav) {
	if (wav.channels == 1 && wav.bitsPerSample == 8) return AL_FORMAT_MONO8;
	if (wav.channels == 1 && wav.bitsPerSample == 16) return AL_FORMAT_MONO16;
	if (wav.channels == 2 && wav.bitsPerSample == 8) return AL_FORMAT_STEREO8;
	if (wav.channels == 2 && wav.bitsPerSample == 16) return AL_FORMAT_STEREO16;
	return 0;
}

static ALuint get_buffer(const std::string& soundPath) {
	if (g_missingSounds.find(soundPath) != g_missingSounds.end())
		return 0;

	auto it = g_bufferCache.find(soundPath);
	if (it != g_bufferCache.end()) return it->second;

	WavData wav;
	if (!load_wav_file(soundPath, wav)) {
		std::cerr << "[audio] Failed to load WAV: " << soundPath << "\n";
		g_missingSounds.insert(soundPath);
		return 0;
	}

	ALenum format = to_al_format(wav);
	if (format == 0) {
		std::cerr << "[audio] Unsupported WAV format: " << soundPath << "\n";
		g_missingSounds.insert(soundPath);
		return 0;
	}

	ALuint buffer = 0;
	alGenBuffers(1, &buffer);
	alBufferData(buffer, format, wav.pcm.data(), (ALsizei)wav.pcm.size(), (ALsizei)wav.sampleRate);

	ALenum err = alGetError();
	if (err != AL_NO_ERROR) {
		std::cerr << "[audio] OpenAL error loading buffer: " << soundPath << "\n";
		if (buffer) alDeleteBuffers(1, &buffer);
		g_missingSounds.insert(soundPath);
		return 0;
	}

	g_bufferCache.emplace(soundPath, buffer);
	return buffer;
}

static ALuint acquire_source() {
	for (ALuint src : g_sources) {
		ALint state = 0;
		alGetSourcei(src, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING) return src;
	}

	ALuint src = 0;
	alGenSources(1, &src);
	if (!src) return 0;
	g_sources.push_back(src);
	return src;
}

static ALuint channel_source(Channel channel) {
	switch (channel) {
	case Channel::UI: return g_uiSource;
	case Channel::STEP: return g_stepSource;
	case Channel::ACTION: return g_actionSource;
	default: return g_actionSource;
	}
}
#endif

} // namespace

bool init() {
#ifdef USE_OPENAL
	if (g_inited) return true;

	g_device = alcOpenDevice(nullptr);
	if (!g_device) {
		std::cerr << "[audio] alcOpenDevice failed\n";
		return false;
	}
	g_context = alcCreateContext(g_device, nullptr);
	if (!g_context) {
		std::cerr << "[audio] alcCreateContext failed\n";
		alcCloseDevice(g_device);
		g_device = nullptr;
		return false;
	}
	if (!alcMakeContextCurrent(g_context)) {
		std::cerr << "[audio] alcMakeContextCurrent failed\n";
		alcDestroyContext(g_context);
		alcCloseDevice(g_device);
		g_context = nullptr;
		g_device = nullptr;
		return false;
	}

	// Reasonable defaults for 3D.
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	alListenerf(AL_GAIN, 1.0f);

	// Dedicated channels so common sounds don't overlap.
	alGenSources(1, &g_uiSource);
	alGenSources(1, &g_stepSource);
	alGenSources(1, &g_actionSource);
	g_sources.push_back(g_uiSource);
	g_sources.push_back(g_stepSource);
	g_sources.push_back(g_actionSource);

	g_inited = true;
	return true;
#else
	return false;
#endif
}

void shutdown() {
#ifdef USE_OPENAL
	if (!g_inited) return;

	for (ALuint src : g_sources) {
		alSourceStop(src);
		alDeleteSources(1, &src);
	}
	g_sources.clear();
	g_uiSource = 0;
	g_stepSource = 0;
	g_actionSource = 0;

	for (auto& kv : g_bufferCache) {
		ALuint buf = kv.second;
		if (buf) alDeleteBuffers(1, &buf);
	}
	g_bufferCache.clear();
	g_missingSounds.clear();

	alcMakeContextCurrent(nullptr);
	if (g_context) alcDestroyContext(g_context);
	if (g_device) alcCloseDevice(g_device);
	g_context = nullptr;
	g_device = nullptr;
	g_inited = false;
#endif
}

void preload_defaults() {
#ifdef USE_OPENAL
	if (!g_inited) return;
	(void)get_buffer("data/sfx/ui_click.wav");
	(void)get_buffer("data/sfx/enter.wav");
	(void)get_buffer("data/sfx/disassemble.wav");
	(void)get_buffer("data/sfx/assemble.wav");
	(void)get_buffer("data/sfx/step.wav");
#endif
}

void update_listener(Vec3 position, Vec3 forward, Vec3 up) {
#ifdef USE_OPENAL
	if (!g_inited) return;

	g_listenerPos = position;
	Vec3 f = normalize(forward);
	Vec3 u = normalize(up);

	alListener3f(AL_POSITION, position.x, position.y, position.z);
	float ori[6] = {f.x, f.y, f.z, u.x, u.y, u.z};
	alListenerfv(AL_ORIENTATION, ori);
#endif
}

void play3d(const std::string& soundPath, Vec3 position, float gain, Channel channel) {
#ifdef USE_OPENAL
	if (!g_inited) return;

	ALuint buffer = get_buffer(soundPath);
	if (!buffer) return;

	ALuint src = channel_source(channel);
	if (!src) return;

	alSourceStop(src);
	alSourcei(src, AL_BUFFER, (ALint)buffer);
	alSource3f(src, AL_POSITION, position.x, position.y, position.z);
	alSource3f(src, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
	alSourcef(src, AL_GAIN, gain);
	alSourcef(src, AL_PITCH, 1.0f);
	alSourcei(src, AL_LOOPING, AL_FALSE);

	// Make sure it's treated as positional.
	alSourcei(src, AL_SOURCE_RELATIVE, AL_FALSE);
	alSourcef(src, AL_REFERENCE_DISTANCE, 2.0f);
	alSourcef(src, AL_MAX_DISTANCE, 50.0f);
	alSourcef(src, AL_ROLLOFF_FACTOR, 1.0f);

	alSourcePlay(src);
#endif
}

void play_ui(const std::string& soundPath, float gain) {
#ifdef USE_OPENAL
	if (!g_inited) return;
	ALuint buffer = get_buffer(soundPath);
	if (!buffer) return;
	ALuint src = channel_source(Channel::UI);
	if (!src) return;

	alSourceStop(src);
	alSourcei(src, AL_BUFFER, (ALint)buffer);
	alSourcef(src, AL_GAIN, gain);
	alSourcef(src, AL_PITCH, 1.0f);
	alSourcei(src, AL_LOOPING, AL_FALSE);

	// UI sounds: not spatial (relative to listener).
	alSourcei(src, AL_SOURCE_RELATIVE, AL_TRUE);
	alSource3f(src, AL_POSITION, 0.0f, 0.0f, 0.0f);
	alSource3f(src, AL_VELOCITY, 0.0f, 0.0f, 0.0f);

	alSourcePlay(src);
#endif
}

void play_step(const std::string& soundPath, float gain) {
#ifdef USE_OPENAL
	play3d(soundPath, g_listenerPos, gain, Channel::STEP);
#endif
}

void stop(Channel channel) {
#ifdef USE_OPENAL
	if (!g_inited) return;
	ALuint src = channel_source(channel);
	if (src) alSourceStop(src);
#endif
}

void stop_all() {
#ifdef USE_OPENAL
	if (!g_inited) return;
	for (ALuint src : g_sources) {
		if (src) alSourceStop(src);
	}
#endif
}

} // namespace audio
