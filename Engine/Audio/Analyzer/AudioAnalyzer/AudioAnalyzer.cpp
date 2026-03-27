#include "Engine/Audio/Analyzer/AudioAnalyzer/AudioAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <vector>

namespace Engine {

static constexpr float kPi = 3.14159265f;

static void FFT_DIT(std::vector<float>& re, std::vector<float>& im) {
    uint32_t n=(uint32_t)re.size();
    // Bit-reversal
    for(uint32_t i=1,j=0;i<n;i++){
        uint32_t bit=n>>1;
        for(;j&bit;bit>>=1) j^=bit;
        j^=bit; if(i<j){std::swap(re[i],re[j]);std::swap(im[i],im[j]);}
    }
    for(uint32_t len=2;len<=n;len<<=1){
        float ang=-2*kPi/len;
        float wr=std::cos(ang),wi=std::sin(ang);
        for(uint32_t i=0;i<n;i+=len){
            float cr=1,ci=0;
            for(uint32_t j=0;j<len/2;j++){
                float ur=re[i+j],ui=im[i+j];
                float vr=re[i+j+len/2]*cr-im[i+j+len/2]*ci;
                float vi=re[i+j+len/2]*ci+im[i+j+len/2]*cr;
                re[i+j]=ur+vr; im[i+j]=ui+vi;
                re[i+j+len/2]=ur-vr; im[i+j+len/2]=ui-vi;
                float ncr=cr*wr-ci*wi; ci=cr*wi+ci*wr; cr=ncr;
            }
        }
    }
}

struct AudioAnalyzer::Impl {
    uint32_t fftSize{1024};
    uint32_t sampleRate{44100};
    std::vector<float> window;
    std::vector<float> buf;
    std::vector<float> spectrum;
    float rms{0}, peak{0};
    bool beat{false};
    float bpm{0}, lastBeatTime{0}, beatInterval{0};
    float beatThreshold{1.3f};
    float elapsed{0};
    std::function<void(float)> onBeat;
    std::vector<std::vector<float>> history;
    uint32_t histLen{30};
    float prevEnergy{0};

    std::vector<float> bandEnergy;

    void BuildWindow(){
        window.resize(fftSize);
        for(uint32_t i=0;i<fftSize;i++) window[i]=0.5f*(1.f-std::cos(2*kPi*i/(fftSize-1)));
    }
    void ComputeBands(uint32_t sr){
        bandEnergy.assign(6,0.f);
        // sub-bass 20-60Hz, bass 60-250, mid 250-2kHz, high 2k-6k, presence 6k-12k, brilliance 12k-20k
        static const float edges[]={20,60,250,2000,6000,12000,20000};
        uint32_t half=fftSize/2;
        for(uint32_t b=0;b<6;b++){
            float f0=edges[b], f1=edges[b+1];
            uint32_t i0=(uint32_t)(f0*half/((float)sr/2)), i1=(uint32_t)(f1*half/((float)sr/2));
            i0=std::min(i0,half-1); i1=std::min(i1,half-1);
            float sum=0; for(uint32_t i=i0;i<=i1;i++) sum+=spectrum[i];
            bandEnergy[b]=(i1>i0)?sum/(i1-i0):0;
        }
    }
};

AudioAnalyzer::AudioAnalyzer() : m_impl(new Impl){}
AudioAnalyzer::~AudioAnalyzer(){ Shutdown(); delete m_impl; }
void AudioAnalyzer::Init(uint32_t fs){ m_impl->fftSize=fs; m_impl->BuildWindow(); m_impl->spectrum.assign(fs/2,0); m_impl->buf.clear(); m_impl->bandEnergy.assign(6,0.f); }
void AudioAnalyzer::Shutdown(){ m_impl->buf.clear(); m_impl->spectrum.clear(); m_impl->history.clear(); }
void AudioAnalyzer::Reset(){ Shutdown(); Init(m_impl->fftSize); }

void AudioAnalyzer::Submit(const float* s, uint32_t n, uint32_t sr){
    m_impl->sampleRate=sr;
    for(uint32_t i=0;i<n;i++) m_impl->buf.push_back(s[i]);
    while(m_impl->buf.size()>=m_impl->fftSize){
        std::vector<float> re(m_impl->fftSize),im(m_impl->fftSize,0);
        for(uint32_t i=0;i<m_impl->fftSize;i++) re[i]=m_impl->buf[i]*m_impl->window[i];
        FFT_DIT(re,im);
        float rms2=0,pk=0,en=0;
        for(uint32_t i=0;i<m_impl->fftSize/2;i++){
            float mag=std::sqrt(re[i]*re[i]+im[i]*im[i])/(m_impl->fftSize/2);
            m_impl->spectrum[i]=mag;
            en+=mag*mag; rms2+=re[i]*re[i]; if(mag>pk) pk=mag;
        }
        m_impl->peak=pk;
        m_impl->rms=std::sqrt(en/(m_impl->fftSize/2));
        // Beat detect
        float energy=en/(m_impl->fftSize/2);
        m_impl->beat=false;
        if(m_impl->prevEnergy>0&&energy>m_impl->prevEnergy*m_impl->beatThreshold){
            m_impl->beat=true;
            float now=m_impl->elapsed;
            if(m_impl->lastBeatTime>0) m_impl->beatInterval=now-m_impl->lastBeatTime;
            if(m_impl->beatInterval>0) m_impl->bpm=60.f/m_impl->beatInterval;
            m_impl->lastBeatTime=now;
            if(m_impl->onBeat) m_impl->onBeat(m_impl->bpm);
        }
        m_impl->prevEnergy=energy;
        m_impl->elapsed+=(float)m_impl->fftSize/sr;
        m_impl->ComputeBands(sr);
        // History
        m_impl->history.push_back(m_impl->spectrum);
        while((uint32_t)m_impl->history.size()>m_impl->histLen) m_impl->history.erase(m_impl->history.begin());
        m_impl->buf.erase(m_impl->buf.begin(),m_impl->buf.begin()+m_impl->fftSize/2);
    }
}

void AudioAnalyzer::GetSpectrum(float* out, uint32_t bins) const {
    for(uint32_t i=0;i<bins;i++) out[i]=(i<m_impl->spectrum.size())?m_impl->spectrum[i]:0;
}
uint32_t AudioAnalyzer::FFTSize()  const { return m_impl->fftSize; }
uint32_t AudioAnalyzer::BinCount() const { return m_impl->fftSize/2; }
float AudioAnalyzer::GetRMS()  const { return m_impl->rms; }
float AudioAnalyzer::GetPeak() const { return m_impl->peak; }
float AudioAnalyzer::GetBandEnergy(FreqBand b) const { return (uint8_t)b<m_impl->bandEnergy.size()?m_impl->bandEnergy[(uint8_t)b]:0; }
bool  AudioAnalyzer::IsBeat()     const { return m_impl->beat; }
float AudioAnalyzer::GetEstBPM()  const { return m_impl->bpm; }
void  AudioAnalyzer::SetOnBeat(std::function<void(float)> cb){ m_impl->onBeat=cb; }
void  AudioAnalyzer::SetBeatThreshold(float t){ m_impl->beatThreshold=t; }
const std::vector<std::vector<float>>& AudioAnalyzer::GetHistory() const { return m_impl->history; }
void  AudioAnalyzer::SetHistoryLength(uint32_t n){ m_impl->histLen=n; }

} // namespace Engine
