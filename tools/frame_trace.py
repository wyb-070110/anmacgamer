#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
"""Summarises a GS_FRAME_TRACE file (GameScrcpy's per-frame timing log): phone + cable delay spread, decode,
wait, draw, received-to-screen, how evenly frames reach the screen, and frames per second.

    GS_FRAME_TRACE=/tmp/trace.csv open build/mac-app/bin/anmacgamer.app   # then mirror and swipe
    tools/frame_trace.py /tmp/trace.csv
"""
import sys,collections
P=[];D=[]
for l in open(sys.argv[1]):
    f=l.split()
    if f[0]=='p' and len(f)==5: P.append((int(f[1]),int(f[2]),int(f[3])))
    if f[0]=='d' and len(f)==6: D.append(tuple(map(int,f[1:])))
def q(a,x): a=sorted(a); return round(a[int(x*(len(a)-1))],1)
put={p:n for n,p,r in P}
off=[r-p for _,p,r in P]; b=min(off); lag=[(o-b)/1000 for o in off]
dec=[(put[d[1]]-d[2])/1000 for d in D if d[1] in put]
wait=[(d[3]-put[d[1]])/1000 for d in D if d[1] in put]
draw=[(d[4]-d[3])/1000 for d in D]
tot=[(d[4]-d[2])/1000 for d in D]
g=[(D[i][4]-D[i-1][4])/1000 for i in range(1,len(D))]
busy=[x for x in g if x<60]
print(f"frames {len(P)} shown {len(D)}")
for n,a in (("phone+cable extra",lag),("decode",dec),("wait",wait),("draw",draw),("received->screen",tot)):
    print(f"{n:18} p50 {q(a,.5)} p90 {q(a,.9)} p99 {q(a,.99)} max {round(max(a),1)}")
print("screen gaps <60ms: even 15-18ms %.0f%%, short<15 %d, long>18 %d, >30 %d"%(100*sum(15<=x<=18.5 for x in busy)/len(busy),sum(x<15 for x in busy),sum(x>18.5 for x in busy),sum(x>30 for x in busy)))
sec=collections.Counter(int((r-P[0][2])/1e6) for _,_,r in P)
print("fps per second:",[sec[s] for s in sorted(sec)])
