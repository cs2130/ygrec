#ifndef __OBS_AUDIORECORD_H__
#define __OBS_AUDIORECORD_H__
#pragma once

class CObsAudioRecord
{
public:
	CObsAudioRecord();
	~CObsAudioRecord();

public:
	bool InitAudio();
	bool SetAudioParam(uint32_t nSamplesPerSec = 44100);
	bool CreateAudioEncoder();
	bool LoadAudioRecordingPreset();
	bool SetAudioMonitoringDevice(const wchar_t* szDeviceName, const wchar_t* szDeviceId);
	void UpdateAudioRecordingSettings(long llBitrate);
	void UpdateAudioBitrateSettings(long llBitrate);
	void SetAudioEncoder();
	void UpdateAudioEncoderTrack();
	void UpdateAudioOutPutTrack(obs_output_t*);
	void AddFilterToAudioInput(const char* id);
	bool AddMic(const char* deviceId, bool isMute = false);
	bool AddSpeaker(const char* deviceId, bool isMute = false);
	bool MuteApeaker(const char* deviceId, bool isMute);
	bool MuteMic(const char* deviceId, bool isMute);
	bool IsSpeaker();
	bool IsMic();
	void StopAudio();
	void DisplayAudio();

private:
	void InitData();
	bool CreateSimpleAACEncoder(OBSEncoder& res, int bitrate,
		const char* name, size_t idx);
	int GetAudioBitrate(CObsAudioEncoders& obsAudioEncoders) const;
	bool IsHasAudioDevices(const char* source_id);
	bool ResetAudioDevice(const char* sourceId, const char* deviceId,
		const char* deviceDesc, int channel, bool isMute);

private:
	CObsAudioEncoders		m_obsAudioEncoders;
	OBSEncoder				m_pAudioRecording;
	OBSEncoder				m_pAudioArchive;
	OBSEncoder				m_pAudioTrack[MAX_AUDIO_MIXES];
	CUtilsAtomic<bool>		m_pIsAudioMic;
	CUtilsAtomic<bool>		m_pIsAudioSpeaker;
};

#endif

