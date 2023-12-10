#include "../stdafx.h"
#include "ObsVideoRecord.h"

#define DL_D3D11						"libd3d11.dll"
#define DL_OPENGL						"libopengl.dll"
#define CROSS_DIST_CUTOFF				2000.0


CObsVideoRecord::CObsVideoRecord()
{
	this->InitData();
}

CObsVideoRecord::~CObsVideoRecord()
{

}

bool CObsVideoRecord::InitVideo()
{
	bool isRet = true;
	this->InitData();
	this->m_pObsVideoInfo = std::make_shared<obs_video_info>();
	if (!m_pObsVideoInfo) {
		isRet = false;
	}
	return isRet;
}

video_format CObsVideoRecord::GetVideoFormat(int nWidth)
{
	video_format fmtRet = VIDEO_FORMAT_I420;
	if (!this->m_pObsVideoInfo->graphics_module) {
		return fmtRet;
	}
	if (strcmp(this->m_pObsVideoInfo->graphics_module, DL_D3D11) == 0) {
		return fmtRet;
	}

	if ((nWidth % 8) != 0) {
		fmtRet = VIDEO_FORMAT_BGRA;
	}
	return fmtRet;
}

bool CObsVideoRecord::SetVideoParam(ObsVideoSet videoSet, SIZE szIn, SIZE szOut, bool isFirst)
{
	int nRes = 0;
	bool isRet = false;
	int nInWidth = 0;
	int nInHeight = 0;
	int nOutWidth = 0;
	int nOutHeight = 0;

	nInWidth = szIn.cx;
	nInHeight = szIn.cy;
	nOutWidth = szOut.cx;
	nOutHeight = szOut.cy;

	if ((nInWidth != nOutWidth) || (nInHeight != nOutHeight)) {
		nOutWidth = static_cast<int>((nInWidth * nOutHeight * 1.0) / nInHeight);
		CUtilsExt::GetRecSize(&nOutWidth, &nOutHeight);
	}

	uint32_t _fps_num = videoSet.usFps;
	uint32_t _fps_den = 1;
	if (_fps_num == 29) {
		_fps_num = 30000;
		_fps_den = 1001;
	}
	if (isFirst) {
		this->m_graphicsModule = GM_DL_D3D11;
		this->m_pObsVideoInfo->fps_num = 1;
		this->m_pObsVideoInfo->fps_den = 1;
		this->m_pObsVideoInfo->graphics_module = DL_D3D11;
		this->m_pObsVideoInfo->output_format = VIDEO_FORMAT_I420; //VIDEO_FORMAT_NV12 VIDEO_FORMAT_BGRA
	}
	else 
	{
		this->m_pObsVideoInfo->fps_num = _fps_num;
		this->m_pObsVideoInfo->fps_den = _fps_den;
		this->m_pObsVideoInfo->output_format = 
			this->GetVideoFormat(nOutWidth); //recHeader.recArea.GetWidth()
	}
	this->m_pObsVideoInfo->colorspace = VIDEO_CS_709;
	this->m_pObsVideoInfo->range = VIDEO_RANGE_FULL;
	this->m_pObsVideoInfo->adapter = 0;
	this->m_pObsVideoInfo->gpu_conversion = true;
	this->m_pObsVideoInfo->scale_type = OBS_SCALE_BICUBIC; //OBS_SCALE_BILINEAR
	
	this->m_pObsVideoInfo->base_width = nInWidth;// recHeader.recArea.GetWidth();
	this->m_pObsVideoInfo->base_height = nInHeight;// recHeader.recArea.GetHeight();
	this->m_pObsVideoInfo->output_width = nOutWidth;// recHeader.recArea.GetWidth();
	this->m_pObsVideoInfo->output_height = nOutHeight;//recHeader.recArea.GetHeight();

	nRes = obs_reset_video(this->m_pObsVideoInfo.get());
	if (nRes != OBS_VIDEO_SUCCESS) {
		if (nRes == OBS_VIDEO_CURRENTLY_ACTIVE) {
			blog(LOG_WARNING, "Tried to reset when "
				"already active %d", __LINE__);
		}
		else {
			if (this->m_graphicsModule == GM_DL_D3D11) {
				blog(LOG_WARNING,
					"Failed to initialize video (%d) "
					"with graphics_module='%s', retrying "
					"with graphics_module='%s' %d",
					nRes, this->m_pObsVideoInfo->graphics_module, DL_OPENGL, __LINE__);
				this->m_pObsVideoInfo->graphics_module = DL_OPENGL;
				this->m_graphicsModule = GM_DL_OPENGL;
			} else {
				blog(LOG_WARNING,
					"Failed to initialize video (%d) "
					"with graphics_module='%s', retrying "
					"with graphics_module='%s' %d",
					nRes, this->m_pObsVideoInfo->graphics_module, DL_D3D11, __LINE__);
				this->m_pObsVideoInfo->graphics_module = DL_D3D11;
				this->m_graphicsModule = GM_DL_D3D11;
			}
			this->m_pObsVideoInfo->output_format =
				this->GetVideoFormat(nOutWidth); //recHeader.recArea.GetWidth()
			nRes = obs_reset_video(this->m_pObsVideoInfo.get());
		}
	}

	//if (nRes != OBS_VIDEO_SUCCESS) {
	//	this->m_pObsVideoInfo->gpu_conversion = false;
	//	this->m_pObsVideoInfo->graphics_module = DL_D3D11;
	//	this->m_pObsVideoInfo->output_format =
	//		this->GetVideoFormat(nOutWidth); //recHeader.recArea.GetWidth()
	//	nRes = obs_reset_video(this->m_pObsVideoInfo.get());
	//}

	if (nRes == OBS_VIDEO_SUCCESS) {
		isRet = true;
	}
	if (nRes == OBS_VIDEO_SUCCESS && isFirst) {
		const float sdr_white_level = 300.00;
		const float hdr_nominal_peak_level = 1000.00;
		obs_set_video_levels(sdr_white_level, hdr_nominal_peak_level);
		video_t* video = obs_get_video();

		this->m_firstEncoded = video_output_get_total_frames(video);
		this->m_firstSkipped = video_output_get_skipped_frames(video);
		this->m_firstRendered = obs_get_total_frames();
		this->m_firstLagged = obs_get_lagged_frames();
	}
	return isRet;
}

