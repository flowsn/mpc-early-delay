#include <cstdio>
#include <cmath>
#include <limits>
#include <vector>
#include <algorithm>
#include "EarlyDelay.cpp"
int main(int argc,char**argv){
 EarlyDelay p;p.internalSetSampleRate(44100);p.internalSetBlockSize(128);p.internalResume();
 float l[128]={},r[128]={};float* io[]={l,r};
 if(argc>1){int ix=std::atoi(argv[1]);p.setNormalised(ix,argc>2?std::strtof(argv[2],nullptr):std::numeric_limits<float>::quiet_NaN());for(int b=0;b<10;b++){for(int i=0;i<128;i++)l[i]=r[i]=.25f*std::sin(.05f*(128*b+i));p.internalProcess(io,io,128);}int nf=0;for(float x:l)nf+=!std::isfinite(x);printf("NAN_PARAMETER index=%d nonfinite=%d\n",ix,nf);return 0;}
 printf("PARAMS %d PROGRAMS %d\n",p.numParams(),p.numPrograms());
 int failures=0;for(int i=0;i<p.numPrograms();i++){p.setProgram(i);std::vector<float> before;for(int k=0;k<p.numParams();k++)before.push_back(p.getNormalised(k));auto state=p.internalSaveState();for(int k=0;k<p.numParams();k++)p.setNormalised(k,.123f);bool ok=p.internalLoadState(state.data(),state.size());for(int k=0;k<p.numParams();k++)ok&=before[k]==p.getNormalised(k);p.internalProcess(io,io,128);for(float x:l)ok&=std::isfinite(x);printf("PROGRAM %d %s roundtrip=%d chunk_bytes=%zu\n",i,p.programName(i),ok,state.size());failures+=!ok;}
 for(int i=0;i<p.numParams();i++)printf("PARAM %d id=%s name=%s\n",i,p.param(i).id.c_str(),p.param(i).name.c_str());
 return failures?1:0;
}
