#include "Engine/Render/Screenshot/ScreenshotSystem.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <queue>
#include <sstream>
#include <vector>

namespace Engine {

static std::string AutoFilename(const std::string& outputDir,
                                 const std::string& tmpl,
                                 uint32_t index,
                                 ImageFormat fmt) {
    auto now=std::chrono::system_clock::now();
    auto tt=std::chrono::system_clock::to_time_t(now);
    std::tm tm_info{}; // POSIX: gmtime_r / Windows: gmtime_s
#if defined(_WIN32)
    gmtime_s(&tm_info,&tt);
#else
    gmtime_r(&tt,&tm_info);
#endif
    std::ostringstream oss; oss<<outputDir;
    if(tmpl.empty()) {
        oss<<"shot_"<<std::put_time(&tm_info,"%Y%m%d_%H%M%S")<<"_"<<index;
    } else {
        std::string t=tmpl;
        // Very simple substitution
        auto Replace=[&](std::string& s, const std::string& from, const std::string& to){
            size_t pos=0; while((pos=s.find(from,pos))!=std::string::npos){ s.replace(pos,from.size(),to); pos+=to.size(); }
        };
        std::ostringstream dateStr; dateStr<<std::put_time(&tm_info,"%Y%m%d");
        std::ostringstream idxStr;  idxStr<<index;
        Replace(t,"{date}",dateStr.str()); Replace(t,"{index}",idxStr.str());
        oss<<t;
    }
    switch(fmt){
        case ImageFormat::JPEG: oss<<".jpg"; break;
        case ImageFormat::BMP:  oss<<".bmp"; break;
        default:                oss<<".ppm"; break; // stub
    }
    return oss.str();
}

struct PendingCapture {
    CaptureRequest req;
};

struct ScreenshotSystem::Impl {
    std::string outputDir{"Screenshots/"};
    std::string filenameTmpl;
    uint32_t    captureIndex{0};

    std::queue<PendingCapture> pending;

    bool     sequencing{false};
    uint32_t seqTotal{0};
    uint32_t seqRemaining{0};
    uint32_t seqFps{60};

    std::function<void(const CaptureResult&)>                       onDone;
    std::function<void(const std::string&,uint32_t)>                onSeqDone;

    void DoCapture(const CaptureRequest& req) {
        CaptureResult r;
        r.path=AutoFilename(outputDir,filenameTmpl,captureIndex++,req.format);
        r.width=1920; r.height=1080; // placeholder dimensions
        r.succeeded=true;
        // In production: read framebuffer, write PNG/JPEG
        // Stub: write a tiny PPM header
        std::ofstream f(r.path,std::ios::binary);
        if(!f.is_open()){ r.succeeded=false; }
        // Don't actually write pixels in stub
        if(onDone) onDone(r);
    }
};

ScreenshotSystem::ScreenshotSystem() : m_impl(new Impl()) {}
ScreenshotSystem::~ScreenshotSystem() { delete m_impl; }
void ScreenshotSystem::Init()     {}
void ScreenshotSystem::Shutdown() {}

void ScreenshotSystem::SetOutputDir(const std::string& d){ m_impl->outputDir=d; if(!d.empty()&&d.back()!='/') m_impl->outputDir+='/'; }
void ScreenshotSystem::SetFilenameTemplate(const std::string& t){ m_impl->filenameTmpl=t; }

void ScreenshotSystem::Capture(const CaptureRequest& req) {
    m_impl->pending.push({req});
}
void ScreenshotSystem::CaptureHiRes(uint32_t scale, ImageFormat fmt) {
    CaptureRequest req; req.supersample=scale; req.format=fmt;
    Capture(req);
}

void ScreenshotSystem::BeginSequence(uint32_t frameCount, uint32_t fps) {
    m_impl->sequencing=true; m_impl->seqTotal=frameCount;
    m_impl->seqRemaining=frameCount; m_impl->seqFps=fps;
}
void ScreenshotSystem::EndSequence() {
    if(m_impl->onSeqDone) m_impl->onSeqDone(m_impl->outputDir, m_impl->seqTotal-m_impl->seqRemaining);
    m_impl->sequencing=false; m_impl->seqRemaining=0;
}
bool     ScreenshotSystem::IsCapturingSequence()     const{ return m_impl->sequencing; }
uint32_t ScreenshotSystem::SequenceFramesRemaining() const{ return m_impl->seqRemaining; }

void ScreenshotSystem::Tick(float) {
    // Drain pending captures
    while(!m_impl->pending.empty()) {
        auto pc=m_impl->pending.front(); m_impl->pending.pop();
        m_impl->DoCapture(pc.req);
    }
    // Sequence frame
    if(m_impl->sequencing && m_impl->seqRemaining>0) {
        CaptureRequest req; req.async=false;
        m_impl->DoCapture(req);
        --m_impl->seqRemaining;
        if(m_impl->seqRemaining==0) EndSequence();
    }
}

uint32_t ScreenshotSystem::PendingCount() const{ return (uint32_t)m_impl->pending.size(); }
void ScreenshotSystem::OnCaptureDone(std::function<void(const CaptureResult&)> cb){ m_impl->onDone=std::move(cb); }
void ScreenshotSystem::OnSequenceDone(std::function<void(const std::string&,uint32_t)> cb){ m_impl->onSeqDone=std::move(cb); }

} // namespace Engine