void CObsVideoRecord::LoadVideoRecordingPreset(string strVideoEncoder)
{
	this->LoadVideoRecordingPresetLossy(this->GetAimpleOutputEncoder(strVideoEncoder.c_str()));
}

int CObsVideoRecord::CalcCRF(RectX recArea, int crf)
{
	int cx = recArea.GetWidth();
	int cy = recArea.GetHeight();
	double fCX = double(cx);
	double fCY = double(cy);

	if (this->m_lowCPUx264)
		crf -= 2;

	double crossDist = sqrt(fCX * fCX + fCY * fCY);
	double crfResReduction =
		fmin(CROSS_DIST_CUTOFF, crossDist) / CROSS_DIST_CUTOFF;
	crfResReduction = (1.0 - crfResReduction) * 10.0;

	return crf - int(crfResReduction);
}

void CObsVideoRecord::UpdateVideoRecordingSettingsX264Crf(
	ushort usFps, RectX recArea, QualityType enQuality, int nCrfValue,
	int nVideoBitrateType, int nVideoBitrate, VideoEnCode nVideoEnCode)
{
	int nCrf = 20;
	nCrf = nCrfValue;

	if (nCrf > 51) { nCrf = 51; }
	else if (nCrf < 0) { nCrf = 0; }

	//
	{
		OBSDataAutoRelease settings = obs_data_create();

		obs_data_set_string(settings, "profile", "high");
		//ultrafast = "%1（低CPU使用率，最低质量）"
		//veryfast = "%1（默认）（中等 CPU 使用率，标准质量）"
		//fast = "%1（高 CPU 使用率，高质量）"
		obs_data_set_bool(settings, "use_bufsize", true);
		

		obs_encoder_update(m_pVideoRecording, settings);
	}
}

void CObsVideoRecord::UpdateVideoFps(ushort usFps)
{
	OBSDataAutoRelease settings = obs_data_create();
	
	obs_data_set_int(settings, "gop_size", usFps);
	obs_encoder_update(m_pVideoRecording, settings);
}

void CObsVideoRecord::SetVideoEncoder()
{
	obs_encoder_set_video(m_pVideoRecording, obs_get_video());
}

void CObsVideoRecord::UpdateVideoOutPut(obs_output_t* pOutPut)
{
	obs_output_set_video_encoder(pOutPut, m_pVideoRecording);
}

void CObsVideoRecord::SetVideoFps(uint usFps)
{
	obs_video_info ovi;
	obs_get_video_info(&ovi);
	if (ovi.fps_num != usFps || ovi.fps_den != 1) {
		this->m_pObsVideoInfo->fps_num = usFps;
		this->m_pObsVideoInfo->fps_den = 1;
		obs_reset_video(this->m_pObsVideoInfo.get());
	}
}

void CObsVideoRecord::ReductionVideoFps()
{
	obs_video_info ovi;
	obs_get_video_info(&ovi);
	if (ovi.fps_num >= 5) {
		this->m_pObsVideoInfo->fps_num = 1;
		this->m_pObsVideoInfo->fps_den = 1;
		obs_reset_video(this->m_pObsVideoInfo.get());
	}
}

