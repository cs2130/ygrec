#include "../stdafx.h"
#include "ObsAudioRecord.h"

#define SIMPLE_ARCHIVE_NAME		"simple_archive_audio"
#define INPUT_AUDIO_SOURCE		"wasapi_input_capture"
#define OUTPUT_AUDIO_SOURCE		"wasapi_output_capture"

CObsAudioRecord::CObsAudioRecord()
{
	this->InitData();
}

CObsAudioRecord::~CObsAudioRecord()
{

}

bool CObsAudioRecord::InitAudio()
{
	this->InitData();
	return true;
}

bool  CObsAudioRecord::CreateAudioEncoder()
{
	return this->CreateSimpleAACEncoder(
		this->m_pAudioArchive,
		this->GetAudioBitrate(this->m_obsAudioEncoders),
		SIMPLE_ARCHIVE_NAME, 1);
}

bool CObsAudioRecord::SetAudioParam(uint32_t nSamplesPerSec)
{
	obs_audio_info2 ai = {};
	ai.samples_per_sec = nSamplesPerSec;
	ai.speakers = SPEAKERS_STEREO;
	return obs_reset_audio2(&ai);
}

bool CObsAudioRecord::LoadAudioRecordingPreset()
{
	bool isRet = false;
	isRet = this->CreateSimpleAACEncoder(
		m_pAudioRecording, 192, "simple_aac_recording", 0);
	if (!isRet)
		throw "Failed to create audio recording encoder "
		"(simple output)";
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		char name[30] = { 0 };
		snprintf(name, sizeof name,
			"simple_aac_recording%d", i);
		isRet = this->CreateSimpleAACEncoder(
			m_pAudioTrack[i], GetAudioBitrate(m_obsAudioEncoders), name,
			i);
		if (!isRet)
			throw "Failed to create multi-track audio recording encoder "
			"(simple output)";
	}
	return isRet;
}

bool CObsAudioRecord::SetAudioMonitoringDevice(
	const wchar_t* szDeviceName, const wchar_t* szDeviceId)
{
	if (obs_audio_monitoring_available()) {
		string strDeviceName = CUtilsConv::WCharToUtf8(szDeviceName);
		string strDeviceId = CUtilsConv::WCharToUtf8(szDeviceId);
		obs_set_audio_monitoring_device(strDeviceName.c_str(), strDeviceId.c_str());
	}
	return true;
}

void CObsAudioRecord::UpdateAudioRecordingSettings(long llBitrate)
{
	int nBitrate = (int)((llBitrate * 1.0) / 1000);
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_int(settings, "bitrate", nBitrate);
	obs_data_set_string(settings, "rate_control", "CBR");

	int tracks = 1;
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		if ((tracks & (1 << i)) != 0) {
			obs_encoder_update(m_pAudioTrack[i], settings);
		}
	}
}

void CObsAudioRecord::UpdateAudioBitrateSettings(long llBitrate)
{
	int nBitrate = (int)((llBitrate * 1.0) / 1000);
	OBSDataAutoRelease settings = obs_data_create();

	obs_data_set_string(settings, "rate_control", "CBR");
	obs_data_set_int(settings, "bitrate", nBitrate);
	obs_encoder_update(m_pAudioArchive, settings);
}

void CObsAudioRecord::SetAudioEncoder()
{
	obs_encoder_set_audio(m_pAudioArchive, obs_get_audio());
}

void CObsAudioRecord::UpdateAudioEncoderTrack()
{
	int nTracks = 1;
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		if ((nTracks & (1 << i)) != 0) {
			obs_encoder_set_audio(
				m_pAudioTrack[i],
				obs_get_audio());
		}
	}
}

void CObsAudioRecord::UpdateAudioOutPutTrack(obs_output_t* pOutPut)
{
	int nIdx = 0;
	int nTracks = 1;
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		if ((nTracks & (1 << i)) != 0) {
			obs_output_set_audio_encoder(
				pOutPut, m_pAudioTrack[i],
				nIdx++);
		}
	}
}

