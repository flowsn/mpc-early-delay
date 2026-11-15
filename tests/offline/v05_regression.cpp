// v0.5: actual plugin wrapper, all factory programs, output/invert and legacy IDs.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>
static bool counting=false; static size_t callbackAllocations=0;
void* operator new(std::size_t n){if(counting)++callbackAllocations;if(void*p=std::malloc(n))return p;throw std::bad_alloc();}
void operator delete(void*p) noexcept{std::free(p);} void operator delete(void*p,std::size_t) noexcept{std::free(p);}
#include "EarlyDelay.cpp"
constexpr int SR=44100; constexpr double PI=3.141592653589793;
static int failures=0;
void check(bool x,const char*s){printf("%s %s\n",x?"PASS":"FAIL",s);failures+=!x;}
struct Audio {std::vector<float> l,r; explicit Audio(int n):l(n),r(n){} }; 
void prepare(EarlyDelay&p){p.internalSetSampleRate(SR);p.internalSetBlockSize(128);p.internalResume();}
void process(EarlyDelay&p,Audio&a,int block=128){for(int off=0;off<(int)a.l.size();off+=block){float*io[]={a.l.data()+off,a.r.data()+off};counting=true;p.internalProcess(io,io,std::min(block,int(a.l.size())-off));counting=false;}}
double peak(const Audio&a){double p=0;for(auto*v:{&a.l,&a.r})for(float x:*v){if(!std::isfinite(x))return INFINITY;p=std::max(p,double(fabs(x)));}return p;}
double rms(const Audio&a,int begin=0){double sum=0;for(auto*v:{&a.l,&a.r})for(int i=begin;i<(int)v->size();++i)sum+=double((*v)[i])*(*v)[i];return sqrt(sum/(2*(a.l.size()-begin)));}
double db(double x){return 20*log10(std::max(1e-20,x));}
void wav(const std::string&path,const Audio&a){FILE*f=fopen(path.c_str(),"wb");if(!f){perror(path.c_str());exit(2);}auto u16=[&](unsigned n){fputc(n&255,f);fputc((n>>8)&255,f);};auto u32=[&](unsigned n){u16(n);u16(n>>16);};fwrite("RIFF",1,4,f);u32(36+a.l.size()*4);fwrite("WAVEfmt ",1,8,f);u32(16);u16(1);u16(2);u32(SR);u32(SR*4);u16(4);u16(16);fwrite("data",1,4,f);u32(a.l.size()*4);for(size_t i=0;i<a.l.size();i++){u16((uint16_t)(int16_t)lrint(a.l[i]*32767));u16((uint16_t)(int16_t)lrint(a.r[i]*32767));}fclose(f);}
unsigned rng=1471;
float noise(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (rng&65535)/32767.5f-1;}
int main(int argc,char**argv){
 setvbuf(stdout,nullptr,_IOLBF,0);
 const char* ids[]={"mix","time","feedback","modspeed","moddepth","tone","grit","bandwidth","drive","hold","stereoize","width","invert","output"};
 EarlyDelay base;check(base.numParams()==14&&base.numPrograms()==20,"fixed 14 parameters / 20 programs");for(int i=0;i<14;i++)check(base.param(i).id==ids[i],ids[i]);
 // A real v2 SDK state reduced to its first 12 records models a v0.1.1 project.
 base.setProgram(5);auto legacy=base.internalSaveState();size_t pos=12;for(int i=0;i<12;i++)pos+=1+legacy[pos]+8;legacy.resize(pos+4);for(size_t i=pos;i<pos+4;i++)legacy[i]=0;legacy[8]=12;
 EarlyDelay restored;check(restored.internalLoadState(legacy.data(),legacy.size()),"12-ID legacy state loads into fresh v0.6 instance");bool same=true;for(int i=0;i<12;i++)same&=base.getNormalised(i)==restored.getNormalised(i);check(same&&restored.get(12)==0&&restored.get(13)==0,"legacy signed values exact; appended controls default neutral");
 for(float fb:{-85.f,0.f,85.f}){EarlyDelay a,b;prepare(a);prepare(b);a.set(2,fb);a.set(12,1);b.set(2,-fb);a.set(1,4);b.set(1,4);Audio x(SR),y(SR);for(int i=0;i<SR;i++)x.l[i]=x.r[i]=y.l[i]=y.r[i]=i<1000?.5f*noise():0;process(a,x);process(b,y);check(x.l==y.l&&x.r==y.r,"Invert exactly reverses signed feedback; double negative is positive");}
 {EarlyDelay a,b;prepare(a);prepare(b);a.set(13,-12);Audio x(SR),y(SR);for(int i=0;i<SR;i++)x.l[i]=x.r[i]=y.l[i]=y.r[i]=.4f*sin(.031*i);process(a,x);process(b,y);double delta=0;float gain=pow(10.f,-12.f/20);for(int i=0;i<SR;i++)delta=std::max(delta,double(fabs(x.l[i]-gain*y.l[i])));check(delta<1e-6,"Output is an exact post-mix trim, outside the feedback loop");}
 // Full-scale (+/-0.98) multitone, broad-band bursts, sine sweep, and square wave,
 // followed by 20s silence. All factory presets are tested, not just demo settings.
 for(int program=0;program<base.numPrograms();program++){
  EarlyDelay p;prepare(p);p.setProgram(program);Audio a(8*SR);rng=1471;
  for(int i=0;i<8*SR;i++){double t=double(i)/SR;float x;if(i<2*SR)x=.98f*noise();else if(i<4*SR)x=.98*sin(2*PI*(70*t+350*t*t));else if(i<6*SR)x=.98*(sin(2*PI*110*t)>0?1:-1);else x=.98*(.4*sin(2*PI*200*t)+.35*sin(2*PI*500*t)+.25*sin(2*PI*3000*t));a.l[i]=x;a.r[i]=(i/22050)%2?x:-x;}
  process(p,a);Audio tail(20*SR);process(p,tail);double pk=std::max(peak(a),peak(tail)),late=rms(tail,19*SR);
  printf("PRESET %02d %-19s output_db=%5.1f feedback=%+.1f invert=%.0f peak=%.7f peak_dbfs=%.3f late_rms_dbfs=%.2f\n",program,p.programName(program),p.get(13),p.get(2),p.get(12),pk,db(pk),db(late));
  check(std::isfinite(pk)&&pk<1,"factory preset finite, no full-scale clipping under stress");check(late<.003,"factory preset decays below -50 dBFS after 20s");
 }
 // 30s held comb: freeze ignores wet input; output can still be turned down.
 {EarlyDelay p;prepare(p);p.setProgram(16);p.set(0,100);Audio a(SR);for(int i=0;i<SR;i++)a.l[i]=a.r[i]=.4*noise();process(p,a);p.set(9,1);Audio hold(30*SR);process(p,hold);printf("HOLD_COMB peak_dbfs=%.3f early_rms_dbfs=%.3f late_rms_dbfs=%.3f\n",db(peak(hold)),db(rms(hold,SR)),db(rms(hold,29*SR)));check(peak(hold)<1&&rms(hold,29*SR)>.00001,"30s Hold Drone Comb finite and sustained");p.set(13,-24);Audio quiet(SR);process(p,quiet);check(rms(quiet,SR/2)<rms(hold,29*SR)*.5,"Output remains active while HOLD is frozen");}
 if(argc>1){const int progs[]={10,11,12,13,16};const char*names[]={"01-metal-comb-plucks.wav","02-inverted-comb-drums.wav","03-bell-comb.wav","04-robot-voice-synthetic.wav","05-hold-drone-comb.wav"};for(int d=0;d<5;d++){
  EarlyDelay p;prepare(p);p.setProgram(progs[d]);Audio result(8*SR);rng=1471;
  for(int block=0;block<8*SR;block+=128){int n=std::min(128,8*SR-block);if(d==4&&block>=2*SR){p.set(9,1);p.set(0,100);if(block>=4*SR)p.set(1,3.5);if(block>=6*SR)p.set(1,10.5);}
   for(int k=0;k<n;k++){int i=block+k;double t=double(i)/SR,phase=fmod(t,.7);float x=0;if(d==3){if(t<5.8)x=.27*(sin(2*PI*110*t)+.4*sin(2*PI*220*t)+.2*sin(2*PI*440*t))*(.5+.5*sin(2*PI*3*t));}else if(d==4&&t<2)x=.3*(sin(2*PI*110*t)+.4*noise());else if(t<5.5&&d!=4)x=.65*noise()*exp(-phase*(d==2?280:70));result.l[i]=result.r[i]=x;}
   float* io[]={result.l.data()+block,result.r.data()+block};counting=true;p.internalProcess(io,io,n);counting=false;
  }
  double pk=peak(result);printf("DEMO %s peak_dbfs=%.3f rms_dbfs=%.3f frames=%zu\n",names[d],db(pk),db(rms(result)),result.l.size());check(pk<.8913,"demo unnormalized peak below -1 dBFS");if(pk<1)wav(std::string(argv[1])+"/"+names[d],result);
 }}
 check(callbackAllocations==0,"zero C++ allocations in actual plugin audio callbacks");printf("V05_REGRESSION failures=%d callback_allocations=%zu\n",failures,callbackAllocations);return failures?1:0;
}