void CObsVideoRecord::SetVideoReInSize(SIZE sz)
{
	obs_video_info ovi;
	obs_get_video_info(&ovi);
	if ((ovi.base_width != sz.cx) ||
		(ovi.base_height != sz.cy) )
	{
		this->m_pObsVideoInfo->base_width = sz.cx;
		this->m_pObsVideoInfo->base_height = sz.cy;
		int nRes = obs_reset_video(this->m_pObsVideoInfo.get());
	}
}

void CObsVideoRecord::SetVideoReInSize(SIZE sz, uint usFps)
{
	if ((this->m_pObsVideoInfo->base_width == sz.cx) &&
		(this->m_pObsVideoInfo->base_width == sz.cy) && 
		(this->m_pObsVideoInfo->fps_num == usFps)) {
		return;
	}
	this->m_pObsVideoInfo->base_width = sz.cx;
	this->m_pObsVideoInfo->base_height = sz.cy;
	this->m_pObsVideoInfo->fps_num = usFps;
	this->m_pObsVideoInfo->fps_den = 1;
	int nRes = obs_reset_video(this->m_pObsVideoInfo.get());
}

SIZE CObsVideoRecord::GetVideoSize()
{
	SIZE szRet = { 0, 0 };
	szRet.cx = this->m_pObsVideoInfo->base_width;
	szRet.cy = this->m_pObsVideoInfo->base_height;
	return szRet;
}

void CObsVideoRecord::StopVideo()
{
}

void CObsVideoRecord::DisplayVideo()
{

}

void CObsVideoRecord::InitData()
{
	this->m_graphicsModule = GM_NULL;
	this->m_lowCPUx264 = true;
	this->m_firstEncoded = 0xFFFFFFFF;
	this->m_firstSkipped = 0xFFFFFFFF;
	this->m_firstRendered = 0xFFFFFFFF;
	this->m_firstLagged = 0xFFFFFFFF;
	this->m_pVideoRecording = nullptr;
	this->m_pObsVideoInfo = nullptr;
}

void CObsVideoRecord::LoadVideoRecordingPresetLossy(const char* encoderId)
{
	m_pVideoRecording = obs_video_encoder_create(
		encoderId, "simple_video_recording", nullptr, nullptr);
	if (!m_pVideoRecording)
		throw "Failed to create video recording encoder (simple output)";
	obs_encoder_release(m_pVideoRecording);
}

const char* CObsVideoRecord::GetAimpleOutputEncoder(const char* encoder)
{
	if (strcmp(encoder, SIMPLE_ENCODER_X264) == 0) {
		return "obs_x264";
	}
	else if (strcmp(encoder, SIMPLE_ENCODER_X264_LOWCPU) == 0) {
		return "obs_x264";
	}
	else if (strcmp(encoder, SIMPLE_ENCODER_QSV) == 0) {
		return "obs_qsv11_v2";
	}
	else if (strcmp(encoder, SIMPLE_ENCODER_QSV_AV1) == 0) {
		return "obs_qsv11_av1";
	}
	else if (strcmp(encoder, SIMPLE_ENCODER_AMD) == 0) {
		return "h264_texture_amf";
#ifdef ENABLE_HEVC
	}
	else if (strcmp(encoder, SIMPLE_ENCODER_AMD_HEVC) == 0) {
		return "h265_texture_amf";
#endif
	}
	else if (strcmp(encoder, SIMPLE_ENCODER_AMD_AV1) == 0) {
		return "av1_texture_amf";
	}
	else if (strcmp(encoder, SIMPLE_ENCODER_NVENC) == 0) {
		return this->EncoderAvailable("jim_nvenc") ? "jim_nvenc"
			: "ffmpeg_nvenc";
#ifdef ENABLE_HEVC
	}
	else if (strcmp(encoder, SIMPLE_ENCODER_NVENC_HEVC) == 0) {
		return EncoderAvailable("jim_hevc_nvenc") ? "jim_hevc_nvenc"
			: "ffmpeg_hevc_nvenc";
#endif
	}
	else if (strcmp(encoder, SIMPLE_ENCODER_NVENC_AV1) == 0) {
		return "jim_av1_nvenc";
	}
	else if (strcmp(encoder, SIMPLE_ENCODER_APPLE_H264) == 0) {
		return "com.apple.videotoolbox.videoencoder.ave.avc";
#ifdef ENABLE_HEVC
	}
	else if (strcmp(encoder, SIMPLE_ENCODER_APPLE_HEVC) == 0) {
		return "com.apple.videotoolbox.videoencoder.ave.hevc";
#endif
	}

	return "obs_x264";
}

bool CObsVideoRecord::EncoderAvailable(const char* encoder)
{
	const char* val;
	int i = 0;

	while (obs_enum_encoder_types(i++, &val))
		if (strcmp(val, encoder) == 0)
			return true;

	return false;
}
