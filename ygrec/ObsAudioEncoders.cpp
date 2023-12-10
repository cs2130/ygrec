#include "../stdafx.h"
#include "ObsAudioEncoders.h"

#define INVALID_BITRATE 10000

CObsAudioEncoders::CObsAudioEncoders()
{

}

CObsAudioEncoders::~CObsAudioEncoders()
{

}

const char* CObsAudioEncoders::NullToEmpty(const char* str)
{
	return str ? str : "";
}

const char* CObsAudioEncoders::EncoderName(const std::string& id)
{
	return NullToEmpty(obs_encoder_get_display_name(id.c_str()));
}

void CObsAudioEncoders::HandleIntProperty(obs_property_t* prop, std::vector<int>& bitrates)
{
	const int max_ = obs_property_int_max(prop);
	const int step = obs_property_int_step(prop);

	for (int i = obs_property_int_min(prop); i <= max_; i += step)
		bitrates.push_back(i);
}

void CObsAudioEncoders::HandleListProperty(obs_property_t* prop, const char* id,
	std::vector<int>& bitrates)
{
	obs_combo_format format = obs_property_list_format(prop);
	if (format != OBS_COMBO_FORMAT_INT) {
		blog(LOG_ERROR,
			"Encoder '%s' (%s) returned bitrate "
			"PROPERTY_LIST property of unhandled "
			"format %d, %d",
			EncoderName(id), id, static_cast<int>(format), __LINE__);
		return;
	}

	const size_t count = obs_property_list_item_count(prop);
	for (size_t i = 0; i < count; i++) {
		if (obs_property_list_item_disabled(prop, i))
			continue;

		int bitrate =
			static_cast<int>(obs_property_list_item_int(prop, i));
		bitrates.push_back(bitrate);
	}
}

void CObsAudioEncoders::HandleSampleRate(obs_property_t* prop, const char* id)
{
	auto ReleaseData = [](obs_data_t* data) { obs_data_release(data); };
	std::unique_ptr<obs_data_t, decltype(ReleaseData)> data{
		obs_encoder_defaults(id), ReleaseData };

	if (!data) {
		blog(LOG_ERROR,
			"Failed to get defaults for encoder '%s' (%s) "
			"while populating bitrate map %d",
			EncoderName(id), id, __LINE__);
		return;
	}

	uint32_t sampleRate = 44100;

	obs_data_set_int(data.get(), "samplerate", sampleRate);

	obs_property_modified(prop, data.get());
}

void CObsAudioEncoders::HandleEncoderProperties(const char* id, std::vector<int>& bitrates)
{
	auto DestroyProperties = [](obs_properties_t* props) {
		obs_properties_destroy(props);
	};
	std::unique_ptr<obs_properties_t, decltype(DestroyProperties)> props{
		obs_get_encoder_properties(id), DestroyProperties };

	if (!props) {
		blog(LOG_ERROR,
			"Failed to get properties for encoder "
			"'%s' (%s) %d",
			EncoderName(id), id, __LINE__);
		return;
	}

	obs_property_t* samplerate =
		obs_properties_get(props.get(), "samplerate");
	if (samplerate)
		HandleSampleRate(samplerate, id);

	obs_property_t* bitrate = obs_properties_get(props.get(), "bitrate");

	obs_property_type type = obs_property_get_type(bitrate);
	switch (type) {
	case OBS_PROPERTY_INT:
		return HandleIntProperty(bitrate, bitrates);

	case OBS_PROPERTY_LIST:
		return HandleListProperty(bitrate, id, bitrates);

	default:
		break;
	}

	blog(LOG_INFO,
		"Encoder '%s' (%s) returned bitrate property "
		"of unhandled type %d",
		EncoderName(id), id, static_cast<int>(type));
}

const char* CObsAudioEncoders::GetCodec(const char* id)
{
	return NullToEmpty(obs_get_encoder_codec(id));
}

