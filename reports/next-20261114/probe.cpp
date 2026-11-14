#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <new>
#include <vector>
#include <cstring>
static bool countAlloc=false; static size_t allocs=0;
void* operator new(std::size_t n){ if(countAlloc)++allocs; if(void*p=std::malloc(n))return p;throw std::bad_alloc(); }
void operator delete(void*p) noexcept {std::free(p);} void operator delete(void*p,std::size_t) noexcept {std::free(p);}
#include "early_delay_dsp.h"
using namespace edly;
constexpr double fs=44100, pi=3.14159265358979323846;
struct Audio{std::vector<float> l,r;Audio(int n):l(n),r(n){}};
EngineParams wet(){EngineParams p;p.mix=1;p.feedback=0;p.modDepth=0;p.grit=0;p.tone=1;p.drive=0;return p;}
void render(Engine&e,Audio&a,int block=128){for(int i=0;i<(int)a.l.size();i+=block){int n=std::min(block,(int)a.l.size()-i);countAlloc=true;e.process(a.l.data()+i,a.r.data()+i,a.l.data()+i,a.r.data()+i,n);countAlloc=false;}}
void sine(Audio&a,double f,double amp=.25){for(int i=0;i<(int)a.l.size();i++)a.l[i]=a.r[i]=amp*std::sin(2*pi*f*i/fs);}
double rms(const std::vector<float>&a,int start,int end){double s=0;for(int i=start;i<end;i++)s+=double(a[i])*a[i];return std::sqrt(s/(end-start));}
double db(double a){return 20*std::log10(std::max(a,1e-20));}
double freq(const std::vector<float>&a,int start,int end){int crossings=0,first=-1,last=0;for(int i=start+1;i<end;i++)if(a[i-1]<0&&a[i]>=0){if(first<0)first=i;last=i;crossings++;}return (crossings-1)*fs/(last-first);}
int main(){
 // Narrow-band mono compatibility; octave averages cannot rule out comb notches.
 for(double f:{62.5,187.5,312.5,437.5,562.5,687.5,812.5,1000.}){auto p=wet();p.timeMs=250;p.stereoize=true;p.width=1;Engine e;e.prepare(fs);e.setParams(p);Audio a(3*fs);sine(a,f);render(e,a,96);double ll=0,rr=0,lr=0,mm=0;for(int i=2*fs;i<3*fs;i++){ll+=a.l[i]*a.l[i];rr+=a.r[i]*a.r[i];lr+=a.l[i]*a.r[i];double m=(a.l[i]+a.r[i])*.5;mm+=m*m;}printf("STEREO f=%.1f corr=%.6f mono_loss_dB=%.3f channel_rms=%.6f\n",f,lr/sqrt(ll*rr),10*log10(mm/((ll+rr)*.5)),sqrt((ll+rr)/(2*fs)));}
 // HOLD speed cap, both below and across clock knee.
 for(float start:{100.f,300.f})for(float end:{50.f,150.f,600.f}){auto p=wet();p.timeMs=start;Engine e;e.prepare(fs);e.setParams(p);Audio a(2*fs);sine(a,1000);render(e,a);p.hold=true;e.setParams(p);Audio held(fs);render(e,held);p.timeMs=end;e.setParams(p);Audio b(3*fs);render(e,b);printf("HOLD start=%.0f end=%.0f before_Hz=%.2f after_Hz=%.2f expected_Hz=%.2f clock_ratio=%.6f\n",start,end,freq(held.l,fs/2,fs),freq(b.l,2*fs,3*fs),1000.*start/end,e.clockRatio(0));}
 // Reset determinism after non-control-aligned processing and changed settings.
 {auto p=wet();p.timeMs=1.75;Engine e;e.prepare(fs);auto old=p;old.timeMs=900;old.bandwidth=0;old.drive=1;e.setParams(old);Audio prim(44103);sine(prim,1000);render(e,prim,97);e.reset();e.setParams(p);Audio a(4096);sine(a,1000);render(e,a,97);Engine ref;ref.prepare(fs);ref.setParams(p);Audio b(4096);sine(b,1000);render(ref,b,97);double d=0;int first=-1;for(int i=0;i<4096;i++){double x=fabs(a.l[i]-b.l[i]);if(x>1e-7&&first<0)first=i;d=std::max(d,x);}printf("RESET max_delta=%.9g first_difference_sample=%d\n",d,first);}
 // In-place vs separate, variable counts including non-multiples and zero.
 {auto p=wet();p.stereoize=true;p.feedback=-1;p.timeMs=1.75;Engine a,b;a.prepare(fs);b.prepare(fs);a.setParams(p);b.setParams(p);Audio x(5000),y(5000);sine(x,330);y=x;render(a,x,37);Audio out(5000);for(int i=0;i<5000;i+=37)b.process(y.l.data()+i,y.r.data()+i,out.l.data()+i,out.r.data()+i,std::min(37,5000-i));printf("INPLACE identical=%d\n",x.l==out.l&&x.r==out.r);}
 // Bad audio ingress recovers? Wet *and* dry are evaluated even at 100% wet.
 for(float bad:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}){auto p=wet();p.timeMs=10;Engine e;e.prepare(fs);e.setParams(p);Audio a(fs);sine(a,440);a.l[100]=a.r[100]=bad;render(e,a);int nf=0;for(float v:a.l)nf+=!std::isfinite(v);printf("BAD_AUDIO kind=%s nonfinite=%d late_rms_dB=%.2f\n",std::isnan(bad)?"NaN":"Inf",nf,db(rms(a.l,fs/2,fs)));}
 // Rapid legal automation, HOLD transitions and signed feedback.
 {Engine e;e.prepare(fs);Audio a(128);unsigned rng=23;double peak=0;int nf=0;for(int block=0;block<20000;block++){auto randf=[&](){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (rng&65535)/65535.f;};auto p=wet();p.timeMs=1.75f+898.25f*randf();p.bandwidth=randf();p.feedback=2*randf()-1;p.drive=randf();p.grit=randf();p.modDepth=randf();p.modSpeedHz=.05+9.95*randf();p.hold=randf()>.5;p.stereoize=randf()>.5;p.width=randf();e.setParams(p);for(int i=0;i<128;i++)a.l[i]=a.r[i]=(block<10000?4:0)*(2*randf()-1);render(e,a);for(float v:a.l){nf+=!std::isfinite(v);peak=std::max(peak,double(fabs(v)));}}printf("AUTOMATION frames=2560000 nonfinite=%d peak=%.6f\n",nf,peak);}
 printf("CALLBACK_NEW_COUNT %zu\n",allocs);
 return 0;
}
