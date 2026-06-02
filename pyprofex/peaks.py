#!/usr/bin/env python3
"""
pyprofex.peaks — XRD 数据峰位查找引擎
"""
from __future__ import annotations
import math
from typing import Any, List, Tuple

def parse_xrd_file(filepath: str) -> list[tuple[float, float]]:
    data = []
    with open(filepath) as f:
        for line in f:
            line = line.strip()
            if not line: continue
            if line.startswith(("/","#","<")): continue
            first = line.split()[0] if line.split() else ""
            if first in ["voltage","current","Slits","Scanning","Group","Sample","diffractometer"]: continue
            if "=" in first: continue
            parts = line.split()
            if len(parts) >= 2:
                try: data.append((float(parts[0]), float(parts[1])))
                except: continue
    return data

def smooth_and_background(intensities, smooth_window=2, bg_window_ratio=0.03):
    n = len(intensities)
    if n < 5: return list(intensities), [0.0]*n
    sm = []
    for i in range(n):
        s=max(0,i-smooth_window); e=min(n,i+smooth_window+1)
        sm.append(sum(intensities[s:e])/(e-s))
    bw = max(10, int(n*bg_window_ratio)); bg = []
    for i in range(n):
        s=max(0,i-bw); e=min(n,i+bw+1); bg.append(min(sm[s:e]))
    cor = [max(0.0, sm[i]-bg[i]) for i in range(n)]
    return cor, bg

def peak_prominence(cor, i, window=8):
    lm = min(cor[max(0,i-window):i+1])
    rm = min(cor[i:min(len(cor),i+window+1)])
    return cor[i]-min(lm,rm)

def gaussian_refine(cor, angles, i, half_window=3):
    n=len(cor)
    if i<half_window or i>=n-half_window: return angles[i]
    xv=angles[i-half_window:i+half_window+1]; yv=cor[i-half_window:i+half_window+1]
    xf=[]; ly=[]
    for x,y in zip(xv,yv):
        if y>0: xf.append(x); ly.append(math.log(y))
    if len(xf)<3: return angles[i]
    nf=len(xf); sx=sum(xf); sx2=sum(x*x for x in xf); sx3=sum(x*x*x for x in xf)
    sx4=sum(x*x*x*x for x in xf); sy=sum(ly); sxy=sum(x*y for x,y in zip(xf,ly))
    sx2y=sum(x*x*y for x,y in zip(xf,ly))
    det=nf*(sx2*sx4-sx3*sx3)-sx*(sx*sx4-sx2*sx3)+sx2*(sx*sx3-sx2*sx2)
    if abs(det)<1e-20: return angles[i]
    inv=1.0/det
    a=(nf*(sx2*sx2y-sx3*sxy)-sx*(sx*sx2y-sx2*sxy)+sx2*(sx*sxy-sx2*sy))*inv
    b=(nf*(sx3*sy-sx2*sxy)-sx*(sx4*sy-sx2*sx2y)+sx2*(sx4*sxy-sx3*sx2y))*inv
    if abs(a)<1e-12: return angles[i]
    x0=-b/(2*a)
    return max(xv[0], min(x0, xv[-1]))

def find_peaks(data, min_prominence=60, refine_method="gaussian", smooth_window=2, bg_window_ratio=0.03):
    if len(data)<5: return []
    angles=[d[0] for d in data]; intens=[d[1] for d in data]; step=angles[1]-angles[0] if len(angles)>1 else 0.02
    cor,_=smooth_and_background(intens, smooth_window, bg_window_ratio)
    def get_win(a):
        return 12 if a<20 else (8 if a<40 else 6)
    peaks=[]
    for i in range(1,len(cor)-1):
        if cor[i]>cor[i-1] and cor[i]>=cor[i+1]:
            prom=peak_prominence(cor,i,get_win(angles[i]))
            if prom<min_prominence: continue
            if refine_method=="gaussian":
                ra=gaussian_refine(cor,angles,i)
            else:
                if i<=0 or i>=len(cor)-1: ra=angles[i]
                else:
                    d=2*cor[i]-cor[i-1]-cor[i+1]
                    ra=angles[i]+(cor[i-1]-cor[i+1])/(d*2)*step if abs(d)>1e-10 else angles[i]
            tr=math.radians(ra/2.0)
            d_val=1.54056/(2*math.sin(tr)) if math.sin(tr)>0 else 999
            peaks.append({"angle":round(ra,4),"d":round(d_val,4),"intensity":round(cor[i],1),"prominence":round(prom,1)})
    peaks.sort(key=lambda p:p["intensity"], reverse=True)
    return peaks

