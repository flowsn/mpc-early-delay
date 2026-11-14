#define main first_main
#include "probe.cpp"
#undef main
double amp(const std::vector<float>&a,int st,int en,double hz){double re=0,im=0,ws=0;for(int i=st;i<en;i++){double w=.5-.5*cos(2*pi*(i-st)/(en-st-1));re+=w*a[i]*cos(2*pi*hz*i/fs);im+=w*a[i]*sin(2*pi*hz*i/fs);ws+=w;}return 2*hypot(re,im)/ws;}
int main(){
 for(float t:{10.f,300.f,900.f})for(double hz:{1000.,4000.,8000.,12000.}){auto p=wet();p.timeMs=t;Engine e;e.prepare(fs);e.setParams(p);Audio a(3*fs);sine(a,hz);render(e,a,192);double v=amp(a.l,2*fs,3*fs,hz);printf("BAND time=%.0f hz=%.0f fundamental_dB=%.3f\n",t,hz,db(v/.25));}
 for(float t:{1.75f,10.f,100.f,300.f,900.f}){auto p=wet();p.timeMs=t;Engine e;e.prepare(fs);e.setParams(p);Audio a(2*fs);a.l[1000]=a.r[1000]=.5f;render(e,a,37);int at=0;float pk=0;for(int i=1000;i<fs+1000;i++)if(fabs(a.l[i])>pk){pk=fabs(a.l[i]);at=i;}printf("IMPULSE time=%.2f measured_ms=%.4f peak_dB=%.3f\n",t,(at-1000)*1000/fs,db(pk));}
 // Repeat polarity against first echo and sustained HOLD release returns to input.
 for(float fb:{-1.f,1.f}){auto p=wet();p.timeMs=200;p.feedback=fb;Engine e;e.prepare(fs);e.setParams(p);Audio a(12*fs);for(int i=0;i<2205;i++)a.l[i]=a.r[i]=.25*sin(2*pi*500*i/fs);render(e,a,64);for(int repeat:{1,2,5,10,20,40}){int st=repeat*8820;printf("REPEAT fb=%+.0f n=%d RMS_dB=%.3f\n",fb,repeat,db(rms(a.l,st,st+2205)));}}
 return 0;
}