bool CObsAudioRecord::AddMic(const char* deviceId, bool isMute)
{
	bool isRet = false; //"default"
	if (IsHasAudioDevices(INPUT_AUDIO_SOURCE)) {
		isRet = ResetAudioDevice(INPUT_AUDIO_SOURCE, deviceId,
			OBS_TAG " Default Mic/Aux",
			SOURCE_CHANNEL_AUDIO_INPUT, isMute);
	}
	if (!isRet || isMute) {
		m_pIsAudioMic.SetAtomic(false);
	} else {
		m_pIsAudioMic.SetAtomic(true);
	}
	return isRet;
}

bool CObsAudioRecord::AddSpeaker(const char* deviceId, bool isMute)
{
	bool isRet = false; //"default"
	if (this->IsHasAudioDevices(OUTPUT_AUDIO_SOURCE)) {
		isRet = ResetAudioDevice(OUTPUT_AUDIO_SOURCE, deviceId,
			OBS_TAG " Default Desktop Audio",
			SOURCE_CHANNEL_AUDIO_OUTPUT, isMute);
	}
	if (!isRet || isMute) {
		m_pIsAudioSpeaker.SetAtomic(false);
	} else {
		m_pIsAudioSpeaker.SetAtomic(true);
	}
	return isRet;
}

bool CObsAudioRecord::IsSpeaker()
{
	return m_pIsAudioSpeaker.GetAtomic();
}

bool CObsAudioRecord::IsMic()
{
	return m_pIsAudioMic.GetAtomic();
}

bool CObsAudioRecord::MuteApeaker(const char* deviceId, bool isMute)
{
	bool isRet = false;
	obs_source_t* source = nullptr;
	if (!this->IsHasAudioDevices(OUTPUT_AUDIO_SOURCE)) {
		goto mute_end;
	}
	isRet = true;
	source = obs_get_output_source(SOURCE_CHANNEL_AUDIO_OUTPUT);
	if (!source)
	{
		isRet = ResetAudioDevice(OUTPUT_AUDIO_SOURCE, deviceId,
			OBS_TAG " Default Desktop Audio",
			SOURCE_CHANNEL_AUDIO_OUTPUT, isMute);
	}
	else if(obs_source_muted(source) != isMute) {
		obs_source_set_muted(source, isMute);
	}
mute_end:
	if (!isRet || isMute) {
		m_pIsAudioSpeaker.SetAtomic(false);
	} else {
		m_pIsAudioSpeaker.SetAtomic(true);
	}
	return isRet;
}

bool CObsAudioRecord::MuteMic(const char* deviceId, bool isMute)
{
	bool isRet = false;
	obs_source_t* source = nullptr;
	if (!this->IsHasAudioDevices(INPUT_AUDIO_SOURCE)) {
		goto mute_end;
	}
	isRet = true;
	source = obs_get_output_source(SOURCE_CHANNEL_AUDIO_INPUT);
	if (!source) 
	{
		isRet = ResetAudioDevice(INPUT_AUDIO_SOURCE, deviceId,
			OBS_TAG " Default Mic/Aux",
			SOURCE_CHANNEL_AUDIO_INPUT, isMute);
	} else if (obs_source_muted(source) != isMute) {
		obs_source_set_muted(source, isMute);
	}
mute_end:
	if (!isRet || isMute) {
		m_pIsAudioMic.SetAtomic(false);
	} else {
		m_pIsAudioMic.SetAtomic(true);
	}
	return isRet;
}

void CObsAudioRecord::StopAudio()
{

}

void CObsAudioRecord::DisplayAudio()
{
	//for (size_t i = 0; i < MAX_AUDIO_MIXES; ++i)
	//	m_pAudioTrack[i] = nullptr;
}

void CObsAudioRecord::InitData()
{
	m_pIsAudioMic.InitAtomic(false);
	m_pIsAudioSpeaker.InitAtomic(false);
	this->m_pAudioRecording = nullptr;
	this->m_pAudioArchive = nullptr;
	for (size_t i = 0; i < MAX_AUDIO_MIXES; ++i)
		this->m_pAudioTrack[i] = nullptr;
}

