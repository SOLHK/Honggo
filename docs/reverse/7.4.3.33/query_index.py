#!/usr/bin/env python3
"""Query the cached ObjC metadata without loading or reanalysing the IPA."""
import argparse,gzip,json,re
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--class',dest='class_pattern',default='.*')
p.add_argument('--selector',default='.*')
p.add_argument('--all',action='store_true',help='Use full 78,840 class index, rather than advertising candidates')
p.add_argument('--limit',type=int,default=100)
a=p.parse_args()
f=Path(__file__).parent/('objc-all.json.gz' if a.all else 'ad-candidates.json.gz')
data=json.loads(gzip.decompress(f.read_bytes()))
cp,mp=re.compile(a.class_pattern),re.compile(a.selector)
n=0
for c in data['classes']:
 if not cp.search(c['name']):continue
 for m in c['methods']:
  if not mp.search(m['selector']):continue
  print(json.dumps({'class':c['name'],**m},ensure_ascii=False));n+=1
  if n>=a.limit:raise SystemExit(0)
