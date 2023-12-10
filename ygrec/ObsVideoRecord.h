#ifndef __OBS_VIDEORECORD_H__
#define __OBS_VIDEORECORD_H__
#pragma once

class CObsVideoRecord
{
public:
	CObsVideoRecord();
	~CObsVideoRecord();

public:
	bool InitVideo();
	bool SetVideoParam(ObsVideoSet videoSet, SIZE szIn, SIZE szOut, bool isFirst = false);
	void LoadVideoRecordingPreset(string strVideoEncoder = SIMPLE_ENCODER_X264);
	void UpdateVideoRecordingSettingsX264Crf(
		ushort usFps, RectX recArea, QualityType enQuality, int nCrfValue,
		int nVideoBitrateType, int nVideoBitrate, VideoEnCode nVideoEnCode);
	void UpdateVideoFps(ushort usFps);
	void SetVideoEncoder();
	void SetVideoFps(uint usFps);
	void SetVideoReInSize(SIZE sz);
	void SetVideoReInSize(SIZE sz, uint usFps);
	SIZE GetVideoSize();
	void ReductionVideoFps();
	void UpdateVideoOutPut(obs_output_t*);
	void StopVideo();
	void DisplayVideo();

private:
	void InitData();
	int CalcCRF(RectX recArea, int crf);
	void LoadVideoRecordingPresetLossy(const char* encoderId);
	const char* GetAimpleOutputEncoder(const char* encoder);
	bool EncoderAvailable(const char* encoder);
	video_format GetVideoFormat(int nWidth);

private:
	uint32_t						m_firstEncoded;
	uint32_t						m_firstSkipped;
	uint32_t						m_firstRendered;
	uint32_t						m_firstLagged;
	bool							m_lowCPUx264;

	OBSEncoder						m_pVideoRecording;
	std::shared_ptr<obs_video_info> m_pObsVideoInfo;

private:
	GraphicsModule					m_graphicsModule;
};
#endif

