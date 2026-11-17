// FINAL calibration. Chooses the 5 input formant frequencies so that the
// physical cascade REALISES the Peterson & Barney adult-male targets, scored
// with prominence-based peak detection (the metric used for the report).
#include "TbEngine.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
using namespace tb;

struct Ref{const char*n;float f[5];float b[3];};
static const Ref R[9]={
 {"ee",{270,2290,3010,3400,3900},{60,90,150}},
 {"ih",{390,1990,2550,3350,3850},{65,95,155}},
 {"eh",{530,1840,2480,3300,3800},{70,100,160}},
 {"ae",{660,1720,2410,3300,3750},{80,110,165}},
 {"ah",{730,1090,2440,3200,3700},{85,110,170}},
 {"aw",{570, 840,2410,3150,3650},{75,95,160}},
 {"oh",{440,1020,2240,3100,3600},{65,85,150}},
 {"oo",{300, 870,2240,3050,3550},{60,80,140}},
 {"uh",{500,1500,2500,3300,3800},{70,100,160}}};

static float gAt(VocalTract& tr,float f,float fs){
    tr.reset(); double acc=0; int cnt=0; const int BL=16;
    static std::vector<float> L(128);
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
// score: sum of relative errors of the 5 order-matched peaks, miss = 0.35
static double score(const float* in,const Ref& r,float fs,float step,int hz){
    Formants t;
    for(int i=0;i<5;++i) t.f[i]=in[i];
    for(int i=0;i<3;++i) t.b[i]=r.b[i];
    t.b[3]=r.b[3]; t.b[4]=r.b[4];
    for(int i=1;i<5;++i) if(t.f[i]<=t.f[i-1]+90.f) return 1e6;
    auto pk=pkOf(t,fs,step,hz);
    double c=0; size_t si=0;
    for(int k=0;k<5;++k){
        float bd=1e9f; size_t bi=0; bool f2=false;
        for(size_t i=si;i<pk.size();++i){
            float d=std::fabs(pk[i].f-r.f[k]);
            if(d<bd){bd=d;bi=i;f2=true;}
            if(pk[i].f>r.f[k]*1.85f) break;
        }
        if(!f2||bd>0.30f*r.f[k]) c+=0.35; else { c+=bd/r.f[k]; si=bi+1; }
    }
    return c;
}
int main(int argc,char**argv){
    const float fs=48000.f; const float step=12.f; const int hz=12;
    // B4/B5 from the grid study
    static const float B45[9][2]={{200,150},{200,150},{200,200},{200,200},{200,200},
                                  {200,150},{80,100},{80,100},{200,150}};
    // initial values: grid-study F4/F5 + old pre-compensated F1-F3
    static const float O13[9][3]={{300,2200,3000},{400,1900,2500},{500,1800,2500},
        {660,1700,2400},{750,1200,2500},{500,900,2500},{400,850,2500},{320,700,2300},{500,1500,2500}};
    static const float ST45[9][2]={{3550,3800},{3500,3750},{3450,3700},{3450,3650},
        {3350,3600},{3000,3250},{3100,3650},{3050,3300},{3450,3700}};
    printf("Optimizacion final (F1-F5 libres), metrica = prominencia\n");
    float RES[9][5];
    for(int v=0;v<9;++v){
        Ref r=R[v]; r.b[3]=B45[v][0]; r.b[4]=B45[v][1];
        float in[5]={O13[v][0],O13[v][1],O13[v][2],ST45[v][0],ST45[v][1]};
        double best=score(in,r,fs,step,hz);
        printf("%-3s start %.3f", r.n, best); fflush(stdout);
        for(int pass=0;pass<7;++pass){
            float sv=(pass<3)?100.f:((pass<5)?40.f:15.f);
            for(int k=0;k<5;++k){
                for(int dir=-1;dir<=1;dir+=2){
                    int guard=0;
                    while(guard++<12){
                        float tr[5]; for(int i=0;i<5;++i) tr[i]=in[i];
                        tr[k]+=dir*sv;
                        if(tr[k]<120.f||tr[k]>4650.f) break;
                        double c2=score(tr,r,fs,step,hz);
                        if(c2<best-1e-4){best=c2;for(int i=0;i<5;++i) in[i]=tr[i];}
                        else break;
                    }
                }
            }
        }
        for(int i=0;i<5;++i) RES[v][i]=std::round(in[i]);
        printf("  -> cost %.3f  :  %4.0f %4.0f %4.0f %4.0f %4.0f\n",best,
               RES[v][0],RES[v][1],RES[v][2],RES[v][3],RES[v][4]); fflush(stdout);
    }
    printf("\n===== TABLA FINAL =====\n");
    for(int v=0;v<9;++v)
        printf("    {\"%s\", %4.0f,%4.0f,%4.0f,%4.0f,%4.0f,   %3.0f,%3.0f,%3.0f,%3.0f,%3.0f},\n",
            R[v].n,RES[v][0],RES[v][1],RES[v][2],RES[v][3],RES[v][4],
            R[v].b[0],R[v].b[1],R[v].b[2],B45[v][0],B45[v][1]);
    printf("\n===== OBJETIVOS P&B (solo para comparar) =====\n");
    for(int v=0;v<9;++v)
        printf("    %-3s %4.0f %4.0f %4.0f %4.0f %4.0f\n",R[v].n,
            R[v].f[0],R[v].f[1],R[v].f[2],R[v].f[3],R[v].f[4]);
    return 0;
}