def adaptive_tolerance(d, base_tol=0.025):
    if d<2.0: return base_tol
    elif d<4.0: return base_tol*1.3
    else: return base_tol*1.6

def match_d_adaptive(obs_d, ref_d, base_tol=0.025):
    if ref_d<=0: return False
    return abs(obs_d-ref_d)/ref_d <= adaptive_tolerance(ref_d, base_tol)

def detect_amorphous(data, min_width_deg=2.5, threshold_rel=0.01):
    """检测非晶鼓包。限制8-80°防止假阳性。"""
    if len(data)<50: return []
    angles=[d[0] for d in data]; intens=[d[1] for d in data]; step=angles[1]-angles[0] if len(angles)>1 else 0.02
    mi=max(intens) if max(intens)>0 else 1
    w=3; sm=[]
    for i in range(len(intens)):
        s=max(0,i-w); e=min(len(intens),i+w+1); sm.append(sum(intens[s:e])/(e-s))
    bw1=max(10,int(0.6/step)); bg1=[]
    for i in range(len(sm)):
        s=max(0,i-bw1); e=min(len(sm),i+bw1+1); bg1.append(min(sm[s:e]))
    bw2=max(30,int(4.0/step)); bg2=[]
    for i in range(len(sm)):
        s=max(0,i-bw2); e=min(len(sm),i+bw2+1); bg2.append(min(sm[s:e]))
    amor=[max(0.0,(bg1[i]-bg2[i])/mi) for i in range(len(sm))]
    bumps=[]; ib=False; bs=0
    for i in range(len(amor)):
        ang=angles[i] if i<len(angles) else 999
        if ang<8.0 or ang>80.0:
            if ib:
                wd=(i-bs)*step
                if wd>min_width_deg:
                    c=angles[(bs+i)//2]; ar=sum(amor[bs:i])*step; h=max(amor[bs:i])
                    if h>threshold_rel: bumps.append({"center_2theta":round(c,1),"center_d":round(1.54056/(2*math.sin(math.radians(c/2))),2) if math.sin(math.radians(c/2))>0 else 0,"width_deg":round(wd,1),"height_rel":round(h,3),"area_rel":round(ar,3)})
                ib=False
            continue
        if amor[i]>threshold_rel:
            if not ib: ib=True; bs=i
        elif ib:
            wd=(i-bs)*step
            if wd>min_width_deg:
                c=angles[(bs+i)//2]; ar=sum(amor[bs:i])*step; h=max(amor[bs:i])
                if h>threshold_rel: bumps.append({"center_2theta":round(c,1),"center_d":round(1.54056/(2*math.sin(math.radians(c/2))),2) if math.sin(math.radians(c/2))>0 else 0,"width_deg":round(wd,1),"height_rel":round(h,3),"area_rel":round(ar,3)})
            ib=False
    if ib:
        wd=(len(amor)-bs)*step
        if wd>min_width_deg:
            c=angles[(bs+len(amor))//2]; ar=sum(amor[bs:])*step; h=max(amor[bs:])
            if h>threshold_rel: bumps.append({"center_2theta":round(c,1),"center_d":round(1.54056/(2*math.sin(math.radians(c/2))),2) if math.sin(math.radians(c/2))>0 else 0,"width_deg":round(wd,1),"height_rel":round(h,3),"area_rel":round(ar,3)})
    return bumps

def full_analysis(filepath, elements=None, n_expected=0, refine_method="gaussian", do_amorphous=True, top_n_peaks=15):
    """全分析管线：峰查找+物相鉴定+非晶检测"""
    result={"file":str(filepath)}
    data=parse_xrd_file(filepath)
    if not data: return {"error":"无法读取数据"}
    result["data_points"]=len(data); result["angle_range"]=[data[0][0],data[-1][0]]
    peaks=find_peaks(data,min_prominence=60,refine_method=refine_method)
    result["peaks_found"]=len(peaks); result["top_peaks"]=peaks[:top_n_peaks]
    if elements:
        try:
            from search_match import independent_score_all_adaptive as matcher
        except ImportError:
            from search_match import independent_score_all as matcher
        sm=matcher([p["d"] for p in peaks[:top_n_peaks]],elements,n_expected=n_expected)
        result["search_match"]=sm
    if do_amorphous:
        bumps=detect_amorphous(data)
        result["amorphous"]=bumps if bumps else []
    return result
