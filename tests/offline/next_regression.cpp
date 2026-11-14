// Assertions specific to the repaired stereo/ping-pong and HOLD design.
#define main legacy_probe_main
#include "../../reports/next-20261114/probe.cpp"
#undef main
#include <string>
static int failures=0;
static void check(bool ok,const char* what){printf("%s %s\n",ok?"PASS":"FAIL",what);failures+=!ok;}
static double energy(const std::vector<float>&v,int a,int b){double x=0;for(int i=a;i<b;i++)x+=double(v[i])*v[i];return x;}
static void wav(const std::string&path,const Audio&a){FILE*f=fopen(path.c_str(),"wb");if(!f){perror(path.c_str());exit(2);}auto u16=[&](unsigned n){fputc(n&255,f);fputc((n>>8)&255,f);};auto u32=[&](unsigned n){u16(n);u16(n>>16);};fwrite("RIFF",1,4,f);u32(36+a.l.size()*4);fwrite("WAVEfmt ",1,8,f);u32(16);u16(1);u16(2);u32(44100);u32(176400);u16(4);u16(16);fwrite("data",1,4,f);u32(a.l.size()*4);for(size_t i=0;i<a.l.size();i++){u16((uint16_t)(int16_t)(clamp(a.l[i],-1.f,1.f)*32767));u16((uint16_t)(int16_t)(clamp(a.r[i],-1.f,1.f)*32767));}fclose(f);}
int main(int argc,char**argv){
 for(float fb:{-.8f,.8f})for(int input:{0,1,2}){
  auto p=wet();p.timeMs=100;p.feedback=fb;p.stereoize=true;p.width=1;Engine e;e.prepare(fs);e.setParams(p);Audio a(fs);a.l[0]=input==2?0:.8;a.r[0]=input==1?0:.8;render(e,a,37);
  for(int echo=1;echo<=4;echo++){int c=echo*4410;double l=energy(a.l,c-100,c+300),r=energy(a.r,c-100,c+300);printf("ECHO input=%d fb=%+.1f n=%d L=%.8g R=%.8g\n",input,fb,echo,l,r);check(echo&1?l>30*r:r>30*l,"echo alternates L/R with >14.7 dB separation");auto&v=echo&1?a.l:a.r;int peak=c-100;for(int i=c-100;i<c+300;i++)if(fabs(v[i])>fabs(v[peak]))peak=i;check(fabs(peak/fs-echo*.1)<.003,"echo timing within 3 ms");}
 }
 // A narrowband burst establishes signed repeat polarity without aligning tiny filtered impulse lobes.
 for(float fb:{-.8f,.8f}){auto p=wet();p.timeMs=100;p.feedback=fb;p.stereoize=true;p.width=1;
 Engine e;e.prepare(fs);e.setParams(p);Audio a(fs);for(int i=0;i<882;i++)a.l[i]=a.r[i]=.25f*sin(2*pi*500*i/fs)*sin(pi*i/882);render(e,a);
 double cross=0;for(int i=4500;i<5200;i++)cross+=double(a.l[i])*a.r[i+4410];
 printf("POLARITY feedback=%+.1f correlation_product=%.9g\n",fb,cross);
 check(cross*fb>0,"feedback sign determines second-echo polarity");}
 // OFF remains stereo; left/right asymmetric material does not cross over.
 for(int side:{0,1}){auto p=wet();p.timeMs=100;p.feedback=.8;Engine e;e.prepare(fs);e.setParams(p);Audio a(fs);(side?a.r:a.l)[0]=.8;render(e,a);check(energy(side?a.l:a.r,4300,19000)<1e-5,"OFF no cross-channel input/feedback leakage above converter noise");}
 // Dry stereo is bit-exact under either mode, even with rapid settings changes.
 for(bool st:{false,true}){Engine e;e.prepare(fs);auto p=wet();p.mix=0;p.stereoize=st;e.setParams(p);Audio a(2048);for(int i=0;i<2048;i++){a.l[i]=.3*sin(.02*i);a.r[i]=.7*cos(.03*i);}Audio original=a;render(e,a,37);check(a.l==original.l&&a.r==original.r,"dry stereo bit-exact with Mix zero");}
 // Width is a post-loop pan matrix: mono sum must not change with Width.
 {auto p=wet();p.timeMs=100;p.feedback=.8;p.stereoize=true;Audio a(fs),b(fs);a.l[0]=a.r[0]=b.l[0]=b.r[0]=.8;Engine x,y;x.prepare(fs);y.prepare(fs);p.width=0;x.setParams(p);p.width=1;y.setParams(p);render(x,a);render(y,b);double delta=0;for(int i=0;i<fs;i++)delta=std::max(delta,double(fabs((a.l[i]+a.r[i])-(b.l[i]+b.r[i]))));check(delta<1e-6,"Width preserves wet mono sum (no clock-skew comb scheme)");}
 // Wide held-clock ratios have bounded cost, no cell-tick while loop.
 {Engine e;e.prepare(fs);auto p=wet();p.timeMs=900;p.bandwidth=1;p.feedback=.9;p.stereoize=true;e.setParams(p);Audio a(2*fs);sine(a,100);render(e,a);p.hold=true;e.setParams(p);Audio settle(4410);render(e,settle);p.timeMs=1.75;e.setParams(p);Audio b(fs);render(e,b);bool finite=true;for(float v:b.l)finite&=std::isfinite(v)&&fabs(v)<8;check(finite&&e.clockRatio(0)>100,"HOLD extreme pitch-up crosses host clock ceiling and stays finite");p.hold=false;e.setParams(p);Audio c(2*fs);render(e,c);for(float v:c.r)finite&=std::isfinite(v)&&fabs(v)<8;check(finite,"HOLD extreme pitch-up release stays finite");}
 if(argc>1){for(bool st:{false,true}){auto p=wet();p.timeMs=250;p.feedback=.72;p.grit=.35;p.stereoize=st;p.width=1;p.mix=.55;Engine e;e.prepare(fs);e.setParams(p);Audio a(5*fs);for(int onset:{0,88200})for(int i=0;i<4000;i++)a.l[onset+i]=a.r[onset+i]=.55*exp(-i/600.)*sin(2*pi*440*i/fs);render(e,a);wav(std::string(argv[1])+(st?"/pingpong-mono-plucks.wav":"/ordinary-stereo-mono-plucks.wav"),a);}
 auto p=wet();p.timeMs=180;p.feedback=-.7;p.stereoize=true;p.width=1;p.mix=.5;Engine e;e.prepare(fs);e.setParams(p);Audio a(5*fs);for(int i=0;i<6000;i++){a.l[i]=.5*exp(-i/900.)*sin(2*pi*330*i/fs);a.r[22050+i]=.4*exp(-i/700.)*sin(2*pi*550*i/fs);}render(e,a);wav(std::string(argv[1])+"/pingpong-stereo-negative-feedback.wav",a);}
 printf("NEXT_REGRESSION failures=%d callback_allocations=%zu\n",failures,allocs);return failures||allocs?1:0;
}
