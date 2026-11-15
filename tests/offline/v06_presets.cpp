// v0.6 measurement/demos of the actual plugin wrapper, not a preset reimplementation.
// Reuse tested WAV/audio helpers and callback allocation instrumentation.
#define main v05_regression_main
#include "v05_regression.cpp"
#undef main
#include <complex>
#include <array>
void fft(std::vector<std::complex<double>>& x){
 const int n=x.size();
 for(int i=1,j=0;i<n;i++){int bit=n>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(x[i],x[j]);}
 for(int len=2;len<=n;len<<=1){auto step=std::polar(1.0,-2*PI/len);for(int i=0;i<n;i+=len){std::complex<double>w=1;for(int j=0;j<len/2;j++){auto a=x[i+j],b=w*x[i+j+len/2];x[i+j]=a+b;x[i+j+len/2]=a-b;w*=step;}}}
}
std::vector<double> spectrum(const Audio&a,int start,int windows){
 constexpr int N=16384;std::vector<double> out(N/2);std::vector<std::complex<double>> bins(N);
 for(int w=0;w<windows;w++){
  for(auto*channel:{&a.l,&a.r}){
   for(int i=0;i<N;i++)bins[i]=(*channel)[start+w*N+i]*(.5-.5*cos(2*PI*i/(N-1)));
   fft(bins);for(int i=0;i<N/2;i++)out[i]+=std::norm(bins[i]);
  }
 }
 return out;
}
double dominant(const std::vector<double>&s){int k=26;for(int i=27;i<1486;i++)if(s[i]>s[k])k=i;return k*SR/16384.0;}
int main(int argc,char**argv){
 setvbuf(stdout,nullptr,_IOLBF,0);
 std::array<std::vector<double>,7> specs;
 printf("MEASURE 44100Hz, actual preset with Mix=100 only, deterministic white noise +/-0.35, 2s warmup, 8 nonoverlap Hann16384 windows, summed L/R power, peak70..4000Hz; centroid70..8000Hz\n");
 for(int index=10;index<=16;index++){
  EarlyDelay p;prepare(p);p.setProgram(index);p.set(0,100);Audio a(6*SR);rng=1471;
  for(int i=0;i<6*SR;i++)a.l[i]=a.r[i]=.35f*noise();process(p,a);
  auto s=spectrum(a,2*SR,8);specs[index-10]=s;
  double power=0,weighted=0;for(int i=26;i<2972;i++){power+=s[i];weighted+=s[i]*i*SR/16384.0;}
  auto early=spectrum(a,2*SR,2),late=spectrum(a,4*SR,2);
  printf("SPECTRUM %d %s peak_hz=%.2f centroid_hz=%.2f early_peak_hz=%.2f late_peak_hz=%.2f peak_dbfs=%.3f\n",index,p.programName(index),dominant(s),weighted/power,dominant(early),dominant(late),db(peak(a)));
  check(peak(a)<1&&power>0,"wet measurement finite and nonzero");
 }
 double minDistance=2;
 for(int a=0;a<7;a++)for(int b=a+1;b<7;b++){
  double sa=0,sb=0;for(int k=26;k<1486;k++){sa+=specs[a][k];sb+=specs[b][k];}
  double dist=0;for(int k=26;k<1486;k++)dist+=fabs(specs[a][k]/sa-specs[b][k]/sb);
  minDistance=std::min(minDistance,dist);printf("DISTANCE %d %d normalized_power_L1=%.5f\n",a+10,b+10,dist);
 }
 printf("MIN_SPECTRAL_L1 %.5f\n",minDistance);check(minDistance>.1,"all 21 comb pairs differ in normalized spectral shape (L1 > 0.1)");
 if(argc>1){
  const char* names[]={"01-seven-combs-comparison.wav","02-chorus-flange-tour.wav","03-robot-voice-flutter.wav","04-stereo-flange-plucks.wav","05-hold-drone-capture.wav"};
  const std::vector<std::vector<int>> banks={{10,11,12,13,14,15,16},{7,5,17,18,19},{13},{15},{16}};
  for(int demo=0;demo<5;demo++){
   int seconds=demo==0?3:demo==1?4:8;Audio result(seconds*SR*banks[demo].size());
   for(size_t segment=0;segment<banks[demo].size();segment++){
    EarlyDelay p;prepare(p);p.setProgram(banks[demo][segment]);rng=1471;
    for(int block=0;block<seconds*SR;block+=128){int n=std::min(128,seconds*SR-block);int dst=segment*seconds*SR+block;
     if(demo==4&&block>=2*SR){p.set(9,1);p.set(0,100);if(block>=4*SR)p.set(1,5.5);if(block>=6*SR)p.set(1,16.5);}
     for(int k=0;k<n;k++){double t=double(block+k)/SR,phase=fmod(t,.65);float x=0;
      if(demo==1&&t<3.2)x=.23*(sin(2*PI*110*t)+.4*sin(2*PI*220*t)+.2*sin(2*PI*440*t))*(1-exp(-t*30));
      else if(demo==2&&t<5.8)x=.27*(sin(2*PI*110*t)+.4*sin(2*PI*220*t)+.2*sin(2*PI*440*t))*(.5+.5*sin(2*PI*3*t));
      else if(demo==4&&t<2)x=.3*(sin(2*PI*110*t)+.4*noise());
      else if(demo!=1&&demo!=2&&demo!=4&&t<seconds-.65)x=.65*noise()*exp(-phase*70);
      result.l[dst+k]=result.r[dst+k]=x;
     }
     float* io[]={result.l.data()+dst,result.r.data()+dst};counting=true;p.internalProcess(io,io,n);counting=false;
     // 10ms boundary fade only, never normalize or alter factory output gain.
     for(int k=0;k<n;k++){double t=double(block+k)/SR;float fade=std::min(1.,std::min(t/.01,(seconds-t)/.01));result.l[dst+k]*=fade;result.r[dst+k]*=fade;}
    }
   }
   double pk=peak(result);printf("DEMO %s peak_dbfs=%.3f rms_dbfs=%.3f frames=%zu\n",names[demo],db(pk),db(rms(result)),result.l.size());check(pk<.8913,"demo below -1 dBFS without normalization");if(pk<1)wav(std::string(argv[1])+"/"+names[demo],result);
  }
 }
 check(callbackAllocations==0,"zero wrapper callback C++ allocations in measurement/demo runs");printf("V06_PRESETS failures=%d callback_allocations=%zu\n",failures,callbackAllocations);return failures?1:0;
}
