// Formant measurement with PROMINENCE-based peak detection.
// Only genuine spectral peaks count; a shoulder on the skirt of a lower
// resonance is rejected. Matching to the 5 reference formants is order-constrained.
#include "TbEngine.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
using namespace tb;

struct Ref{const char*n;float f[5];};
static const Ref R[9]={
 {"ee",{270,2290,3010,3400,3900}},{"ih",{390,1990,2550,3350,3850}},
 {"eh",{530,1840,2480,3300,3800}},{"ae",{660,1720,2410,3300,3750}},
 {"ah",{730,1090,2440,3200,3700}},{"aw",{570, 840,2410,3150,3650}},
 {"oh",{440,1020,2240,3100,3600}},{"oo",{300, 870,2240,3050,3550}},
 {"uh",{500,1500,2500,3300,3800}}};

// Reads the pad coordinates from the table itself; works against either the
// v1.0.0 layout (x/y inside VowelAnchor) or v1.1.0 (separate kVowelPadXY).
#if defined(TB_LEGACY_TABLE)
static inline float AX(int v){return kVowels[v].x;} static inline float AY(int v){return kVowels[v].y;}
#else
static inline float AX(int v){return kVowelPadXY[v][0];} static inline float AY(int v){return kVowelPadXY[v][1];}
#endif

static float gainAt(VocalTract& tr,float f,float fs){
    tr.reset(); std::vector<float> L(128);
    double acc=0; int cnt=0; const int BL=48;
    for(int b=0;b<BL;++b){
        for(int i=0;i<128;++i){ double t=(double)(b*128+i)/fs; L[(size_t)i]=(float)std::sin(2.0*kPi*f*t); }
        for(int i=0;i<128;++i) L[(size_t)i]=tr.process(L[(size_t)i]);
        if(b>=BL/2) for(int i=0;i<128;++i){ double t=(double)(b*128+i)/fs; acc+=L[(size_t)i]*std::sin(2.0*kPi*f*t)*2.0; ++cnt; }
    }
    return (float)std::fabs(acc/(double)cnt);
}

struct Peak{float f,m,pr;};
static std::vector<Peak> peaksOf(const std::vector<float>& s,const std::vector<float>& fr){
    std::vector<Peak> out;
    for(size_t i=2;i+2<s.size();++i){
        if(!(s[i]>s[i-1]&&s[i]>=s[i+1])) continue;
        float v=s[i];
        float minL=v,minR=v;
        for(long j=(long)i-1;j>=0;--j){ if(s[(size_t)j]>v) break; minL=std::min(minL,s[(size_t)j]); }
        for(size_t j=i+1;j<s.size();++j){ if(s[j]>v) break; minR=std::min(minR,s[j]); }
        float pr=v-std::max(minL,minR);
        if(pr>0.02f*v) out.push_back({fr[i],v,pr});
    }
    std::sort(out.begin(),out.end(),[](const Peak&a,const Peak&b){return a.f<b.f;});
    return out;
}

int main(){
    const float fs=48000.f;
    double S[5]={0,0,0,0,0}; int n[5]={0,0,0,0,0};
    printf("%-4s %-14s %-30s %s\n","","picos hallados","asignados a F1..F5","error %");
    printf("-----+---------------+------------------------------+-----------------\n");
    for(int v=0;v<9;++v){
        Formants tgt=padToFormants(AX(v),AY(v),1.f,0.f);
        VocalTract tr; tr.prepare(fs); tr.setTarget(tgt,0.f,0.8f,0.f);
        for(int i=0;i<60;++i) tr.updateCoeffs();
        std::vector<float> fr,mg;
        for(float f=120.f;f<=4600.f;f+=2.f){ fr.push_back(f); mg.push_back(gainAt(tr,f,fs)); }
        std::vector<float> s(mg.size()); int h=3;
        for(size_t i=0;i<mg.size();++i){double a=0;int c=0;
            for(int k=-h;k<=h;++k){long j=(long)i+k;if(j>=0&&j<(long)mg.size()){a+=mg[(size_t)j];++c;}}
            s[i]=(float)(a/c);}
        auto pk=peaksOf(s,fr);
        char pb[64]=""; int off=0;
        for(size_t i=0;i<pk.size()&&i<6;++i) off+=snprintf(pb+off,sizeof(pb)-off,"%.0f ",pk[i].f);
        // order-constrained greedy match: for each reference formant in turn,
        // take the nearest un-used peak above the previous assignment
        float got[5]; bool ok[5]; float prev=0.f; size_t si=0;
        for(int k=0;k<5;++k){
            ok[k]=false; got[k]=0; float bestd=1e9f; size_t bi=0; bool found=false;
            for(size_t i=si;i<pk.size();++i){
                float d=std::fabs(pk[i].f-R[v].f[k]);
                if(d<bestd){bestd=d;bi=i;found=true;}
                if(pk[i].f>R[v].f[k]*1.9f) break;
            }
            if(found&&bestd<=0.30f*R[v].f[k]){ got[k]=pk[bi].f; ok[k]=true; si=bi+1; }
            else { got[k]=R[v].f[k]; ok[k]=false; }
        }
        char ab[64]="",eb[64]="";
        for(int k=0;k<5;++k){
            if(ok[k]){ float e=std::fabs(got[k]-R[v].f[k])/R[v].f[k]*100.f; S[k]+=e; ++n[k];
                       snprintf(ab+strlen(ab),sizeof(ab)-strlen(ab),"%4.0f ",got[k]);
                       snprintf(eb+strlen(eb),sizeof(eb)-strlen(eb),"%4.1f ",e); }
            else { snprintf(ab+strlen(ab),sizeof(ab)-strlen(ab)," --- "); snprintf(eb+strlen(eb),sizeof(eb)-strlen(eb)," miss "); }
        }
        printf("%-4s %-14s %-30s %s\n",R[v].n,pb,ab,eb);
    }
    printf("\nMEDIA %% error: ");
    double tot=0; int c=0;
    for(int k=0;k<5;++k){
        printf("F%d %.1f (%d/9)  ",k+1,n[k]?S[k]/n[k]:0.0,n[k]);
        if(n[k]){tot+=S[k]/n[k];++c;}
    }
    printf("\nGLOBAL (media de F1-F5 con deteccion) = %.1f%%\n",c?tot/c:0);
    return 0;
}
