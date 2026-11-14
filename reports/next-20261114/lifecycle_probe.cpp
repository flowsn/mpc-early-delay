#define main first_main
#include "probe.cpp"
#undef main
int main(){
 for(int history:{44103,44160}){auto p=wet();p.timeMs=1.75;Engine e;e.prepare(fs);auto old=p;old.timeMs=900;old.bandwidth=0;old.drive=1;e.setParams(old);Audio prim(history);sine(prim,1000);render(e,prim);e.reset();e.setParams(p);Audio a(4096);sine(a,1000);render(e,a);Engine ref;ref.prepare(fs);ref.setParams(p);Audio b(4096);sine(b,1000);render(ref,b);double d=0;for(int i=0;i<4096;i++)d=std::max(d,double(fabs(a.l[i]-b.l[i])));printf("RESET history=%d max_delta=%.9g\n",history,d);}
 for(float sr:{44100.f,48000.f,96000.f}){auto p=wet();p.timeMs=100;Engine e;e.prepare(sr);e.setParams(p);Audio a(sr);a.l[1000]=a.r[1000]=.5;render(e,a,37);int at=0;float pk=0;int nf=0;for(int i=1000;i<(int)a.l.size();i++){nf+=!std::isfinite(a.l[i]);if(fabs(a.l[i])>pk){pk=fabs(a.l[i]);at=i;}}printf("SAMPLE_RATE sr=%.0f delay_ms=%.4f nonfinite=%d\n",sr,(at-1000)*1000./sr,nf);}
 auto p=wet();p.timeMs=200;p.grit=.7;p.feedback=-1;Engine e;e.prepare(fs);e.setParams(p);Audio source(fs);sine(source,723,.35);render(e,source);p.hold=true;e.setParams(p);Audio h(65*fs);render(e,h,96);for(int t:{2,10,30,60})printf("HOLD_SUSTAIN second=%d RMS_dB=%.4f\n",t,db(rms(h.l,t*fs,(t+1)*fs)));p.hold=false;e.setParams(p);Audio z(15*fs);render(e,z,96);printf("HOLD_RELEASE early_RMS_dB=%.3f final_RMS_dB=%.3f\n",db(rms(z.l,fs,2*fs)),db(rms(z.l,14*fs,15*fs)));
 return 0;
}
