#include "Engine/Audio/Stream/AudioStream/AudioStream.h"
#include <algorithm>
#include <cstring>
#include <vector>

namespace Engine {

struct AudioStream::Impl {
    bool     playing{false};
    bool     paused {false};
    bool     looping{false};
    float    volume {1.f};
    float    pitch  {1.f};
    uint32_t sampleRate{44100};
    uint32_t channels  {2};
    uint32_t bitsPerSample{16};
    uint64_t totalFrames{0};
    uint64_t position   {0};
    bool     atEnd{false};
    bool     hasData{false};
    std::vector<float> pcmData; // normalized float PCM (stereo interleaved)
    std::function<void()>          onEnd;
    std::function<void(uint64_t)>  onSeek;
    std::function<void(float*,uint32_t)> feedCb;
};

AudioStream::AudioStream(): m_impl(new Impl){}
AudioStream::~AudioStream(){ Shutdown(); delete m_impl; }
void AudioStream::Init(){}
void AudioStream::Shutdown(){ m_impl->pcmData.clear(); m_impl->playing=false; }
void AudioStream::Reset(){ *m_impl=Impl{}; }

bool AudioStream::OpenFile(const std::string& path){
    // Stub: treat file as empty PCM, just set valid state
    (void)path;
    m_impl->hasData=true; m_impl->totalFrames=44100; // 1 second placeholder
    m_impl->pcmData.assign(m_impl->totalFrames*m_impl->channels, 0.f);
    m_impl->position=0; m_impl->atEnd=false;
    return true;
}
bool AudioStream::OpenMemory(const uint8_t* data, uint32_t size){
    // Treat raw bytes as 16-bit PCM
    uint32_t frames=size/(m_impl->channels*2);
    m_impl->pcmData.resize(frames*m_impl->channels);
    for(uint32_t i=0;i<m_impl->pcmData.size();i++){
        uint32_t byteOff=i*2;
        if(byteOff+1<size){
            int16_t s=(int16_t)(data[byteOff]|(data[byteOff+1]<<8));
            m_impl->pcmData[i]=s/32768.f;
        }
    }
    m_impl->totalFrames=frames; m_impl->hasData=true;
    m_impl->position=0; m_impl->atEnd=false;
    return true;
}
void AudioStream::Close(){ m_impl->pcmData.clear(); m_impl->hasData=false; Stop(); }

void AudioStream::Play (){ if(m_impl->hasData){ m_impl->playing=true; m_impl->paused=false; } }
void AudioStream::Pause(){ if(m_impl->playing){ m_impl->paused=true; } }
void AudioStream::Stop (){ m_impl->playing=false; m_impl->paused=false; m_impl->position=0; m_impl->atEnd=false; }

bool AudioStream::Seek(uint64_t offset){
    if(offset>m_impl->totalFrames) return false;
    m_impl->position=offset; m_impl->atEnd=(offset==m_impl->totalFrames);
    if(m_impl->onSeek) m_impl->onSeek(offset);
    return true;
}
uint64_t AudioStream::GetPositionSamples() const { return m_impl->position; }
uint64_t AudioStream::GetDurationSamples() const { return m_impl->totalFrames; }
uint32_t AudioStream::GetSampleRate    () const { return m_impl->sampleRate; }
uint32_t AudioStream::GetChannelCount  () const { return m_impl->channels; }
uint32_t AudioStream::GetBitsPerSample () const { return m_impl->bitsPerSample; }

uint32_t AudioStream::ReadSamples(float* out, uint32_t frameCount){
    if(!m_impl->playing||m_impl->paused||m_impl->atEnd) return 0;
    uint64_t avail=m_impl->totalFrames-m_impl->position;
    uint32_t toRead=(uint32_t)std::min((uint64_t)frameCount,avail);
    uint32_t ch=m_impl->channels;
    for(uint32_t f=0;f<toRead;f++){
        for(uint32_t c=0;c<ch;c++){
            uint64_t idx=(m_impl->position+f)*ch+c;
            float s = idx<m_impl->pcmData.size()?m_impl->pcmData[idx]:0.f;
            out[f*ch+c]=s*m_impl->volume;
        }
    }
    m_impl->position+=toRead;
    if(m_impl->position>=m_impl->totalFrames){
        m_impl->atEnd=true;
        if(m_impl->looping){ m_impl->position=0; m_impl->atEnd=false; }
        else { m_impl->playing=false; if(m_impl->onEnd) m_impl->onEnd(); }
    }
    if(m_impl->feedCb) m_impl->feedCb(out,toRead);
    return toRead;
}

void  AudioStream::SetLooping(bool on)  { m_impl->looping=on; }
bool  AudioStream::IsLooping () const   { return m_impl->looping; }
void  AudioStream::SetVolume (float v)  { m_impl->volume=std::max(0.f,v); }
float AudioStream::GetVolume () const   { return m_impl->volume; }
void  AudioStream::SetPitch  (float p)  { m_impl->pitch=std::max(0.01f,p); }
float AudioStream::GetPitch  () const   { return m_impl->pitch; }
bool  AudioStream::IsPlaying () const   { return m_impl->playing&&!m_impl->paused; }
bool  AudioStream::IsPaused  () const   { return m_impl->paused; }
bool  AudioStream::IsAtEnd   () const   { return m_impl->atEnd; }

void AudioStream::SetOnEnd (std::function<void()>          cb){ m_impl->onEnd=cb; }
void AudioStream::SetOnSeek(std::function<void(uint64_t)>  cb){ m_impl->onSeek=cb; }
void AudioStream::SetFeedCallback(std::function<void(float*,uint32_t)> cb){ m_impl->feedCb=cb; }

} // namespace Engine