void CObsAudioEncoders::PopulateBitrateLists()
{
	once_flag once;
	call_once(once, [this]() {
		struct obs_audio_info aoi;
		obs_get_audio_info(&aoi);

		/* NOTE: ffmpeg_aac and ffmpeg_opus have the same properties
		 * their bitrates will also be used as a fallback */
		HandleEncoderProperties("ffmpeg_aac", m_fallbackBitrates);

		if (m_fallbackBitrates.empty())
			blog(LOG_ERROR, "Could not enumerate fallback encoder "
				"bitrates %d", __LINE__);

		//ostringstream ss;
		//for (auto& bitrate : m_fallbackBitrates)
		//	ss << "\n	" << setw(3) << bitrate << " kbit/s:";

		//blog(LOG_DEBUG, "Fallback encoder bitrates:%s",
		//	ss.str().c_str());

		const char* id = nullptr;
		for (size_t i = 0; obs_enum_encoder_types(i, &id); i++) {
			if (obs_get_encoder_type(id) != OBS_ENCODER_AUDIO)
				continue;

			if (strcmp(id, "ffmpeg_aac") == 0 ||
				strcmp(id, "ffmpeg_opus") == 0)
				continue;

			std::string encoder = id;

			HandleEncoderProperties(id, m_encoderBitrates[encoder]);

			if (m_encoderBitrates[encoder].empty())
				blog(LOG_ERROR,
					"Could not enumerate %s encoder "
					"bitrates, %d", id, __LINE__);

			//ostringstream ss;
			//for (auto& bitrate : m_encoderBitrates[encoder])
			//	ss << "\n	" << setw(3) << bitrate
			//	<< " kbit/s";

			//blog(LOG_DEBUG, "%s (%s) encoder bitrates:%s",
			//	EncoderName(id), id, ss.str().c_str());
		}

		if (m_encoderBitrates.empty() && m_fallbackBitrates.empty())
			blog(LOG_ERROR, "Could not enumerate any audio encoder "
				"bitrates, %d", __LINE__);
	});
}

void CObsAudioEncoders::PopulateSimpleAACBitrateMap()
{
	PopulateBitrateLists();

	static once_flag once;

	call_once(once, [this]() {
		const string encoders[] = {
			"ffmpeg_aac",
			"libfdk_aac",
			"CoreAudio_AAC",
		};

		const string fallbackEncoder = encoders[0];

		struct obs_audio_info aoi;
		obs_get_audio_info(&aoi);

		for (auto& bitrate : m_fallbackBitrates)
			m_simpleAACBitrateMap[bitrate] = fallbackEncoder;

		const char* id = nullptr;
		for (size_t i = 0; obs_enum_encoder_types(i, &id); i++) {
			auto Compare = [=](const string& val) {
				return val == NullToEmpty(id);
			};

			if (find_if(begin(encoders), end(encoders), Compare) !=
				end(encoders))
				continue;

			if (strcmp(GetCodec(id), "aac") != 0)
				continue;

			std::string encoder = id;
			if (m_encoderBitrates[encoder].empty())
				continue;

			for (auto& bitrate : m_encoderBitrates[encoder])
				m_simpleAACBitrateMap[bitrate] = encoder;
		}

		for (auto& encoder : encoders) {
			if (encoder == fallbackEncoder)
				continue;

			if (strcmp(GetCodec(encoder.c_str()), "aac") != 0)
				continue;

			for (auto& bitrate : m_encoderBitrates[encoder])
				m_simpleAACBitrateMap[bitrate] = encoder;
		}

		if (m_simpleAACBitrateMap.empty()) {
			blog(LOG_ERROR, "Could not enumerate any AAC encoder "
				"bitrates %d", __LINE__);
			return;
		}

		//ostringstream ss;
		//for (auto& entry : m_simpleAACBitrateMap)
		//	ss << "\n	" << setw(3) << entry.first
		//	<< " kbit/s: '" << EncoderName(entry.second) << "' ("
		//	<< entry.second << ')';

		//blog(LOG_DEBUG, "AAC simple encoder bitrate mapping:%s",
		//	ss.str().c_str());
	});
}

