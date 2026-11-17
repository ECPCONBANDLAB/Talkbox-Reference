// Interpolation fidelity: at arbitrary pad positions the model should realise
// the IDW blend of the published P&B targets. Common reference for both builds.
#include "TbEngine.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
using namespace tb;

static const float PBF[9][5]={
 {270,2290,3010,3400,3900},{390,1990,2550,3350,3850},{530,1840,2480,3300,3800},
 {660,1720,2410,3300,3750},{730,1090,2440,3200,3700},{570, 840,2410,3150,3650},
 {440,1020,2240,3100,3600},{300, 870,2240,3050,3550},{500,1500,2500,3300,3800}};
static const float PBXY[9][2]={{0.95f,0.10f},{0.80f,0.28f},{0.70f,0.45f},{0.63f,0.75f},
 {0.33f,0.91f},{0.15f,0.45f},{0.12f,0.28f},{0.03f,0.13f},{0.51f,0.45f}};

// same IDW as padToFormants, but over the published acoustic targets
static void idealRef(float x,float y,float out[5]){
    float ws=0; float a[5]={0,0,0,0,0};
    for(int i=0;i<9;++i){
        float dx=x-PBXY[i][0],dy=y-PBXY[i][1];
        float d2=dx*dx+dy*dy+1e-6f; float w=1.f/d2; w=w*std::sqrt(w); ws+=w;
        for(int k=0;k<5;++k) a[k]+=w*PBF[i][k];
    }
    for(int k=0;k<5;++k) out[k]=a[k]/ws;
}
static float gAt(VocalTract& tr,float f,float fs){
    tr.reset(); double acc=0; int cnt=0; const int BL=16; std::vector<float> L(128);
    for(int b=0;b<BL;++b){
        for(int i=0;i<128;++i){ double t=(double)(b*128+i)/fs; L[(size_t)i]=(float)std::sin(2.0*kPi*f*t); }
        for(int i=0;i<128;++i) L[(size_t)i]=tr.process(L[(size_t)i]);
        if(b>=BL/2) for(int i=0;i<128;++i){ double t=(double)(b*128+i)/fs; acc+=L[(size_t)i]*std::sin(2.0*kPi*f*t)*2.0; ++cnt; }
    }
    return (float)std::fabs(acc/(double)cnt);
}
struct Pk{float f,m;};
static std::vector<Pk> pkOf(const Formants& t,float fs,float step,int hz){
    VocalTract tr; tr.prepare(fs); tr.setTarget(t,0.f,0.8f,0.f);
    for(int i=0;i<24;++i) tr.updateCoeffs();
    std::vector<float> fr,mg;
    for(float f=120.f;f<=4700.f;f+=step){ fr.push_back(f); mg.push_back(gAt(tr,f,fs)); }
    std::vector<float> s(mg.size()); int h=(int)std::lround((float)hz/step);
    for(size_t i=0;i<mg.size();++i){double a=0;int c=0;
        for(int k=-h;k<=h;++k){long j=(long)i+k;if(j>=0&&j<(long)mg.size()){a+=mg[(size_t)j];++c;}}
        s[i]=(float)(a/c);}
    std::vector<Pk> o;
    for(size_t i=2;i+2<s.size();++i){
        if(!(s[i]>s[i-1]&&s[i]>=s[i+1])) continue;
        float v=s[i],ml=v,mr=v;
        for(long j=(long)i-1;j>=0;--j){ if(s[(size_t)j]>v) break; ml=std::min(ml,s[(size_t)j]); }
        for(size_t j=i+1;j<s.size();++j){ if(s[j]>v) break; mr=std::min(mr,s[j]); }
        if(v-std::max(ml,mr)>0.015f*v) o.push_back({fr[i],v});
    }
    std::sort(o.begin(),o.end(),[](const Pk&a,const Pk&b){return a.f<b.f;});
    return o;
}
int main(){
    const float fs=48000.f; const float step=14.f; const int hz=14;
    // deterministic sample of interior pad positions
    std::vector<std::pair<float,float>> pts;
    for(int a=1;a<=4;++a) for(int b=1;b<=4;++b) pts.push_back({a*0.2f,b*0.2f});
    double S[5]={0,0,0,0,0}; int N[5]={0,0,0,0,0};
    printf("%-10s %-32s %s\n","pos","error F1..F5 (%)","");
    for(auto&p:pts){
        float x=p.first,y=p.second;
        float ref[5]; idealRef(x,y,ref);
        Formants t=padToFormants(x,y,1.f,0.f);
        auto pk=pkOf(t,fs,step,hz);
        float got[5]; bool ok[5]; size_t si=0;
        for(int k=0;k<5;++k){ ok[k]=false;got[k]=0; float bd=1e9f;size_t bi=0;bool f2=false;
            for(size_t i=si;i<pk.size();++i){ float d=std::fabs(pk[i].f-ref[k]);
                if(d<bd){bd=d;bi=i;f2=true;} if(pk[i].f>ref[k]*1.85f) break; }
            if(f2&&bd<=0.30f*ref[k]){got[k]=pk[bi].f;ok[k]=true;si=bi+1;} }
        printf("%.1f,%.1f   ",x,y);
        for(int k=0;k<5;++k){
            if(ok[k]){ double e=std::fabs(got[k]-ref[k])/ref[k]*100.0; S[k]+=e; ++N[k]; printf("%5.1f ",e); }
            else printf(" miss ");
        }
        printf("\n");
    }
    printf("\nMEDIA %%: ");
    double tot=0;int c=0;
    for(int k=0;k<5;++k){ printf("F%d %.1f (%d/%zu)  ",k+1,N[k]?S[k]/N[k]:0.0,N[k],pts.size());
        if(N[k]){tot+=S[k]/N[k];++c;} }
    printf("\nGLOBAL interpolacion = %.1f%%\n",c?tot/c:0);
    return 0;
}
