#ifndef __OBS_AUDIOENCODERS_H__
#define __OBS_AUDIOENCODERS_H__
#pragma once

class CObsAudioEncoders
{
public:
	CObsAudioEncoders();
	~CObsAudioEncoders();

public:
	const std::map<int, std::string>&	GetSimpleAACEncoderBitrateMap();
	const char*							GetSimpleAACEncoderForBitrate(int bitrate);
	int									FindClosestAvailableSimpleAACBitrate(int bitrate);

	const std::map<int, std::string>&	GetSimpleOpusEncoderBitrateMap();
	const char*							GetSimpleOpusEncoderForBitrate(int bitrate);
	int									FindClosestAvailableSimpleOpusBitrate(int bitrate);

	const std::vector<int>&				GetAudioEncoderBitrates(const char* id);
	int									FindClosestAvailableAudioBitrate(const char* id, int bitrate);

private:
	const char* NullToEmpty(const char* str);
	const char* EncoderName(const std::string& id);
	void HandleIntProperty(obs_property_t* prop, std::vector<int>& bitrates);
	void HandleListProperty(obs_property_t* prop, const char* id,
		std::vector<int>& bitrates);
	void HandleSampleRate(obs_property_t* prop, const char* id);
	void HandleEncoderProperties(const char* id, std::vector<int>& bitrates);
	const char* GetCodec(const char* id);
	void PopulateBitrateLists();
	void PopulateSimpleAACBitrateMap();
	void PopulateSimpleOpusBitrateMap();
	int FindClosestAvailableSimpleBitrate(int bitrate,
		const map<int, std::string>& map);

private:
	map<int, std::string>				m_simpleOpusBitrateMap;
	map<int, std::string>				m_simpleAACBitrateMap;
	std::vector<int>					m_fallbackBitrates;
	map<std::string, std::vector<int>>	m_encoderBitrates;

};

#endif // !__RESHELPER_H__