bool CObsAudioRecord::CreateSimpleAACEncoder(OBSEncoder& res, int bitrate,
	const char* name, size_t idx)
{
	const char* id_ = m_obsAudioEncoders.GetSimpleAACEncoderForBitrate(bitrate);
	if (!id_) {
		blog(LOG_ERROR, "set audio bitrate fail %d", __LINE__);
		res = nullptr;
		return false;
	}

	res = obs_audio_encoder_create(id_, name, nullptr, idx, nullptr);

	if (res) {
		obs_encoder_release(res);
		return true;
	}
	blog(LOG_ERROR, "set audio bitrate fail %s,%s,%d", name, id_, __LINE__);
	return false;
}

int CObsAudioRecord::GetAudioBitrate(CObsAudioEncoders& obsAudioEncoders) const
{
	int bitrate = 160;
	return obsAudioEncoders.FindClosestAvailableSimpleAACBitrate(bitrate);
}

bool CObsAudioRecord::IsHasAudioDevices(const char* source_id)
{
	const char* output_id = source_id;
	obs_properties_t* props = obs_get_source_properties(output_id);
	size_t count = 0;

	if (!props)
		return false;

	obs_property_t* devices = obs_properties_get(props, "device_id");
	if (devices)
		count = obs_property_list_item_count(devices);

	obs_properties_destroy(props);

	return count != 0;
}

bool CObsAudioRecord::ResetAudioDevice(const char* sourceId, const char* deviceId,
	const char* deviceDesc, int channel, bool isMute)
{
	bool isRet = false;
	bool disable = deviceId && (strcmp(deviceId, "disabled") == 0);
	obs_source_t* source;
	obs_data_t* settings;
	
	source = obs_get_output_source(channel);
	if (source) {
		if (disable) {
			obs_set_output_source(channel, nullptr);
		} else {
			settings = obs_source_get_settings(source);
			const char* oldId = obs_data_get_string(settings,
				"device_id");
			if (deviceId && strcmp(oldId, deviceId) != 0) {
				obs_data_set_string(settings, "device_id", deviceId);
			}
			obs_data_set_bool(settings, "use_device_timing", false);
			obs_source_update(source, settings);
			obs_data_release(settings);
		}

	} else if (!disable) {
		settings = obs_data_create();
		obs_data_set_string(settings, "device_id", deviceId);
		obs_data_set_bool(settings, "use_device_timing", false);
		source = obs_source_create(sourceId, deviceDesc, settings, nullptr);
		obs_data_release(settings);
		if (source) {
			obs_set_output_source(channel, source);
		}
	}
	isRet = ((source != nullptr) ? true : false);
	if (isRet && isMute) {
		obs_source_set_muted(source, isMute);
	}
	if (isRet) {
		obs_source_release(source);
	}
	return isRet;
}

//bool CObsAudioRecord::IsSourceUnassigned(obs_source_t* source)
//{
//	return (obs_source_get_audio_mixers(source) &
//		((1 << MAX_AUDIO_MIXES) - 1)) == 0;
//}

void CObsAudioRecord::AddFilterToAudioInput(const char* id)
{
	if (id == nullptr || *id == '\0')
		return;

	obs_source_t* source = obs_get_output_source(SOURCE_CHANNEL_AUDIO_INPUT);
	if (!source)
		return;

	obs_source_t* existing_filter;
	std::string name = obs_source_get_display_name(id);
	if (name.empty())
		name = id;
	existing_filter = obs_source_get_filter_by_name(source, name.c_str());
	if (existing_filter) {
		obs_source_release(existing_filter);
		obs_source_release(source);
		return;
	}

	obs_source_t* filter = obs_source_create(id, name.c_str(), nullptr, nullptr);
	if (filter) {
		const char* sourceName = obs_source_get_name(source);
		blog(LOG_INFO, "added filter '%s' (%s) to source '%s' %d,", name.c_str(),
			id, sourceName, __LINE__);
		obs_source_filter_add(source, filter);
		obs_source_release(filter);
	}

	obs_source_release(source);
}