void CObsAudioEncoders::PopulateSimpleOpusBitrateMap()
{
	PopulateBitrateLists();

	once_flag once;

	call_once(once, [this]() {
		struct obs_audio_info aoi;
		obs_get_audio_info(&aoi);

		for (auto& bitrate : m_fallbackBitrates)
			m_simpleOpusBitrateMap[bitrate] = "ffmpeg_opus";

		const char* id = nullptr;
		for (size_t i = 0; obs_enum_encoder_types(i, &id); i++) {
			if (strcmp(GetCodec(id), "opus") != 0)
				continue;

			std::string encoder = id;
			if (m_encoderBitrates[encoder].empty())
				continue;

			for (auto& bitrate : m_encoderBitrates[encoder])
				m_simpleOpusBitrateMap[bitrate] = encoder;
		}

		if (m_simpleOpusBitrateMap.empty()) {
			blog(LOG_ERROR, "Could not enumerate any Opus encoder "
				"bitrates %d", __LINE__);
			return;
		}

		//ostringstream ss;
		//for (auto& entry : m_simpleOpusBitrateMap)
		//	ss << "\n	" << setw(3) << entry.first
		//	<< " kbit/s: '" << EncoderName(entry.second) << "' ("
		//	<< entry.second << ')';

		//blog(LOG_DEBUG, "Opus simple encoder bitrate mapping:%s",
		//	ss.str().c_str());
	});
}

const map<int, std::string>& CObsAudioEncoders::GetSimpleAACEncoderBitrateMap()
{
	this->PopulateSimpleAACBitrateMap();
	return m_simpleAACBitrateMap;
}

const map<int, std::string>& CObsAudioEncoders::GetSimpleOpusEncoderBitrateMap()
{
	PopulateSimpleOpusBitrateMap();
	return m_simpleOpusBitrateMap;
}

const char* CObsAudioEncoders::GetSimpleAACEncoderForBitrate(int bitrate)
{
	auto& map_ = GetSimpleAACEncoderBitrateMap();
	auto res = map_.find(bitrate);
	if (res == end(map_))
		return NULL;
	return res->second.c_str();
}

const char* CObsAudioEncoders::GetSimpleOpusEncoderForBitrate(int bitrate)
{
	auto& map_ = GetSimpleOpusEncoderBitrateMap();
	auto res = map_.find(bitrate);
	if (res == end(map_))
		return NULL;
	return res->second.c_str();
}

int CObsAudioEncoders::FindClosestAvailableSimpleBitrate(int bitrate,
	const map<int, std::string>& map)
{
	int prev = 0;
	int next = INVALID_BITRATE;

	for (auto val : map) {
		if (next > val.first) {
			if (val.first == bitrate)
				return bitrate;

			if (val.first < next && val.first > bitrate)
				next = val.first;
			if (val.first > prev && val.first < bitrate)
				prev = val.first;
		}
	}

	if (next != INVALID_BITRATE)
		return next;
	if (prev != 0)
		return prev;
	return 192;
}

int CObsAudioEncoders::FindClosestAvailableSimpleAACBitrate(int bitrate)
{
	return FindClosestAvailableSimpleBitrate(
		bitrate, GetSimpleAACEncoderBitrateMap());
}

int CObsAudioEncoders::FindClosestAvailableSimpleOpusBitrate(int bitrate)
{
	return FindClosestAvailableSimpleBitrate(
		bitrate, GetSimpleOpusEncoderBitrateMap());
}

const std::vector<int>& CObsAudioEncoders::GetAudioEncoderBitrates(const char* id)
{
	std::string encoder = id;
	PopulateBitrateLists();
	if (m_encoderBitrates[encoder].empty())
		return m_fallbackBitrates;
	return m_encoderBitrates[encoder];
}

int CObsAudioEncoders::FindClosestAvailableAudioBitrate(const char* id, int bitrate)
{
	int prev = 0;
	int next = INVALID_BITRATE;
	std::string encoder = id;

	for (auto val : m_encoderBitrates[encoder].empty()
		? m_fallbackBitrates
		: m_encoderBitrates[encoder]) {
		if (next > val) {
			if (val == bitrate)
				return bitrate;

			if (val < next && val > bitrate)
				next = val;
			if (val > prev && val < bitrate)
				prev = val;
		}
	}

	if (next != INVALID_BITRATE)
		return next;
	if (prev != 0)
		return prev;
	return 192;
}